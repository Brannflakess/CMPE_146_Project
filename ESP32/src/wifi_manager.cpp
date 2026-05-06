// wifi_manager.cpp
#include "config.h"
#include "wifi_manager.h"
#include "logger.h"

void WifiManager::begin(const char *ssid, const char *pass)
{
    this->ssid = ssid;
    this->pass = pass;
    lastTryMs = millis();

    Logger::info("WiFi Manager initialized");
}

void WifiManager::loop()
{
    uint32_t now = millis();

    // Check if we're already connected
    if (WiFi.status() == WL_CONNECTED)
    {
        if (!isConnected)
        {
            isConnected = true;
            Serial.print("[WIFI] Connected to ");
            Serial.print(ssid);
            Serial.print(" - IP: ");
            Serial.println(WiFi.localIP());
        }
        return;
    }

    // Not connected - try to reconnect every WIFI_RETRY_INTERVAL_MS
    if (now - lastTryMs >= WIFI_RETRY_INTERVAL_MS)
    {
        lastTryMs = now;

        if (isConnected)
        {
            isConnected = false;
            Logger::warn("WiFi disconnected, attempting reconnect...");
        }

        Serial.print("[WIFI] Connecting to ");
        Serial.println(ssid);

        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid, pass);

        // Wait for connection with timeout
        uint32_t startMs = millis();
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && millis() - startMs < WIFI_CONNECT_TIMEOUT_MS)
        {
            delay(500);
            attempts++;
            Serial.print(".");
            if (attempts % 10 == 0)
                Serial.println();
        }

        if (WiFi.status() == WL_CONNECTED)
        {
            isConnected = true;
            Serial.print("\n[WIFI] Connected! IP: ");
            Serial.println(WiFi.localIP());
        }
        else
        {
            Logger::warn("WiFi connection timeout, will retry...");
        }
    }
}

bool WifiManager::connected() const
{
    return WiFi.status() == WL_CONNECTED;
}
