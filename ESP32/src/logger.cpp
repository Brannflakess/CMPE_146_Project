// logger.cpp
#include "logger.h"

void Logger::begin(uint32_t baud)
{
    Serial.begin(baud);
    delay(2000); // Wait for USB serial to be ready on ESP32
}

void Logger::info(const char *msg)
{
    Serial.print("[INFO]  ");
    Serial.println(msg);
}

void Logger::warn(const char *msg)
{
    Serial.print("[WARN]  ");
    Serial.println(msg);
}

void Logger::error(const char *msg)
{
    Serial.print("[ERROR] ");
    Serial.println(msg);
}
