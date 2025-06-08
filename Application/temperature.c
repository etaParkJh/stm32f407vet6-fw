#include "temperature.h"

/* 외부 변수 (CubeMX 생성) */
extern ADC_HandleTypeDef hadc1;

/* 전역 변수 */
static uint16_t adc_buffer[TEMP_SAMPLE_COUNT];
static volatile uint8_t adc_conversion_complete = 0;
static uint32_t last_log_time = 0;
static float current_temperature = 0.0f;

/**
 * @brief 온도 DMA 시스템 초기화
 */
void Temp_DMA_Init(void)
{

	 // STM32F407은 자동 캘리브레이션 없음
	// 대신 안정화 시간만 주면 됨
	HAL_Delay(10);  // ADC 안정화 대기

	DMA_Log_Printf("Temperature DMA System Initialized\n");
	DMA_Log_Printf("STM32F407 - No calibration needed\n");

    DMA_Log_Printf("Temperature DMA System Initialized\n");
    DMA_Log_Printf("ADC Calibration completed\n");
}

/**
 * @brief ADC DMA 시작
 */
void Temp_DMA_Start(void)
{
    // DMA로 연속 변환 시작
    HAL_StatusTypeDef status = HAL_ADC_Start_DMA(&hadc1,
                                                 (uint32_t*)adc_buffer,
                                                 TEMP_SAMPLE_COUNT);

    if (status == HAL_OK) {
        DMA_Log_Printf("ADC DMA started successfully\n");
    } else {
        DMA_Log_Printf("ADC DMA start failed: %d\n", status);
    }
}

/**
 * @brief ADC DMA 정지
 */
void Temp_DMA_Stop(void)
{
    HAL_ADC_Stop_DMA(&hadc1);
    DMA_Log_Printf("ADC DMA stopped\n");
}

/**
 * @brief ADC 값을 섭씨 온도로 변환
 */
static float Convert_ADC_to_Celsius(uint16_t adc_value)
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
float Temp_Get_Celsius(void)
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
    current_temperature = Convert_ADC_to_Celsius(avg_adc);

    adc_conversion_complete = 0;  // 플래그 리셋

    return current_temperature;
}

/**
 * @brief 온도 로그 처리 (메인 루프에서 호출)
 */
void Temp_Log_Process(void)
{
    uint32_t current_time = HAL_GetTick();

    // 주기적 로그 출력
    if (current_time - last_log_time >= TEMP_LOG_INTERVAL) {
        float temp = Temp_Get_Celsius();

        DMA_Log_Printf("Temperature: %.2f°C (Raw ADC avg: %d)\n",
                      temp,
                      (adc_buffer[0] + adc_buffer[TEMP_SAMPLE_COUNT-1]) / 2);

        last_log_time = current_time;
    }

    // DMA 로그 처리
    DMA_Log_Process();
}

/**
 * @brief ADC 변환 완료 콜백
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    if (hadc == &hadc1) {
        adc_conversion_complete = 1;

        // 디버깅용 (선택적)
        static uint32_t conversion_count = 0;
        conversion_count++;

        if (conversion_count % 100 == 0) {  // 100번마다 로그
//            DMA_Log_Printf("ADC conversions: %lu\n", conversion_count);
        }
    }
}

/**
 * @brief 온도 모니터링 데모
 */
