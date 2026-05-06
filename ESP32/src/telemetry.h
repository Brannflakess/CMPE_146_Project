// telemetry.h
#pragma once
#include <Arduino.h>

struct TelemetryPacket
{
    char mode[32] = "";
    char state[32] = "";
    long ax = 0, ay = 0, az = 0;
    long raw = 0, filtered = 0;
    long squat_count = 0, pushup_count = 0;
    String raw_json = "";
};

struct Telemetry
{
    static void print(const TelemetryPacket &pkt)
    {
        Serial.println();
        Serial.println("===== TELEMETRY FROM STM32 =====");
        Serial.print("Raw JSON     : ");
        Serial.println(pkt.raw_json);
        Serial.print("Mode         : ");
        Serial.println(pkt.mode);
        Serial.print("State        : ");
        Serial.println(pkt.state);
        Serial.print("AX           : ");
        Serial.println(pkt.ax);
        Serial.print("AY           : ");
        Serial.println(pkt.ay);
        Serial.print("AZ           : ");
        Serial.println(pkt.az);
        Serial.print("Raw axis     : ");
        Serial.println(pkt.raw);
        Serial.print("Filtered     : ");
        Serial.println(pkt.filtered);
        Serial.print("Squat count  : ");
        Serial.println(pkt.squat_count);
        Serial.print("Pushup count : ");
        Serial.println(pkt.pushup_count);
        Serial.println("=================================");
        Serial.println();
    }
};
