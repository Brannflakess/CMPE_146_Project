#pragma once
#include <Arduino.h>

// Serial (USB to computer)
static constexpr uint32_t PC_BAUD = 115200;

// UART (ESP32 <-> STM32)
static constexpr uint32_t UART_BAUD = 115200;
static constexpr int UART_RX_PIN = 16;
static constexpr int UART_TX_PIN = 17;

// Send interval (ms)
static constexpr uint32_t SEND_INTERVAL_MS = 1000;

// WiFi
static constexpr const char *WIFI_SSID = "YourSSID";
static constexpr const char *WIFI_PASS = "YourPassword";

// Local credentials (git-ignored)
// Copy config_local.h.example to config_local.h and fill in your actual credentials
// #include "config_local.h"
static constexpr uint32_t WIFI_RETRY_INTERVAL_MS = 5000;
static constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 20000;
