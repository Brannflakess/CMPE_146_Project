// logger.h
#pragma once
#include <Arduino.h>

struct Logger
{
    static void begin(uint32_t baud);
    static void info(const char *msg);
    static void warn(const char *msg);
    static void error(const char *msg);
};
