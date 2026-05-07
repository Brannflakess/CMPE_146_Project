// telemetry.h
#pragma once
#include <Arduino.h>

struct TelemetryPacket
{
    String src = "";
    uint32_t t_ms = 0;
    String mode = "";
    String state = "";

    int ax = 0;
    int ay = 0;
    int az = 0;
    int raw = 0;
    int filtered = 0;

    uint32_t squat_count = 0;
    uint32_t pushup_count = 0;

    String oled_mode = "";
    uint32_t oled_squats = 0;
    uint32_t oled_pushups = 0;

    String raw_json = "";

    bool hasError = false;
    String errorMsg;
};

struct Telemetry
{
    static void print(const TelemetryPacket &pkt)
    {
        if (pkt.hasError)
        {
            Serial.print("STM32 error: ");
            Serial.println(pkt.errorMsg);
            return;
        }

        Serial.println("\n===== STM32 TELEMETRY =====");
        Serial.print("Raw JSON: ");
        Serial.println(pkt.raw_json);

        Serial.print("Source: ");
        Serial.println(pkt.src);
        Serial.print("t_ms: ");
        Serial.println((unsigned long)pkt.t_ms);

        Serial.print("Mode: ");
        Serial.println(pkt.mode);
        Serial.print("State: ");
        Serial.println(pkt.state);

        Serial.print("Accel X: ");
        Serial.println(pkt.ax);
        Serial.print("Accel Y: ");
        Serial.println(pkt.ay);
        Serial.print("Accel Z: ");
        Serial.println(pkt.az);

        Serial.print("Raw axis: ");
        Serial.println(pkt.raw);
        Serial.print("Filtered: ");
        Serial.println(pkt.filtered);

        Serial.print("Squat count: ");
        Serial.println((unsigned long)pkt.squat_count);
        Serial.print("Pushup count: ");
        Serial.println((unsigned long)pkt.pushup_count);

        Serial.print("OLED mode: ");
        Serial.println(pkt.oled_mode);
        Serial.print("OLED squats: ");
        Serial.println((unsigned long)pkt.oled_squats);
        Serial.print("OLED pushups: ");
        Serial.println((unsigned long)pkt.oled_pushups);

        Serial.println("===========================");
        Serial.println();
    }
};
