#include <Arduino.h>
#include <HTTPClient.h>
#include "config.h"
#include "logger.h"
#include "uart_receiver.h"
#include "wifi_manager.h"
#include "telemetry.h"

static UartReceiver uart;
static WifiManager wifi;

static bool sendTelemetryToCloud(const TelemetryPacket &pkt)
{
    if (DATABASE_UPLOAD_URL[0] == '\0')
    {
        Logger::warn("DATABASE_UPLOAD_URL is empty - skipping cloud upload");
        return false;
    }

    HTTPClient http;
    http.begin(DATABASE_UPLOAD_URL);
    http.addHeader("Content-Type", "application/json");

    if (DATABASE_API_KEY[0] != '\0')
    {
        http.addHeader("X-API-Key", DATABASE_API_KEY);
    }

    // Use PUT instead of POST to UPDATE/OVERWRITE the file
    int statusCode = http.sendRequest("PUT", pkt.raw_json);
    if (statusCode <= 0)
    {
        Serial.printf("[CLOUD] PUT failed: %s\n", http.errorToString(statusCode).c_str());
        http.end();
        return false;
    }

    String response = http.getString();
    Serial.printf("[CLOUD] Updated telemetry, HTTP %d\n", statusCode);
    if (response.length() > 0)
    {
        Serial.print("[CLOUD] Response: ");
        Serial.println(response);
    }

    http.end();
    return statusCode >= 200 && statusCode < 300;
}

void setup()
{
    Logger::begin(PC_BAUD);

    Serial.println("\n\n");
    Logger::info("========================================");
    Logger::info("CMPE_146 ESP32 Starting...");
    Logger::info("========================================");

    uart.begin(UART_BAUD, UART_RX_PIN, UART_TX_PIN);
    Serial.printf("\n[UART] Initialized on RX=%d TX=%d @ %lu baud\n",
                  UART_RX_PIN, UART_TX_PIN, (unsigned long)UART_BAUD);

    wifi.begin(WIFI_SSID, WIFI_PASS);

    Logger::info("Setup complete. Waiting for data from STM32...");
    Serial.println();
}

void loop()
{
    // Handle WiFi connectivity
    wifi.loop();

    // Try to read telemetry packet from STM32
    TelemetryPacket pkt;
    if (uart.tryReadPacket(pkt))
    {
        // Display received telemetry
        Telemetry::print(pkt);

        if (pkt.hasError)
        {
            Logger::warn("Skipping cloud upload because packet parse failed");
        }
        else if (wifi.connected())
        {
            if (!sendTelemetryToCloud(pkt))
            {
                Logger::warn("Cloud upload failed");
            }
        }
        else
        {
            Logger::warn("WiFi offline - telemetry not uploaded");
        }
    }

    delay(10);
}
