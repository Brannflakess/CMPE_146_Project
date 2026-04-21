#ifndef OLED_SSD1306_H
#define OLED_SSD1306_H

#include "main.h"
#include <string.h>

/* SSD1306 OLED Configuration */
#define SSD1306_I2C_ADDR 0x78 /* 8-bit address (0x3C in 7-bit) */
#define SSD1306_WIDTH 128
#define SSD1306_HEIGHT 64
#define SSD1306_BUFFER_SIZE (SSD1306_WIDTH * SSD1306_HEIGHT / 8)

/* SSD1306 Commands */
#define SSD1306_CMD_SETCONTRAST 0x81
#define SSD1306_CMD_DISPNORMAL 0xA6
#define SSD1306_CMD_DISPINVERT 0xA7
#define SSD1306_CMD_DISPOFF 0xAE
#define SSD1306_CMD_DISPON 0xAF
#define SSD1306_CMD_SETADDRESS 0x00
#define SSD1306_CMD_SETPAGEADDR 0xB0
#define SSD1306_CMD_SETCOLADDR 0x00
#define SSD1306_CMD_MEMORYMODE 0x20
#define SSD1306_CMD_COLUMDIR 0xA1
#define SSD1306_CMD_SCANDIR 0xC8
#define SSD1306_CMD_SETMULTIPLEX 0xA8
#define SSD1306_CMD_SETDISPCLOCK 0xD5
#define SSD1306_CMD_SETPRECHARGE 0xD9
#define SSD1306_CMD_SETVCOMH 0xDB
#define SSD1306_CMD_CHARGEPUMP 0x8D

/* OLED Structure */
typedef struct
{
    uint8_t buffer[SSD1306_BUFFER_SIZE];
    uint8_t width;
    uint8_t height;
    I2C_HandleTypeDef *i2c;
} OLED_t;

/* Function prototypes */
void OLED_Init(I2C_HandleTypeDef *hi2c);
void OLED_Display(void);
void OLED_Clear(void);
void OLED_SetPixel(uint8_t x, uint8_t y, uint8_t state);
void OLED_WriteString(uint8_t x, uint8_t y, const char *str);
void OLED_WriteLine(uint8_t line, const char *str);
void OLED_DisplayOn(void);
void OLED_DisplayOff(void);
void OLED_SetContrast(uint8_t contrast);

#endif /* OLED_SSD1306_H */
