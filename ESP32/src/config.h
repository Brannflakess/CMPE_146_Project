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

// Firebase Realtime Database root (no trailing slash, no ".json")
// Current telemetry: {DATABASE_ROOT_URL}/telemetry/current.json
// Past sessions (same JSON shape as current): {DATABASE_ROOT_URL}/telemetry/archive/<session_id>.json
static constexpr const char *DATABASE_ROOT_URL = "https://mbet146project-default-rtdb.firebaseio.com";
static constexpr const char *DATABASE_API_KEY = "";

// WiFi
static constexpr const char *WIFI_SSID = "BrandoniPhone";
static constexpr const char *WIFI_PASS = "weeniehutjr";

// Local credentials (git-ignored)
// Copy config_local.h.example to config_local.h and fill in your actual credentials
// #include "config_local.h"
static constexpr uint32_t WIFI_RETRY_INTERVAL_MS = 5000;
static constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 20000;
