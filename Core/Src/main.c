#include "main.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart2;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART2_UART_Init(void);

/* Redirect printf to UART ---------------------------------------------------*/
int __io_putchar(int ch)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

/* MPU-6050 definitions ------------------------------------------------------*/
#define MPU6050_ADDR            (0x68 << 1)
#define MPU6050_REG_WHO_AM_I    0x75
#define MPU6050_REG_PWR_MGMT_1  0x6B
#define MPU6050_REG_ACCEL_XOUT_H 0x3B

/* Squat counter settings ----------------------------------------------------*/
/*
 * IMPORTANT:
 * These threshold values are placeholders.
 * You will almost certainly need to tune them after testing on your chest.
 *
 * For now, this code assumes:
 * - AZ is the main squat axis
 * - the chosen axis DECREASES when going down
 * - the chosen axis INCREASES when coming back up
 */
#define FILTER_SIZE             5
#define SAMPLE_DELAY_MS         100
#define REP_LOCKOUT_MS          1200

#define STAND_THRESHOLD         15000
#define DOWN_THRESHOLD          13800
#define BOTTOM_THRESHOLD        12800
#define UP_THRESHOLD            13800

typedef enum
{
    STATE_STANDING = 0,
    STATE_GOING_DOWN,
    STATE_BOTTOM,
    STATE_GOING_UP
} SquatState;

/* Global squat state --------------------------------------------------------*/
static SquatState squat_state = STATE_STANDING;
static uint32_t squat_count = 0;
static uint32_t last_rep_time = 0;

/* Function prototypes -------------------------------------------------------*/
HAL_StatusTypeDef MPU6050_Init(void);
HAL_StatusTypeDef MPU6050_ReadAccelRaw(int16_t *ax, int16_t *ay, int16_t *az);
int MovingAverage_Update(int new_value);
void Squat_Update(int value, uint32_t now_ms);
const char* SquatState_ToString(SquatState state);

/* Moving average buffer -----------------------------------------------------*/
static int filter_buffer[FILTER_SIZE] = {0};
static int filter_index = 0;
static int filter_sum = 0;
static uint8_t filter_filled = 0;

HAL_StatusTypeDef MPU6050_Init(void)
{
    uint8_t who_am_i = 0;
    uint8_t wake_data = 0;
    HAL_StatusTypeDef status;

    printf("Starting MPU6050 squat counter...\r\n");

    status = HAL_I2C_Mem_Read(&hi2c1,
                              MPU6050_ADDR,
                              MPU6050_REG_WHO_AM_I,
                              I2C_MEMADD_SIZE_8BIT,
                              &who_am_i,
                              1,
                              100);

    if (status != HAL_OK)
    {
        printf("WHO_AM_I read failed\r\n");
        return status;
    }

    printf("WHO_AM_I = 0x%02X\r\n", who_am_i);

    if (who_am_i != 0x68)
    {
        printf("Unexpected WHO_AM_I value\r\n");
    }

    wake_data = 0;
    status = HAL_I2C_Mem_Write(&hi2c1,
                               MPU6050_ADDR,
                               MPU6050_REG_PWR_MGMT_1,
                               I2C_MEMADD_SIZE_8BIT,
                               &wake_data,
                               1,
                               100);

    if (status != HAL_OK)
    {
        printf("MPU6050 wake-up failed\r\n");
        return status;
    }

    printf("MPU6050 wake-up successful\r\n");
    HAL_Delay(100);

    return HAL_OK;
}

HAL_StatusTypeDef MPU6050_ReadAccelRaw(int16_t *ax, int16_t *ay, int16_t *az)
{
    uint8_t accel_data[6];
    HAL_StatusTypeDef status;

    status = HAL_I2C_Mem_Read(&hi2c1,
                              MPU6050_ADDR,
                              MPU6050_REG_ACCEL_XOUT_H,
                              I2C_MEMADD_SIZE_8BIT,
                              accel_data,
                              6,
                              100);

    if (status != HAL_OK)
    {
        return status;
    }

    *ax = (int16_t)((accel_data[0] << 8) | accel_data[1]);
    *ay = (int16_t)((accel_data[2] << 8) | accel_data[3]);
    *az = (int16_t)((accel_data[4] << 8) | accel_data[5]);

    return HAL_OK;
}

