#include "main.h"
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "oled_ssd1306.h"
/* Ensure device/CMSIS headers are available via project include paths */
#include "stm32f4xx.h"
#include "stm32f401xe.h"
#include "stm32f4xx_hal_flash_ex.h"

/* Fallback for IntelliSense: define FLASH_LATENCY_0 if HAL headers were not processed */
#ifndef FLASH_LATENCY_0
#define FLASH_LATENCY_0 0
#endif
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
/* MPU-6050 definitions ------------------------------------------------------*/
#define MPU6050_ADDR (0x68 << 1)
#define MPU6050_REG_WHO_AM_I 0x75
#define MPU6050_REG_PWR_MGMT_1 0x6B
#define MPU6050_REG_ACCEL_XOUT_H 0x3B

/* General settings ----------------------------------------------------------*/
#define FILTER_SIZE 5
#define SAMPLE_DELAY_MS 80
#define MODE_LOCK_MS 600

/* -------------------------------------------------------------------------- */
/* EXERCISE MODE DETECTION                                                    */
/* -------------------------------------------------------------------------- */
/*
 * Assumption for chest-mounted vertical board:
 *
 * - Standing upright for squats:
 *     |AY| tends to be large (near 1g), while |AZ| is smaller
 *
 * - Horizontal body position for push-ups:
 *     |AZ| tends to be large (near 1g), while |AY| is smaller
 *
 * These values are placeholders and should be tuned from real logs.
 */
#define MODE_GRAVITY_HIGH 13000
#define MODE_GRAVITY_LOW 9000

typedef enum
{
    EXERCISE_UNKNOWN = 0,
    EXERCISE_SQUAT,
    EXERCISE_PUSHUP
} ExerciseMode;

/* -------------------------------------------------------------------------- */
/* SQUAT SETTINGS                                                             */
/* -------------------------------------------------------------------------- */
#define SQUAT_REP_LOCKOUT_MS 1200
#define SQUAT_STAND_THRESHOLD 15000
#define SQUAT_DOWN_THRESHOLD 13800
#define SQUAT_BOTTOM_THRESHOLD 12800
#define SQUAT_UP_THRESHOLD 13800

/* Range/delta-based thresholds (values are in raw sensor units) */
#define SQUAT_DOWN_DELTA 1400   /* drop from baseline to consider going down */
#define SQUAT_BOTTOM_DELTA 2200 /* deeper drop to consider bottom */
#define SQUAT_MIN_ROM 1200      /* minimum peak-to-peak chest travel to count rep */
#define MIN_STATE_HOLD 2

typedef enum
{
    SQUAT_STATE_STANDING = 0,
    SQUAT_STATE_GOING_DOWN,
    SQUAT_STATE_BOTTOM,
    SQUAT_STATE_GOING_UP
} SquatState;

/* -------------------------------------------------------------------------- */
/* PUSH-UP SETTINGS                                                           */
/* -------------------------------------------------------------------------- */
#define PUSHUP_REP_LOCKOUT_MS 900

#define PUSHUP_TOP_THRESHOLD -16500
#define PUSHUP_DOWN_THRESHOLD -15500
#define PUSHUP_BOTTOM_THRESHOLD -14200
#define PUSHUP_UP_THRESHOLD -15200

typedef enum
{
    PUSHUP_STATE_TOP = 0,
    PUSHUP_STATE_GOING_DOWN,
    PUSHUP_STATE_BOTTOM,
    PUSHUP_STATE_GOING_UP
} PushupState;

/* -------------------------------------------------------------------------- */
/* MOVING AVERAGE FILTER                                                      */
/* -------------------------------------------------------------------------- */
typedef struct
{
    int buffer[FILTER_SIZE];
    int index;
    int sum;
    uint8_t filled;
} MovingAverageFilter;

/* Global state --------------------------------------------------------------*/
static ExerciseMode current_mode = EXERCISE_UNKNOWN;
static ExerciseMode candidate_mode = EXERCISE_UNKNOWN;
static uint32_t candidate_mode_start = 0;

