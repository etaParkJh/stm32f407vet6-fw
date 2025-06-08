#include "temperature.h"

/* 외부 변수 (CubeMX 생성) */
extern ADC_HandleTypeDef hadc1;

/* 전역 변수 */
static uint16_t adc_buffer[TEMP_SAMPLE_COUNT];
static volatile uint8_t adc_conversion_complete = 0;
static uint32_t last_log_time = 0;
static float current_temperature = 0.0f;

/**
 * @brief ADC DMA 시작
 */
void temp_init(void)
{
    // DMA로 연속 변환 시작
    HAL_StatusTypeDef status = HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buffer, TEMP_SAMPLE_COUNT);

    if (status == HAL_OK) {
        log_printf("ADC DMA started successfully\n");
    } else {
    	log_printf("ADC DMA start failed: %d\n", status);
    }
}

/**
 * @brief ADC DMA 정지
 */
void temp_dma_stop(void)
{
    HAL_ADC_Stop_DMA(&hadc1);
    log_printf("ADC DMA stopped\n");
}

/**
 * @brief ADC 값을 섭씨 온도로 변환
 */
static float convert_adc_to_celsius(uint16_t adc_value)
{
    // ADC 값을 전압으로 변환
    float voltage = ((float)adc_value / TEMP_ADC_MAX) * TEMP_VREF;

    // STM32F407 온도 센서 공식
    float temperature = ((voltage - TEMP_V25) / TEMP_AVG_SLOPE) + 25.0f;

    return temperature;
}

/**
 * @brief 현재 온도 값 가져오기 (평균화)
 */
float temp_get_celsius(void)
{
    if (!adc_conversion_complete) {
        return current_temperature;  // 이전 값 반환
    }

    // 평균 계산
    uint32_t sum = 0;
    for (int i = 0; i < TEMP_SAMPLE_COUNT; i++) {
        sum += adc_buffer[i];
    }

    uint16_t avg_adc = sum / TEMP_SAMPLE_COUNT;
    current_temperature = convert_adc_to_celsius(avg_adc);

    adc_conversion_complete = 0;  // 플래그 리셋

    return current_temperature;
}

/**
 * @brief 온도 로그 처리 (메인 루프에서 호출)
 */
void temp_process(void)
{
    uint32_t current_time = HAL_GetTick();

    // 주기적 로그 출력
    if (current_time - last_log_time >= TEMP_LOG_INTERVAL) {
        float temp = temp_get_celsius();

        log_printf("Temperature: %.2f°C (Raw ADC avg: %d)\n",
                      temp,
                      (adc_buffer[0] + adc_buffer[TEMP_SAMPLE_COUNT-1]) / 2);

        last_log_time = current_time;
    }

    // DMA 로그 처리
    log_process();
}

/**
 * @brief ADC 변환 완료 콜백
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    if (hadc == &hadc1)
    {
        adc_conversion_complete = 1;
//        // 디버깅용 (선택적)
//        static uint32_t conversion_count = 0;
//        conversion_count++;
//
//        if (conversion_count % 100 == 0) {  // 100번마다 로그
////            log_printf("ADC conversions: %lu\n", conversion_count);
//        }
    }
}
