/**
 * @file temperature.h
 * @brief ADC DMA 온도 측정 + DMA 로그 시스템
 */

#ifndef TEMPERATURE_H
#define TEMPERATURE_H

#include "stm32f4xx_hal.h"
#include "log.h"
#include <stdint.h>
#include "adc.h"

/* 설정 */
#define TEMP_SAMPLE_COUNT    10     // 평균화용 샘플 개수
#define TEMP_LOG_INTERVAL    1000   // 로그 출력 간격 (ms)

/* 온도 보정 상수 (STM32F407 기준) */
#define TEMP_V25            0.76f   // 25°C에서의 전압 (V)
#define TEMP_AVG_SLOPE      0.0025f // 평균 기울기 (V/°C)
#define TEMP_VREF           3.3f    // 기준 전압 (V)
#define TEMP_ADC_MAX        4095.0f // 12비트 ADC 최대값

/* 함수 선언 */
void temp_init(void);
void temp_dma_stop(void);
float temp_get_celsius(void);
void temp_process(void);

#endif /* TEMPERATURE_H */
