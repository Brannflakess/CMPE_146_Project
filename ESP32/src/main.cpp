#include <Arduino.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include "config.h"
#include "logger.h"
#include "uart_receiver.h"
#include "wifi_manager.h"
#include "telemetry.h"

static UartReceiver uart;
static WifiManager wifi;
static Preferences s_prefs;

/** Last good UART packet — used for idle/session-change archive (not raw JSON string). */
static TelemetryPacket g_uart_last_pkt;
static uint32_t g_uart_last_stm32_session_id = 0;
static bool g_uart_have_last_session = false;

static uint32_t g_cloud_session_id = 0;

static constexpr uint32_t UART_IDLE_ARCHIVE_MS = 4000;
static uint32_t g_last_uart_rx_ms = 0;
static bool g_uart_idle_archive_done = false;
static bool g_skip_session_change_archive_once = false;

/** Per cloud session — rep counts must not decrease (guards against partial UART JSON). */
static uint32_t g_cloud_max_squats = 0;
static uint32_t g_cloud_max_pushups = 0;

static void resetCloudSessionRepMax(void)
{
    g_cloud_max_squats = 0;
    g_cloud_max_pushups = 0;
}

static void applyMonotonicRepCounts(TelemetryPacket &pkt)
{
    if (pkt.squat_count > g_cloud_max_squats)
    {
        g_cloud_max_squats = pkt.squat_count;
    }
    else
    {
        pkt.squat_count = g_cloud_max_squats;
        pkt.oled_squats = g_cloud_max_squats;
    }

    if (pkt.pushup_count > g_cloud_max_pushups)
    {
        g_cloud_max_pushups = pkt.pushup_count;
    }
    else
    {
        pkt.pushup_count = g_cloud_max_pushups;
        pkt.oled_pushups = g_cloud_max_pushups;
    }
}

static uint32_t takeNextPersistentCloudSessionId(void)
{
    uint32_t n = s_prefs.getULong("next", 1);
    (void)s_prefs.putULong("next", (unsigned long)n + 1U);
    return n;
}

static bool httpPutJson(const char *url, const String &jsonBody)
{
    if (url == nullptr || url[0] == '\0')
    {
        return false;
    }

    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    if (DATABASE_API_KEY[0] != '\0')
    {
        http.addHeader("X-API-Key", DATABASE_API_KEY);
    }

    int statusCode = http.sendRequest("PUT", jsonBody);
    if (statusCode <= 0)
    {
        Serial.printf("[CLOUD] PUT failed: %s\n", http.errorToString(statusCode).c_str());
        http.end();
        return false;
    }

    String response = http.getString();
    Serial.printf("[CLOUD] PUT HTTP %d\n", statusCode);
    if (response.length() > 0)
    {
        Serial.print("[CLOUD] Response: ");
        Serial.println(response);
    }

    http.end();
    return statusCode >= 200 && statusCode < 300;
}

static bool archiveSessionSnapshot(uint32_t cloudArchiveKey, const TelemetryPacket &pkt)
{
    if (cloudArchiveKey == 0U)
    {
        return true;
    }

    const String body = Telemetry::toCloudJson(pkt, cloudArchiveKey, true, millis());
    String url =
        String(DATABASE_ROOT_URL) + "/telemetry/archive/" + String((unsigned long)cloudArchiveKey) + ".json";
    return httpPutJson(url.c_str(), body);
}

static bool uploadCurrentTelemetry(const TelemetryPacket &pkt, uint32_t cloudSessionId)
{
    const String body = Telemetry::toCloudJson(pkt, cloudSessionId, false, 0);
    String url = String(DATABASE_ROOT_URL) + "/telemetry/current.json";
    return httpPutJson(url.c_str(), body);
}

static void rememberUartPacket(const TelemetryPacket &pkt)
{
    g_uart_last_pkt = pkt;
    g_uart_last_pkt.raw_json = pkt.raw_json;
    g_uart_last_stm32_session_id = pkt.session_id;
    g_uart_have_last_session = (pkt.session_id != 0U);
}

