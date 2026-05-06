#include <Arduino.h>
#include "config.h"
#include "logger.h"
#include "uart_receiver.h"
#include "wifi_manager.h"
#include "telemetry.h"

static UartReceiver uart;
static WifiManager wifi;
static uint32_t lastSendMs = 0;

void setup()
{
    Logger::begin(PC_BAUD);
    delay(500);

    Logger::info("========================================");
    Logger::info("CMPE_146 ESP32 Starting...");
    Logger::info("========================================");

    uart.begin(UART_BAUD, UART_RX_PIN, UART_TX_PIN);
    Serial.printf("[UART] Initialized on RX=%d TX=%d @ %lu baud\n",
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

        // Optionally send to cloud (future feature)
        uint32_t now = millis();
        if (now - lastSendMs >= SEND_INTERVAL_MS)
        {
            lastSendMs = now;

            if (wifi.connected())
            {
                Logger::info("WiFi connected - ready to send to cloud");
                // TODO: sendTelemetryToCloud(pkt);
            }
            else
            {
                Logger::warn("WiFi offline - buffering telemetry locally");
            }
        }
    }

    delay(10);
}
