# ESP32 UART JSON Receiver

This directory contains an ESP32 serial receiver example that reads JSON text sent from the STM32.

## What it does

- listens on `UART2` at `115200 8N1`
- receives line-delimited JSON from the STM32
- prints the raw JSON line to the ESP32 USB serial monitor
- parses the JSON fields and prints a cleaned English summary

## Files

- `SerialReceiver.ino` — Arduino-compatible ESP32 sketch

## Wiring

Connect the STM32 UART TX pin to the ESP32 RX pin, and share ground:

- STM32 `PA2` (USART2 TX) -> ESP32 `GPIO16` (UART2 RX)
- STM32 `PA3` (USART2 RX) -> ESP32 `GPIO17` (UART2 TX) if you need bidirectional comms
- STM32 `GND` -> ESP32 `GND`

> For receiving only, the critical connection is STM32 TX -> ESP32 RX and common ground.

## How to use

1. Open the ESP32 sketch in Arduino IDE or PlatformIO.
2. Upload `SerialReceiver.ino` to the ESP32.
3. Open the ESP32 serial monitor at `115200`.
4. Run the STM32 firmware and verify the JSON lines appear in the ESP32 monitor.

## Notes

- This sketch uses manual JSON field extraction so it does not require extra libraries.
- If your STM32 output is not readable, double-check the serial port, baud rate, and wiring.