static SquatState squat_state = SQUAT_STATE_STANDING;
static uint32_t squat_count = 0;
static uint32_t squat_last_rep_time = 0;
static float squat_baseline_ay = 0.0f;
static uint8_t squat_baseline_init = 0;
static uint8_t squat_state_hold_count = 0;
static float squat_min_ay = 99999.0f;

static PushupState pushup_state = PUSHUP_STATE_TOP;
static uint32_t pushup_count = 0;
static uint32_t pushup_last_rep_time = 0;

static MovingAverageFilter squat_filter = {0};
static MovingAverageFilter pushup_filter = {0};

static uint32_t last_oled_update = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void OLED_UpdateScreen(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART2_UART_Init(void);

/* USER CODE BEGIN PFP */
HAL_StatusTypeDef MPU6050_Init(void);
HAL_StatusTypeDef MPU6050_ReadAccelRaw(int16_t *ax, int16_t *ay, int16_t *az);

int MovingAverage_Update(MovingAverageFilter *f, int new_value);
void MovingAverage_Reset(MovingAverageFilter *f);

ExerciseMode Detect_ExerciseMode(int16_t ax, int16_t ay, int16_t az);
void Update_Mode_Lock(ExerciseMode detected_mode, uint32_t now_ms);

void Squat_Update(int value, uint32_t now_ms);
void Pushup_Update(int value, uint32_t now_ms);

const char *ExerciseMode_ToString(ExerciseMode mode);
const char *SquatState_ToString(SquatState state);
const char *PushupState_ToString(PushupState state);
/* USER CODE END PFP */

/* USER CODE BEGIN 0 */
/* Redirect printf to UART ---------------------------------------------------*/
int __io_putchar(int ch)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

/* -------------------------------------------------------------------------- */
/* MPU INIT                                                                   */
/* -------------------------------------------------------------------------- */
HAL_StatusTypeDef MPU6050_Init(void)
{
    uint8_t who_am_i = 0;
    uint8_t wake_data = 0;
    HAL_StatusTypeDef status;

    printf("Starting MPU6050 squat + push-up counter...\r\n");

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

/* -------------------------------------------------------------------------- */
/* MPU READ                                                                   */
/* -------------------------------------------------------------------------- */
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

/* -------------------------------------------------------------------------- */
/* MOVING AVERAGE                                                             */
/* -------------------------------------------------------------------------- */
int MovingAverage_Update(MovingAverageFilter *f, int new_value)
{
    f->sum -= f->buffer[f->index];
    f->buffer[f->index] = new_value;
    f->sum += new_value;

    f->index++;
    if (f->index >= FILTER_SIZE)
    {
        f->index = 0;
        f->filled = 1;
    }

    if (f->filled)
    {
        return f->sum / FILTER_SIZE;
    }
    else
    {
        int count = f->index;
        if (count == 0)
            count = 1;
        return f->sum / count;
    }
}

void MovingAverage_Reset(MovingAverageFilter *f)
{
    memset(f->buffer, 0, sizeof(f->buffer));
    f->index = 0;
    f->sum = 0;
    f->filled = 0;
}

/* -------------------------------------------------------------------------- */
/* MODE DETECTION                                                             */
/* -------------------------------------------------------------------------- */
ExerciseMode Detect_ExerciseMode(int16_t ax, int16_t ay, int16_t az)
{
    (void)ax; /* not used yet */

    int abs_ay = abs(ay);
    int abs_az = abs(az);

    if ((abs_ay > MODE_GRAVITY_HIGH) && (abs_az < MODE_GRAVITY_LOW))
    {
        return EXERCISE_SQUAT;
    }
    else if ((abs_az > MODE_GRAVITY_HIGH) && (abs_ay < MODE_GRAVITY_LOW))
    {
        return EXERCISE_PUSHUP;
    }
    else
    {
        return EXERCISE_UNKNOWN;
    }
}

void Update_Mode_Lock(ExerciseMode detected_mode, uint32_t now_ms)
{
    if (detected_mode != candidate_mode)
    {
        candidate_mode = detected_mode;
        candidate_mode_start = now_ms;
        return;
    }

    if ((now_ms - candidate_mode_start) >= MODE_LOCK_MS)
    {
        if (current_mode != candidate_mode)
        {
            current_mode = candidate_mode;

            /* Reset filters and per-exercise state when switching modes */
            MovingAverage_Reset(&squat_filter);
            MovingAverage_Reset(&pushup_filter);

            squat_state = SQUAT_STATE_STANDING;
            pushup_state = PUSHUP_STATE_TOP;

            printf("MODE -> %s\r\n", ExerciseMode_ToString(current_mode));
        }
    }
}

/* -------------------------------------------------------------------------- */
/* SQUAT LOGIC                                                                */
/* -------------------------------------------------------------------------- */
void Squat_Update(int value, uint32_t now_ms)
{
    float fvalue = (float)value;
    float delta = 0.0f;

    /* Initialize baseline while standing */
    if (!squat_baseline_init && (squat_state == SQUAT_STATE_STANDING))
    {
        squat_baseline_ay = fvalue;
        squat_baseline_init = 1;
    }

    if (squat_baseline_init)
    {
        /* when standing, slowly track baseline to adapt to drift */
        if (squat_state == SQUAT_STATE_STANDING)
        {
            squat_baseline_ay = (0.95f * squat_baseline_ay) + (0.05f * fvalue);
        }

        delta = squat_baseline_ay - fvalue; /* positive when chest moves down */
    }

    switch (squat_state)
    {
    case SQUAT_STATE_STANDING:
        if (squat_baseline_init && (delta >= SQUAT_DOWN_DELTA))
        {
            squat_state_hold_count++;
            if (squat_state_hold_count >= MIN_STATE_HOLD)
            {
                squat_state = SQUAT_STATE_GOING_DOWN;
                squat_state_hold_count = 0;
                squat_min_ay = fvalue;
                printf("SQUAT STATE -> GOING_DOWN (delta=%.0f)\r\n", delta);
            }
        }
        else
        {
            squat_state_hold_count = 0;
        }
        break;

    case SQUAT_STATE_GOING_DOWN:
        /* track minimum AY while descending */
        if (fvalue < squat_min_ay)
            squat_min_ay = fvalue;
        if (squat_baseline_init && (delta >= SQUAT_BOTTOM_DELTA))
        {
            squat_state_hold_count++;
            if (squat_state_hold_count >= MIN_STATE_HOLD)
            {
                squat_state = SQUAT_STATE_BOTTOM;
                squat_state_hold_count = 0;
                printf("SQUAT STATE -> BOTTOM (delta=%.0f)\r\n", delta);
            }
        }
        else if (squat_baseline_init && (delta < (SQUAT_DOWN_DELTA / 2.0f)))
        {
            /* aborted descent */
            squat_state = SQUAT_STATE_STANDING;
            squat_state_hold_count = 0;
            squat_min_ay = 99999.0f;
            printf("SQUAT STATE -> STANDING (abort)\r\n");
        }
        break;

    case SQUAT_STATE_BOTTOM:
        if (squat_baseline_init && (delta < (SQUAT_BOTTOM_DELTA / 2.0f)))
        {
            squat_state_hold_count++;
            if (squat_state_hold_count >= MIN_STATE_HOLD)
            {
                squat_state = SQUAT_STATE_GOING_UP;
                squat_state_hold_count = 0;
                printf("SQUAT STATE -> GOING_UP\r\n");
            }
        }
        else
        {
            squat_state_hold_count = 0;
        }
        break;

    case SQUAT_STATE_GOING_UP:
        if (squat_baseline_init && (delta <= (SQUAT_DOWN_DELTA / 2.0f)))
        {
            /* compute ROM and decide whether to count */
            float rom = squat_baseline_ay - squat_min_ay;
            if ((now_ms - squat_last_rep_time) > SQUAT_REP_LOCKOUT_MS)
            {
                if (rom >= SQUAT_MIN_ROM)
                {
                    squat_count++;
                    squat_last_rep_time = now_ms;
                    printf("SQUAT COUNT = %lu (rom=%.0f)\r\n", (unsigned long)squat_count, rom);
                }
                else
                {
                    printf("SQUAT REP REJECTED (rom=%.0f min=%d)\r\n", rom, SQUAT_MIN_ROM);
                }
            }

            squat_state = SQUAT_STATE_STANDING;
            squat_min_ay = 99999.0f;
            squat_state_hold_count = 0;
            printf("SQUAT STATE -> STANDING\r\n");
        }
        break;

    default:
        squat_state = SQUAT_STATE_STANDING;
        squat_state_hold_count = 0;
        break;
    }
}

/* -------------------------------------------------------------------------- */
/* PUSH-UP LOGIC                                                              */
/* -------------------------------------------------------------------------- */
void Pushup_Update(int value, uint32_t now_ms)
{
    switch (pushup_state)
    {
    case PUSHUP_STATE_TOP:
        /* Going down = value becomes LESS negative */
        if (value > PUSHUP_DOWN_THRESHOLD)
        {
            pushup_state = PUSHUP_STATE_GOING_DOWN;
            printf("PUSHUP STATE -> GOING_DOWN\r\n");
        }
        break;

    case PUSHUP_STATE_GOING_DOWN:
        if (value > PUSHUP_BOTTOM_THRESHOLD)
        {
            pushup_state = PUSHUP_STATE_BOTTOM;
            printf("PUSHUP STATE -> BOTTOM\r\n");
        }
        else if (value < PUSHUP_TOP_THRESHOLD)
        {
            /* Went back to top too early */
            pushup_state = PUSHUP_STATE_TOP;
            printf("PUSHUP STATE -> TOP (reset)\r\n");
        }
        break;

    case PUSHUP_STATE_BOTTOM:
        /* Going up = value becomes MORE negative again */
        if (value < PUSHUP_UP_THRESHOLD)
        {
            pushup_state = PUSHUP_STATE_GOING_UP;
            printf("PUSHUP STATE -> GOING_UP\r\n");
        }
        break;

    case PUSHUP_STATE_GOING_UP:
        if (value < PUSHUP_TOP_THRESHOLD)
        {
            if ((now_ms - pushup_last_rep_time) > PUSHUP_REP_LOCKOUT_MS)
            {
                pushup_count++;
                pushup_last_rep_time = now_ms;
                printf("PUSHUP COUNT = %lu\r\n", (unsigned long)pushup_count);
            }

            pushup_state = PUSHUP_STATE_TOP;
            printf("PUSHUP STATE -> TOP\r\n");
        }
        break;

    default:
        pushup_state = PUSHUP_STATE_TOP;
        break;
    }
}

/* -------------------------------------------------------------------------- */
/* STRING HELPERS                                                             */
/* -------------------------------------------------------------------------- */
const char *ExerciseMode_ToString(ExerciseMode mode)
{
    switch (mode)
    {
    case EXERCISE_SQUAT:
        return "SQUAT";
    case EXERCISE_PUSHUP:
        return "PUSHUP";
    default:
        return "UNKNOWN";
    }
}

const char *SquatState_ToString(SquatState state)
{
    switch (state)
    {
    case SQUAT_STATE_STANDING:
        return "STANDING";
    case SQUAT_STATE_GOING_DOWN:
        return "GOING_DOWN";
    case SQUAT_STATE_BOTTOM:
        return "BOTTOM";
    case SQUAT_STATE_GOING_UP:
        return "GOING_UP";
    default:
        return "UNKNOWN";
    }
}

const char *PushupState_ToString(PushupState state)
{
    switch (state)
    {
    case PUSHUP_STATE_TOP:
        return "TOP";
    case PUSHUP_STATE_GOING_DOWN:
        return "GOING_DOWN";
    case PUSHUP_STATE_BOTTOM:
        return "BOTTOM";
    case PUSHUP_STATE_GOING_UP:
        return "GOING_UP";
    default:
        return "UNKNOWN";
    }
}

void OLED_UpdateScreen(void)
{
    char line[24];

    OLED_Clear();

    OLED_WriteLine(0, "Motion Tracker");

    sprintf(line, "Mode: %s", ExerciseMode_ToString(current_mode));
    OLED_WriteLine(2, line);

    sprintf(line, "Squats: %lu", (unsigned long)squat_count);
    OLED_WriteLine(4, line);

    sprintf(line, "Pushups: %lu", (unsigned long)pushup_count);
    OLED_WriteLine(5, line);

    OLED_Display();
}
/* USER CODE END 0 */

/* -------------------------------------------------------------------------- */
/* MAIN                                                                       */
/* -------------------------------------------------------------------------- */
int main(void)
{
    /* USER CODE BEGIN 1 */

    /* USER CODE END 1 */

    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_USART2_UART_Init();

    /* USER CODE BEGIN 2 */
    if (MPU6050_Init() != HAL_OK)
    {
        printf("MPU init failed. Stopping.\r\n");
        while (1)
        {
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
            HAL_Delay(200);
        }
    }

    OLED_Init(&hi2c1);
    OLED_Clear();
    OLED_WriteLine(0, "Starting...");
    OLED_Display();

    printf("Starting combined squat + push-up detection loop...\r\n");
    printf("Chest-mounted IMU mode detection enabled.\r\n");
    printf("Tune thresholds after first real test.\r\n\r\n");
    /* USER CODE END 2 */

    /* USER CODE BEGIN WHILE */
    while (1)
    {
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
        int16_t ax, ay, az;
        HAL_StatusTypeDef status;

        status = MPU6050_ReadAccelRaw(&ax, &ay, &az);

        if (status == HAL_OK)
        {
            int ax_g_x100 = (ax * 100) / 16384;
            int ay_g_x100 = (ay * 100) / 16384;
            int az_g_x100 = (az * 100) / 16384;

            ExerciseMode detected_mode = Detect_ExerciseMode(ax, ay, az);
            Update_Mode_Lock(detected_mode, HAL_GetTick());

            if (current_mode == EXERCISE_SQUAT)
            {
                int raw_axis = ay;
                int filtered_axis = MovingAverage_Update(&squat_filter, raw_axis);

                Squat_Update(filtered_axis, HAL_GetTick());

                printf("[MODE=%s] AX=%d AY=%d AZ=%d | ",
                       ExerciseMode_ToString(current_mode), ax, ay, az);

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

                printf("RAW=%d FILTERED=%d SQUAT_STATE=%s SQUAT_COUNT=%lu PUSHUP_COUNT=%lu\r\n",
                       raw_axis,
                       filtered_axis,
                       SquatState_ToString(squat_state),
                       (unsigned long)squat_count,
                       (unsigned long)pushup_count);
            }
            else if (current_mode == EXERCISE_PUSHUP)
            {
                int raw_axis = az;
                int filtered_axis = MovingAverage_Update(&pushup_filter, raw_axis);

                Pushup_Update(filtered_axis, HAL_GetTick());

                printf("[MODE=%s] AX=%d AY=%d AZ=%d | ",
                       ExerciseMode_ToString(current_mode), ax, ay, az);

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

                printf("RAW=%d FILTERED=%d PUSHUP_STATE=%s SQUAT_COUNT=%lu PUSHUP_COUNT=%lu\r\n",
                       raw_axis,
                       filtered_axis,
                       PushupState_ToString(pushup_state),
                       (unsigned long)squat_count,
                       (unsigned long)pushup_count);
            }
            else
            {
                printf("[MODE=%s] AX=%d AY=%d AZ=%d | ",
                       ExerciseMode_ToString(current_mode), ax, ay, az);

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

                printf("SQUAT_COUNT=%lu PUSHUP_COUNT=%lu\r\n",
                       (unsigned long)squat_count,
                       (unsigned long)pushup_count);
            }
        }
        else
        {
            printf("Accel read failed\r\n");
        }

        if (status == HAL_OK)
        {
            if (HAL_GetTick() - last_oled_update >= 300)
            {
                OLED_UpdateScreen();
                last_oled_update = HAL_GetTick();
            }
        }

        HAL_Delay(SAMPLE_DELAY_MS);
    }
    /* USER CODE END 3 */
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

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
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
    /* USER CODE BEGIN 6 */
    /* USER CODE END 6 */
}
#endif
