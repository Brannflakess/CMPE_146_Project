// uart_receiver.h
#pragma once
#include <Arduino.h>
#include "telemetry.h"

class UartReceiver
{
public:
    void begin(uint32_t baud, int rxPin, int txPin);
    bool tryReadPacket(TelemetryPacket &out);

private:
    HardwareSerial *port = &Serial2;
    String lineBuf;

    bool extractStringValue(const char *json, const char *key, char *out, size_t maxLen);
    bool extractIntValue(const char *json, const char *key, long *out);
    void parseJsonLine(const char *json, TelemetryPacket &out);
};
