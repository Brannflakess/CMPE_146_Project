#include "telemetry.h"
#include <ArduinoJson.h>

String Telemetry::toCloudJson(const TelemetryPacket &pkt, uint32_t cloudSessionId, bool archived,
                              uint32_t archivedAtEspMs)
{
    JsonDocument doc;
    doc["src"] = pkt.src.length() > 0 ? pkt.src : "stm32";
    doc["stm32_session_id"] = pkt.session_id;
    doc["session_id"] = cloudSessionId;
    doc["t_ms"] = pkt.t_ms;
    doc["mode"] = pkt.mode;
    doc["state"] = pkt.state;
    doc["ax"] = pkt.ax;
    doc["ay"] = pkt.ay;
    doc["az"] = pkt.az;
    doc["raw"] = pkt.raw;
    doc["filtered"] = pkt.filtered;
    doc["squat_count"] = pkt.squat_count;
    doc["pushup_count"] = pkt.pushup_count;
    doc["oled_mode"] = pkt.oled_mode.length() > 0 ? pkt.oled_mode : pkt.mode;
    doc["oled_squats"] = pkt.oled_squats;
    doc["oled_pushups"] = pkt.oled_pushups;

    if (archived)
    {
        doc["archived"] = true;
        doc["archived_at_esp_ms"] = archivedAtEspMs;
    }

    String out;
    serializeJson(doc, out);
    return out;
}
