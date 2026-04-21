# STM32F401 IMU Squat Counter with OLED Display

## 📋 Project Overview

This project is an **embedded firmware application** that counts squats in real-time using an **MPU-6050 accelerometer** (6-axis IMU sensor) and displays results on an **SSD1306 OLED display (128x64)**. The system continuously monitors acceleration data, detects complete squat movements using a state machine algorithm, and shows real-time feedback on the OLED screen and serial connection.

### What It Does
- Reads 3-axis accelerometer data from the MPU-6050 sensor via I2C
- Filters noisy sensor readings using a moving average algorithm
- Detects squat movements through state transitions
- Counts completed squats with a lockout timer to prevent double-counting
- Displays results on 128x64 pixel OLED display
- Outputs real-time data (raw values, G-forces, squat count, state) via UART/serial

---

## 🔧 Hardware Components

### Microcontroller
- **Part**: STM32F401RE (ARM Cortex-M4)
- **Architecture**: 32-bit
- **Flash Memory**: 512 KB
- **RAM**: 96 KB
- **Clock Speed**: 84 MHz (via HSI)

### Sensors & Displays
- **IMU Sensor**: MPU-6050 (6-axis, I2C communication)
- **OLED Display**: SSD1306 (128x64 pixels, I2C communication)
- **Display Type**: Yellow/Blue OLED with self-luminous pixels

### Other Components
- USB-to-UART adapter (for serial communication and debugging)
- LED indicator (status/error signaling)
- Pull-up resistors (for I2C communication, typically 10kΩ - included on modules)

---

## 📍 Pin Configuration

### **I2C1 Bus (Shared by MPU-6050 and OLED SSD1306)**

| Pin | Port | Function | Direction | Device |
|-----|------|----------|-----------|--------|
| PB8 | Port B | I2C1_SCL (Serial Clock) | Bidirectional | Both MPU-6050 & OLED |
| PB9 | Port B | I2C1_SDA (Serial Data) | Bidirectional | Both MPU-6050 & OLED |

**I2C Settings:**
- Clock Speed: 100 kHz
- Address Mode: 7-bit
- **MPU-6050 I2C Address**: 0x68 (7-bit)
- **OLED SSD1306 Address**: 0x3C (7-bit)

### **USART2 (Serial/Debug Connection)**

| Pin | Port | Function | Direction |
|-----|------|----------|-----------|
| PA2 | Port A | USART2_TX (Transmit) | Output |
| PA3 | Port A | USART2_RX (Receive) | Input |

**UART Settings:**
- Baud Rate: 115200
- Data Bits: 8
- Stop Bits: 1
- Parity: None
- Flow Control: None

### **GPIO (Status LED)**

| Pin | Port | Function | Direction |
|-----|------|----------|-----------|
| PA5 | Port A | Status LED | Output |

**LED Behavior:**
- Toggles every 200ms if MPU-6050 initialization fails (error indicator)
- Used for visual debugging during startup

---

## 🔌 Wiring Diagram (Connections)

```
┌──────────────────────────────────────────────────────────┐
│          STM32F401RE Microcontroller                     │
├──────────────────────────────────────────────────────────┤
│                                                          │
│  I2C1 Bus (Shared):                                      │
│  ┌─────────────────────────────────────────────────────┐ │
│  │ PB8 (I2C1_SCL) ─────────┬──→ MPU-6050 (SCL)        │ │
│  │ PB9 (I2C1_SDA) ─────────┼──→ MPU-6050 (SDA)        │ │
│  │                         ├──→ OLED (SCL)            │ │
│  │                         └──→ OLED (SDA)            │ │
│  │ GND ────────────────────┬──→ MPU-6050 (GND)        │ │
│  │                         └──→ OLED (GND)            │ │
│  │ 3.3V ───────────────────┬──→ MPU-6050 (VCC)        │ │
│  │                         └──→ OLED (VCC)            │ │
│  └─────────────────────────────────────────────────────┘ │
│                                                          │
│  USART2 (Serial Debug):                                  │
│  PA2 (TX) ──────────→ USB-to-UART (RX)                 │
│  PA3 (RX) ←────────── USB-to-UART (TX)                 │
│  GND ───────────────→ USB-to-UART (GND)                │
│                      (115200 baud)                      │
│                                                          │
│  Status LED:                                             │
│  PA5 ──→ LED ──→ GND (330Ω resistor)                   │
│                                                          │
└──────────────────────────────────────────────────────────┘
```

### Quick Connection Summary - I2C Bus

| Device | STM32 Pin | Connection |
|--------|-----------|-----------|
| **MPU-6050** | PB8 (I2C_SCL) | SCL pin |
| **MPU-6050** | PB9 (I2C_SDA) | SDA pin |
| **MPU-6050** | 3.3V | VCC pin |
| **MPU-6050** | GND | GND pin |
| **OLED SSD1306** | PB8 (I2C_SCL) | SCL pin |
| **OLED SSD1306** | PB9 (I2C_SDA) | SDA pin |
| **OLED SSD1306** | 3.3V | VCC pin |
| **OLED SSD1306** | GND | GND pin |
| **USB-UART** | PA2 (TX) | RX pin |
| **USB-UART** | PA3 (RX) | TX pin |
| **USB-UART** | GND | GND pin |

