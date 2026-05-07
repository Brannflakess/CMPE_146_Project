// uart_receiver.h
#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include "telemetry.h"

class UartReceiver
{
public:
    void begin(uint32_t baud, int rxPin, int txPin);
    bool tryReadPacket(TelemetryPacket &out);

private:
    HardwareSerial *port = &Serial2;
    String lineBuf;
    void parseJsonLine(const char *json, TelemetryPacket &out);
};