int MovingAverage_Update(int new_value)
{
    filter_sum -= filter_buffer[filter_index];
    filter_buffer[filter_index] = new_value;
    filter_sum += new_value;

    filter_index++;
    if (filter_index >= FILTER_SIZE)
    {
        filter_index = 0;
        filter_filled = 1;
    }

    if (filter_filled)
    {
        return filter_sum / FILTER_SIZE;
    }
    else
    {
        int count = filter_index;
        if (count == 0) count = 1;
        return filter_sum / count;
    }
}

void Squat_Update(int value, uint32_t now_ms)
{
    switch (squat_state)
    {
        case STATE_STANDING:
            if (value < DOWN_THRESHOLD)
            {
                squat_state = STATE_GOING_DOWN;
                printf("STATE -> GOING_DOWN\r\n");
            }
            break;

        case STATE_GOING_DOWN:
            if (value < BOTTOM_THRESHOLD)
            {
                squat_state = STATE_BOTTOM;
                printf("STATE -> BOTTOM\r\n");
            }
            else if (value > STAND_THRESHOLD)
            {
                /* Went back up before completing a squat */
                squat_state = STATE_STANDING;
                printf("STATE -> STANDING (reset)\r\n");
            }
            break;

        case STATE_BOTTOM:
            if (value > UP_THRESHOLD)
            {
                squat_state = STATE_GOING_UP;
                printf("STATE -> GOING_UP\r\n");
            }
            break;

        case STATE_GOING_UP:
            if (value > STAND_THRESHOLD)
            {
                if ((now_ms - last_rep_time) > REP_LOCKOUT_MS)
                {
                    squat_count++;
                    last_rep_time = now_ms;
                    printf("SQUAT COUNT = %lu\r\n", (unsigned long)squat_count);
                }

                squat_state = STATE_STANDING;
                printf("STATE -> STANDING\r\n");
            }
            break;

        default:
            squat_state = STATE_STANDING;
            break;
    }
}

const char* SquatState_ToString(SquatState state)
{
    switch (state)
    {
        case STATE_STANDING:   return "STANDING";
        case STATE_GOING_DOWN: return "GOING_DOWN";
        case STATE_BOTTOM:     return "BOTTOM";
        case STATE_GOING_UP:   return "GOING_UP";
        default:               return "UNKNOWN";
    }
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_USART2_UART_Init();

    if (MPU6050_Init() != HAL_OK)
    {
        printf("MPU init failed. Stopping.\r\n");
        while (1)
        {
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
            HAL_Delay(200);
        }
    }

    printf("Starting squat detection loop...\r\n");
    printf("Default axis = AZ\r\n");
    printf("Tune thresholds after first real test.\r\n\r\n");

    while (1)
    {
        int16_t ax, ay, az;
        HAL_StatusTypeDef status;

        status = MPU6050_ReadAccelRaw(&ax, &ay, &az);

        if (status == HAL_OK)
        {
            int ax_g_x100 = (ax * 100) / 16384;
            int ay_g_x100 = (ay * 100) / 16384;
            int az_g_x100 = (az * 100) / 16384;

            /* Pick one axis for squat detection.
               Start with AZ. Change this later if needed. */
            int raw_axis = ay;
            int filtered_axis = MovingAverage_Update(raw_axis);

            Squat_Update(filtered_axis, HAL_GetTick());

            printf("AX=%d AY=%d AZ=%d | ", ax, ay, az);

            printf("AX=%s%d.%02d g ",
                   (ax_g_x100 < 0 ? "-" : ""),
                   abs(ax_g_x100) / 100,
                   abs(ax_g_x100) % 100);

            printf("AY=%s%d.%02d g ",
                   (ay_g_x100 < 0 ? "-" : ""),
                   abs(ay_g_x100) / 100,
                   abs(ay_g_x100) % 100);

            printf("AZ=%s%d.%02d g | ",
                   (az_g_x100 < 0 ? "-" : ""),
                   abs(az_g_x100) / 100,
                   abs(az_g_x100) % 100);

            printf("RAW_AXIS=%d FILTERED=%d STATE=%s COUNT=%lu\r\n",
                   raw_axis,
                   filtered_axis,
                   SquatState_ToString(squat_state),
                   (unsigned long)squat_count);
        }
        else
        {
            printf("Accel read failed\r\n");
        }

        HAL_Delay(SAMPLE_DELAY_MS);
    }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 100000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    if (HAL_I2C_Init(&hi2c1) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart2) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
    }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif
