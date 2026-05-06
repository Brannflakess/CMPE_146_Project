#pragma once
#include <Arduino.h>

// Serial (USB to computer)
static constexpr uint32_t PC_BAUD = 115200;

// UART (ESP32 <-> STM32)
static constexpr uint32_t UART_BAUD = 115200;

// PRIMARY pins (GPIO16/17)
static constexpr int UART_RX_PIN = 16; // Connect STM32 TX here
static constexpr int UART_TX_PIN = 17; // Connect STM32 RX here

// ALTERNATIVE pins if above doesn't work (GPIO13/12):
// static constexpr int UART_RX_PIN = 13;
// static constexpr int UART_TX_PIN = 12;

// Send interval (ms)
static constexpr uint32_t SEND_INTERVAL_MS = 1000;

// WiFi
static constexpr const char *WIFI_SSID = "RaghaviPhone";
static constexpr const char *WIFI_PASS = "12345678";

// Local credentials (git-ignored)
// Copy config_local.h.example to config_local.h and fill in your actual credentials
// #include "config_local.h"
static constexpr uint32_t WIFI_RETRY_INTERVAL_MS = 5000;
static constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 20000;
