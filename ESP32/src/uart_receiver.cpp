// uart_receiver.cpp
#include "uart_receiver.h"
#include "logger.h"
#include "config.h"

void UartReceiver::begin(uint32_t baud, int rxPin, int txPin)
{
    port->begin(baud, SERIAL_8N1, rxPin, txPin);
    delay(500);
    Logger::info("UART receiver ready.");
}

void UartReceiver::parseJsonLine(const char *json, TelemetryPacket &out)
{
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json);
    if (err)
    {
        out.hasError = true;
        out.errorMsg = "JSON parse error: ";
        out.errorMsg += err.c_str();
        Serial.print("[PARSE ERROR] ");
        Serial.println(out.errorMsg);
        return;
    }

    out.hasError = false;
    out.errorMsg = "";

    out.src = String((const char *)(doc["src"] | ""));
    out.t_ms = doc["t_ms"] | 0;
    out.mode = String((const char *)(doc["mode"] | "UNKNOWN"));
    out.state = String((const char *)(doc["state"] | "UNKNOWN"));

    out.ax = doc["ax"] | 0;
    out.ay = doc["ay"] | 0;
    out.az = doc["az"] | 0;
    out.raw = doc["raw"] | 0;
    out.filtered = doc["filtered"] | 0;

    out.squat_count = doc["squat_count"] | 0;
    out.pushup_count = doc["pushup_count"] | 0;

    out.oled_mode = String((const char *)(doc["oled_mode"] | out.mode.c_str()));
    out.oled_squats = doc["oled_squats"] | out.squat_count;
    out.oled_pushups = doc["oled_pushups"] | out.pushup_count;

    if (out.src != "stm32")
    {
        out.hasError = true;
        out.errorMsg = "Unexpected src field";
    }

}

bool UartReceiver::tryReadPacket(TelemetryPacket &out)
{
    while (port->available() > 0)
    {
        char c = (char)port->read();

        if (c == '\r')
            continue;

        if (c == '\n')
        {
            if (lineBuf.length() > 0)
            {
                String input = lineBuf;
                lineBuf = "";
                input.trim();
                if (input.isEmpty())
                    return false;

                out.raw_json = input; // Store raw JSON for display
                parseJsonLine(input.c_str(), out);
                return true;
            }
        }
        else
        {
            if (lineBuf.length() < 512)
            {
                lineBuf += c;
            }
            else
            {
                lineBuf = "";
            }
        }
    }
    return false;
}