---

## 💻 How It Works

### 1. **Initialization Phase**
- Initializes the microcontroller's clock system (HSI 16MHz internal oscillator)
- Enables GPIO ports A and B
- Configures I2C1 at 100 kHz clock speed for both MPU-6050 and OLED
- Configures USART2 at 115200 baud rate
- Initializes OLED SSD1306 display
- Displays welcome message on OLED
- Wakes up MPU-6050 and verifies communication via WHO_AM_I register (should read 0x68)

### 2. **Data Acquisition Loop**
Runs continuously every 100ms:
- Reads 6 bytes of accelerometer data (X, Y, Z axes) from MPU-6050
- Converts raw 16-bit values to gravitational acceleration (G-force)
- Each raw value represents acceleration in the range ±16g with 16,384 LSB/g

### 3. **Filtering**
- Applies a **5-sample moving average filter** to smooth noise
- Reduces false detections from sensor noise
- Formula: `filtered_value = sum(last 5 samples) / 5`

### 4. **Squat Detection State Machine**
The algorithm tracks 4 states to detect a complete squat:

```
STATE_STANDING
    ↓ (when AY < DOWN_THRESHOLD)
STATE_GOING_DOWN
    ↓ (when AY < BOTTOM_THRESHOLD)
STATE_BOTTOM
    ↓ (when AY > UP_THRESHOLD)
STATE_GOING_UP
    ↓ (when AY > STAND_THRESHOLD + lockout delay)
STATE_STANDING (count += 1)
```

**Parameters (Tunable - Values are Placeholders):**

| Parameter | Value | Description |
|-----------|-------|-------------|
| STAND_THRESHOLD | 15000 | Acceleration threshold for standing position |
| DOWN_THRESHOLD | 13800 | Threshold to trigger squat descent |
| BOTTOM_THRESHOLD | 12800 | Minimum acceleration (bottom of squat) |
| UP_THRESHOLD | 13800 | Threshold to start ascent detection |
| REP_LOCKOUT_MS | 1200 | Minimum time between squat counts (prevents double-counting) |
| SAMPLE_DELAY_MS | 100 | Time between readings in milliseconds |

### 5. **OLED Display Output**
Real-time display on 128x64 pixel screen showing:
- Line 1: "Hello Guys!"
- Line 2: "Squat Counter"
- Additional info like squat count, state, and values

### 6. **Serial Output**
Continuously prints to UART2 in this format:
```
AX=raw_x AY=raw_y AZ=raw_z | AX=x.xx g AY=y.yy g AZ=z.zz g | RAW_AXIS=val FILTERED=val STATE=state COUNT=count
```

Example output:
```
AX=100 AY=16200 AZ=50 | AX=0.61 g AY=98.84 g AZ=0.30 g | RAW_AXIS=16200 FILTERED=16180 STATE=STANDING COUNT=5
```

---

## ⚙️ Configuration & Tuning

### **Important Note**
The threshold values are **placeholders** and need to be tuned after the first real test on your body. The algorithm assumes:
- **Axis Used**: AY (Y-axis acceleration) - currently set on line 281
- **Behavior**: The axis **DECREASES** when squatting down, **INCREASES** when standing up

### **How to Tune**
1. Upload the firmware and wear the sensor on your chest
2. Open a serial monitor (115200 baud) to view output
3. Perform some squats and observe the RAW_AXIS and FILTERED values
4. Note the minimum value when at the bottom of a squat
5. Note the maximum value when standing
6. Adjust thresholds accordingly:
   - Set DOWN_THRESHOLD slightly below your standing value
   - Set BOTTOM_THRESHOLD at the deepest squat point
   - Set UP_THRESHOLD on the way back up
   - Set STAND_THRESHOLD slightly below your standing value

### **Changing the Detection Axis**
To use AX or AZ instead of AY, modify line 281 in `Core/Src/main.c`:
```c
int raw_axis = ay;  // Change to: ax or az
```

---

## 📊 Data Flow Diagram

```
MPU-6050 Sensor
      ↓ (I2C, Raw 16-bit values)
Read Accelerometer (6 bytes)
      ↓
Convert to 3-axis values (ax, ay, az)
      ↓
Apply Moving Average Filter (5 samples)
      ↓
Extract Selected Axis (AY)
      ↓
Squat State Machine
      ├→ Check current state
      ├→ Evaluate thresholds
      ├→ Transition to next state
      └→ Increment count if squat completed
      ↓
Update OLED Display & Print to UART
      ↓
Serial Monitor (115200 baud) + OLED Screen (128x64)
```

---

## 🛠️ Building & Uploading

