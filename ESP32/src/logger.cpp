// logger.cpp
#include "logger.h"

void Logger::begin(uint32_t baud)
{
    Serial.begin(baud);
    delay(200);
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