void Temperature_Monitoring_Demo(void)
{
    DMA_Log_Printf("\n=== Temperature Monitoring Demo ===\n");
    DMA_Log_Printf("System Clock: %lu Hz\n", HAL_RCC_GetSysClockFreq());
    DMA_Log_Printf("Starting continuous temperature monitoring...\n");

    // 초기화
    Temp_DMA_Init();

    // ADC DMA 시작
    Temp_DMA_Start();

    // 실시간 모니터링
    uint32_t demo_start = HAL_GetTick();
    uint32_t stats_interval = 10000;  // 10초마다 통계
    uint32_t last_stats = demo_start;

    float min_temp = 999.0f;
    float max_temp = -999.0f;
    uint32_t measurement_count = 0;
    float temp_sum = 0.0f;

    while (HAL_GetTick() - demo_start < 60000) {  // 60초 데모
        // 온도 로그 처리
        Temp_Log_Process();

        // 통계 수집
        if (adc_conversion_complete) {
            float current_temp = Temp_Get_Celsius();

            if (current_temp < min_temp) min_temp = current_temp;
            if (current_temp > max_temp) max_temp = current_temp;
            temp_sum += current_temp;
            measurement_count++;
        }

        // 주기적 통계 출력
        if (HAL_GetTick() - last_stats >= stats_interval) {
            if (measurement_count > 0) {
                float avg_temp = temp_sum / measurement_count;

                DMA_Log_Printf("\n--- 10s Statistics ---\n");
                DMA_Log_Printf("Measurements: %lu\n", measurement_count);
                DMA_Log_Printf("Average: %.2f°C\n", avg_temp);
                DMA_Log_Printf("Min: %.2f°C, Max: %.2f°C\n", min_temp, max_temp);
                DMA_Log_Printf("Range: %.2f°C\n", max_temp - min_temp);
                DMA_Log_Printf("---------------------\n");

                // 리셋
                min_temp = 999.0f;
                max_temp = -999.0f;
                temp_sum = 0.0f;
                measurement_count = 0;
            }

            last_stats = HAL_GetTick();
        }

        HAL_Delay(10);  // 10ms 간격
    }

    // 데모 종료
    Temp_DMA_Stop();
    DMA_Log_Printf("\nTemperature monitoring demo completed\n");

    // 남은 로그 전송
    DMA_Log_Flush();
}

/**
 * @brief 온도 경보 시스템
 */
void Temperature_Alert_System(void)
{
    DMA_Log_Printf("\n=== Temperature Alert System ===\n");

    const float TEMP_WARNING_HIGH = 50.0f;   // 경고 온도
    const float TEMP_CRITICAL_HIGH = 60.0f;  // 위험 온도
    const float TEMP_WARNING_LOW = 10.0f;    // 저온 경고

    Temp_DMA_Init();
    Temp_DMA_Start();

    uint32_t alert_start = HAL_GetTick();
    uint8_t last_alert_level = 0;  // 0: 정상, 1: 경고, 2: 위험

    while (HAL_GetTick() - alert_start < 30000) {  // 30초
        Temp_Log_Process();

        if (adc_conversion_complete) {
            float temp = Temp_Get_Celsius();
            uint8_t current_alert_level = 0;

            // 경보 레벨 판정
            if (temp >= TEMP_CRITICAL_HIGH) {
                current_alert_level = 2;  // 위험
            } else if (temp >= TEMP_WARNING_HIGH || temp <= TEMP_WARNING_LOW) {
                current_alert_level = 1;  // 경고
            } else {
                current_alert_level = 0;  // 정상
            }

            // 상태 변화 시 로그
            if (current_alert_level != last_alert_level) {
                switch (current_alert_level) {
                    case 0:
                        DMA_Log_Printf("[  ] NORMAL: Temperature %.2f°C\n", temp);
                        break;
                    case 1:
                        DMA_Log_Printf("[  ] WARNING: Temperature %.2f°C\n", temp);
                        break;
                    case 2:
                        DMA_Log_Printf("[  ] CRITICAL: Temperature %.2f°C\n", temp);
                        break;
                }
                last_alert_level = current_alert_level;
            }
        }

        HAL_Delay(100);
    }

    Temp_DMA_Stop();
    DMA_Log_Printf("Alert system demo completed\n");
    DMA_Log_Flush();
}

/*
===============================================
main.c에 추가할 코드
===============================================

#include "adc_dma_temp_log.h"

// ADC 변환 완료 콜백 (main.c에 추가)
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    if (hadc == &hadc1) {
        // 온도 센서 처리 (이미 adc_dma_temp_log.c에 구현됨)
    }
}

// main() 함수 예제
int main(void)
{
    // HAL 초기화 (CubeMX 생성 코드)
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_DMA_Init();        // DMA 먼저!
    MX_ADC1_Init();       // ADC 초기화
    MX_USART3_UART_Init(); // UART 초기화

    // DMA 로그 시스템 초기화
    DMA_Log_Init();

    // 온도 모니터링 데모
    Temperature_Monitoring_Demo();

    // 경보 시스템 데모
    Temperature_Alert_System();

    // 메인 루프
    while (1)
    {
        // 온도 로그 처리
        Temp_Log_Process();

        HAL_Delay(50);
    }
}

===============================================
*/