### Prerequisites
- STM32CubeIDE or equivalent ARM Cortex-M compiler
- ST-Link V2 debugger/programmer
- USB cable for power and programming

### Build Steps
1. Open `CMPE_146_Project.ioc` in STM32CubeIDE
2. Generate code (if modified)
3. Build project
4. Connect ST-Link to the STM32F401
5. Upload firmware via STM32CubeIDE

### Debugging
- Open any serial terminal (PuTTY, Arduino IDE, etc.)
- Connect USB-UART adapter to PA2/PA3
- Set baudrate to **115200**
- View real-time squat detection output
- OLED display shows status and values simultaneously

---

## 📝 Code Structure

| File | Purpose |
|------|---------|
| `Core/Src/main.c` | Main application logic (squat detection, filtering, I2C/UART communication) |
| `Core/Inc/main.h` | Header file with configurations and function prototypes |
| `Core/Src/oled_ssd1306.c` | OLED SSD1306 driver implementation |
| `Core/Inc/oled_ssd1306.h` | OLED driver header with function prototypes |
| `Core/Src/stm32f4xx_hal_msp.c` | Hardware abstraction layer setup (GPIO, I2C, UART initialization) |
| `Core/Src/stm32f4xx_it.c` | Interrupt handlers |
| `Core/Src/system_stm32f4xx.c` | System clock configuration |
| `Drivers/STM32F4xx_HAL_Driver/` | STM32 Hardware Abstraction Layer library |
| `Drivers/CMSIS/` | ARM Cortex-M core support library |

---

## 🔍 Key Functions

### MPU6050_Init()
Initializes the MPU-6050 sensor:
- Reads WHO_AM_I register (should return 0x68)
- Wakes up the sensor by clearing power management register
- Returns HAL_OK on success

### MPU6050_ReadAccelRaw()
Reads 6 bytes of acceleration data from the sensor and converts to 16-bit signed integers for X, Y, Z axes

### MovingAverage_Update()
Implements a circular buffer for 5-sample moving average filtering to reduce noise

### Squat_Update()
Main state machine function that processes the filtered acceleration value and tracks squat progression

### OLED Functions

#### OLED_Init(I2C_HandleTypeDef *hi2c)
Initializes the OLED SSD1306 display via I2C with proper initialization sequence

#### OLED_Clear()
Clears all pixels from the display

#### OLED_Display()
Sends the buffer content to the OLED display to show text/graphics

#### OLED_WriteLine(uint8_t line, const char *str)
Writes a string of text on a specific line (0-7)
- Example: `OLED_WriteLine(0, "Hello Guys!");` writes on first line

#### OLED_WriteString(uint8_t x, uint8_t y, const char *str)
Writes a string at specific pixel coordinates

#### OLED_SetPixel(uint8_t x, uint8_t y, uint8_t state)
Sets or clears individual pixels for graphics

#### OLED_DisplayOn() / OLED_DisplayOff()
Turns the display on or off

#### OLED_SetContrast(uint8_t contrast)
Adjusts display contrast (0-255)

---

## 🎯 Performance Notes

- **Sampling Rate**: 10 Hz (100ms interval) - sufficient for squat detection
- **I2C Speed**: 100 kHz - standard speed for both MPU-6050 and OLED SSD1306
- **Processing Time**: < 1ms per loop (non-blocking)
- **Display Update**: Real-time after each squat state change
- **Accuracy**: Depends on threshold tuning and sensor placement

---

## 📌 Troubleshooting

| Issue | Cause | Solution |
|-------|-------|----------|
| OLED not displaying | I2C communication issue | Check I2C wiring (PB8, PB9), verify pull-up resistors |
| Blank OLED screen | Both devices on same I2C bus | Use I2C address 0x3C for OLED, 0x68 for MPU-6050 |
| MPU-6050 not detected | I2C communication error | Check I2C wiring, verify pull-up resistors |
| No serial output | UART not connected | Check PA2/PA3 connections, set baud to 115200 |
| Inaccurate squat counts | Poor threshold tuning | Perform real test and recalibrate thresholds |
| Noisy readings | Sensor placement or bad wiring | Move away from EMI sources, check connections |

---

## 📚 References

- [STM32F401RE Datasheet](https://www.st.com/resource/en/datasheet/stm32f401re.pdf)
- [MPU-6050 Datasheet](https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Datasheet1.pdf)
- [SSD1306 OLED Driver Datasheet](https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf)
- [STM32CubeF4 HAL Documentation](https://www.st.com/en/embedded-software/stm32cubef4.html)

---

## 📄 License

This project is part of CMPE 146 coursework.

---

## 👤 Author Notes

This is a real-time embedded system project demonstrating:
- ✅ I2C communication with multiple devices on same bus
- ✅ UART for debugging/output
- ✅ Digital signal processing (moving average filtering)
- ✅ State machine design
- ✅ Hardware abstraction with STM32 HAL
- ✅ OLED display integration

**Remember to tune the thresholds after your first real test!**