void setup()
{
    Logger::begin(PC_BAUD);

    Serial.println("\n\n");
    Logger::info("========================================");
    Logger::info("CMPE_146 ESP32 Starting...");
    Logger::info("========================================");

    if (!s_prefs.begin("motion", false))
    {
        Logger::warn("Preferences begin failed — cloud session ids may not persist across ESP32 reboots");
    }

    uart.begin(UART_BAUD, UART_RX_PIN, UART_TX_PIN);
    Serial.printf("\n[UART] Initialized on RX=%d TX=%d @ %lu baud\n",
                  UART_RX_PIN, UART_TX_PIN, (unsigned long)UART_BAUD);

    wifi.begin(WIFI_SSID, WIFI_PASS);

    Logger::info("Setup complete. Waiting for data from STM32...");
    Serial.println();
}

void loop()
{
    wifi.loop();

    TelemetryPacket pkt;
    if (uart.tryReadPacket(pkt))
    {
        Telemetry::print(pkt);

        if (pkt.hasError)
        {
            Logger::warn("Skipping cloud upload because packet parse failed");
        }
        else
        {
            g_last_uart_rx_ms = millis();
            g_uart_idle_archive_done = false;

            if (pkt.session_id != 0U && g_cloud_session_id == 0U)
            {
                g_cloud_session_id = takeNextPersistentCloudSessionId();
                resetCloudSessionRepMax();
                Serial.printf("[CLOUD] Assigned cloud session_id=%lu\n", (unsigned long)g_cloud_session_id);
            }

            if (g_skip_session_change_archive_once)
            {
                g_skip_session_change_archive_once = false;
            }
            else if (pkt.session_id != 0U && g_uart_have_last_session &&
                     pkt.session_id != g_uart_last_stm32_session_id)
            {
                if (wifi.connected())
                {
                    const uint32_t archiveKey = g_cloud_session_id;
                    if (!archiveSessionSnapshot(archiveKey, g_uart_last_pkt))
                    {
                        Logger::warn("Archive of previous session failed; continuing with live PUT");
                    }
                    else
                    {
                        Serial.printf("[CLOUD] Archived cloud session %lu (STM32 session id changed)\n",
                                      (unsigned long)archiveKey);
                    }
                    g_cloud_session_id = takeNextPersistentCloudSessionId();
                    resetCloudSessionRepMax();
                    Serial.printf("[CLOUD] New cloud session_id=%lu\n", (unsigned long)g_cloud_session_id);
                }
                else
                {
                    Logger::warn("Session changed but WiFi offline — could not archive previous session");
                }
            }

            applyMonotonicRepCounts(pkt);
            rememberUartPacket(pkt);

            if (wifi.connected() && g_cloud_session_id != 0U)
            {
                if (!uploadCurrentTelemetry(pkt, g_cloud_session_id))
                {
                    Logger::warn("Cloud upload failed");
                }
                else
                {
                    Serial.println("[CLOUD] Updated telemetry/current");
                }
            }
            else if (!wifi.connected())
            {
                Logger::warn("WiFi offline - telemetry not uploaded");
            }
        }
    }

    if (g_uart_have_last_session && (g_last_uart_rx_ms != 0U) && wifi.connected() && (g_cloud_session_id != 0U) &&
        !g_uart_idle_archive_done && ((millis() - g_last_uart_rx_ms) >= UART_IDLE_ARCHIVE_MS))
    {
        const uint32_t archiveKey = g_cloud_session_id;
        if (!archiveSessionSnapshot(archiveKey, g_uart_last_pkt))
        {
            Logger::warn("Idle archive failed (UART quiet)");
        }
        else
        {
            Serial.printf("[CLOUD] Idle-archived cloud session %lu (no UART for %lu ms)\n",
                          (unsigned long)archiveKey, (unsigned long)UART_IDLE_ARCHIVE_MS);
        }
        g_cloud_session_id = takeNextPersistentCloudSessionId();
        resetCloudSessionRepMax();
        Serial.printf("[CLOUD] New cloud session_id=%lu (after idle)\n", (unsigned long)g_cloud_session_id);
        g_uart_idle_archive_done = true;
        g_skip_session_change_archive_once = true;
    }

    delay(10);
}
