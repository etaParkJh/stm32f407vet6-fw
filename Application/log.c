#include "log.h"


/* 전역 변수 */
extern UART_HandleTypeDef huart3;  // CubeMX에서 생성됨

static uint8_t log_buffer[DMA_LOG_BUFFER_SIZE];
static volatile uint32_t write_pos = 0;
static volatile uint32_t read_pos = 0;
static volatile uint8_t dma_busy = 0;
static uint8_t tx_buffer[DMA_LOG_MAX_MESSAGE];

/**
 * @brief 버퍼에 저장된 데이터 개수
 */
static uint32_t GetDataCount(void)
{
    if (write_pos >= read_pos) {
        return write_pos - read_pos;
    } else {
        return DMA_LOG_BUFFER_SIZE - read_pos + write_pos;
    }
}

/**
 * @brief 버퍼에 데이터 쓰기
 */
static uint32_t WriteToBuffer(const uint8_t *data, uint32_t len)
{
    uint32_t written = 0;

    __disable_irq();

    for (uint32_t i = 0; i < len; i++) {
        uint32_t next_pos = (write_pos + 1) % DMA_LOG_BUFFER_SIZE;

        // 버퍼 가득 참 체크
        if (next_pos == read_pos) {
            break;  // 더 이상 쓸 수 없음
        }

        log_buffer[write_pos] = data[i];
        write_pos = next_pos;
        written++;
    }

    __enable_irq();

    return written;
}

/**
 * @brief 버퍼에서 데이터 읽기
 */
static uint32_t ReadFromBuffer(uint8_t *data, uint32_t max_len)
{
    uint32_t read = 0;

    __disable_irq();

    while (read_pos != write_pos && read < max_len) {
        data[read] = log_buffer[read_pos];
        read_pos = (read_pos + 1) % DMA_LOG_BUFFER_SIZE;
        read++;
    }

    __enable_irq();

    return read;
}

/**
 * @brief DMA 로그 시스템 초기화
 */
void DMA_Log_Init(void)
{
    write_pos = 0;
    read_pos = 0;
    dma_busy = 0;

    printf("DMA Log System Initialized\n");
}

/**
 * @brief DMA 로그 출력
 */
void DMA_Log_Printf(const char* format, ...)
{
    char temp[DMA_LOG_MAX_MESSAGE];
    va_list args;

    va_start(args, format);
    int len = vsnprintf(temp, DMA_LOG_MAX_MESSAGE, format, args);
    va_end(args);

    if (len > 0) {
        WriteToBuffer((uint8_t*)temp, len);
    }
}

/**
 * @brief DMA 전송 시작
 */
static void StartDMATransmission(void)
{
    if (dma_busy) {
        return;  // 이미 전송 중
    }

    uint32_t data_count = GetDataCount();
    if (data_count == 0) {
        return;  // 전송할 데이터 없음
    }

    // 전송할 크기 결정
    uint32_t tx_size = (data_count > DMA_LOG_MAX_MESSAGE) ?
                       DMA_LOG_MAX_MESSAGE : data_count;

    // 버퍼에서 데이터 읽기
    uint32_t read_size = ReadFromBuffer(tx_buffer, tx_size);

    if (read_size > 0) {
        dma_busy = 1;
        HAL_StatusTypeDef status = HAL_UART_Transmit_DMA(&huart3, tx_buffer, read_size);

        if (status != HAL_OK) {
            dma_busy = 0;  // 실패 시 리셋
            printf("DMA TX Failed: %d\n", status);
        }
    }
}

/**
 * @brief DMA 로그 처리 (메인 루프에서 호출)
 */
void DMA_Log_Process(void)
{
    if (!dma_busy) {
        StartDMATransmission();
    }
}

/**
 * @brief DMA 전송 완료 콜백
 */
void DMA_Log_TxComplete(void)
{
    dma_busy = 0;

    // 더 전송할 데이터가 있으면 바로 시작
    StartDMATransmission();
}

/**
 * @brief 상태 확인 (디버깅용)
 */
void DMA_Log_Status(void)
{
    printf("Buffer: %lu/%d bytes, DMA: %s\n",
           GetDataCount(), DMA_LOG_BUFFER_SIZE,
           dma_busy ? "BUSY" : "IDLE");
}

/**
 * @brief 강제 플러시 (모든 데이터 전송 완료까지 대기)
 */
void DMA_Log_Flush(void)
{
    uint32_t timeout = HAL_GetTick() + 5000;  // 5초 타임아웃

    while (GetDataCount() > 0 && HAL_GetTick() < timeout) {
        DMA_Log_Process();
        HAL_Delay(10);
    }

    if (GetDataCount() > 0) {
        printf("WARNING: %lu bytes not transmitted\n", GetDataCount());
    }
}

/**
 * @brief 간단한 테스트 함수
 */
void DMA_Log_Simple_Test(void)
{
    printf("\n=== Simple DMA Log Test ===\n");

    // 1. 간단한 메시지
    DMA_Log_Printf("Hello DMA Log!\n");
    DMA_Log_Status();

    // 2. 처리
    DMA_Log_Process();
    HAL_Delay(100);
    DMA_Log_Status();

    // 3. 여러 메시지
    for (int i = 0; i < 5; i++) {
        DMA_Log_Printf("Test message %d\n", i);
    }
    DMA_Log_Status();

    // 4. 모든 데이터 전송 완료까지 대기
    DMA_Log_Flush();
    DMA_Log_Status();

    printf("Simple test complete\n");
}

/**
 * @brief 성능 테스트
 */
void DMA_Log_Performance_Test(void)
{
    printf("\n=== Performance Test ===\n");

    // printf 테스트
    uint32_t start = HAL_GetTick();
    for (int i = 0; i < 100; i++) {
        printf("Printf message %d\n", i);
    }
    uint32_t printf_time = HAL_GetTick() - start;

    // DMA 로그 테스트
    start = HAL_GetTick();
    for (int i = 0; i < 100; i++) {
        DMA_Log_Printf("DMA message %d\n", i);
    }
    uint32_t dma_store_time = HAL_GetTick() - start;

    // 전송 완료까지 대기
    start = HAL_GetTick();
    DMA_Log_Flush();
    uint32_t dma_total_time = dma_store_time + (HAL_GetTick() - start);
    printf("Results (100 messages):\n");
    printf("- printf: %lu ms\n", printf_time);
    printf("- DMA store: %lu ms\n", dma_store_time);
    printf("- DMA total: %lu ms\n", dma_total_time);
    printf("- Store speedup: %.1fx\n", (float)printf_time / dma_store_time);
    fflush(stdout);
}
/**
 * @brief 안전한 성능 테스트 (같은 UART 사용)
 */
void Safe_Performance_Test(void)
{
    printf("=== Single UART Performance Test ===\n");
    printf("Starting test... Please wait\n");
    fflush(stdout);
    HAL_Delay(100);

    /* === 1. printf 성능 측정 === */
    printf("Testing printf performance...\n");
    fflush(stdout);

    uint32_t printf_start = HAL_GetTick();

    // printf 테스트 (결과는 즉시 출력 안 함)
    for (int i = 0; i < 100; i++) {
        printf("Printf test %d\n", i);
    }
    fflush(stdout);  // 모든 printf 완료까지 대기

    uint32_t printf_end = HAL_GetTick();
    uint32_t printf_time = printf_end - printf_start;

    HAL_Delay(200);  // printf 완전 종료 대기

    /* === 2. DMA 로그 성능 측정 === */
    printf("Testing DMA log performance...\n");
    fflush(stdout);
    HAL_Delay(100);

    // DMA 저장 시간 측정
    uint32_t dma_store_start = HAL_GetTick();

    for (int i = 0; i < 100; i++) {
        DMA_Log_Printf("DMA test %d\n", i);
    }

    uint32_t dma_store_end = HAL_GetTick();
    uint32_t dma_store_time = dma_store_end - dma_store_start;

    // DMA 전송 시간 측정
    uint32_t dma_tx_start = HAL_GetTick();
    DMA_Log_Flush();
    uint32_t dma_tx_end = HAL_GetTick();
    uint32_t dma_tx_time = dma_tx_end - dma_tx_start;

    /* === 3. 결과 출력 (모든 테스트 완료 후) === */
    printf("\n=== Performance Results ===\n");
    fflush(stdout);
    printf("Test Messages: 100 each\n");
    fflush(stdout);
    printf("printf time: %lu ms\n", printf_time);
    fflush(stdout);
    printf("DMA store time: %lu ms\n", dma_store_time);
    fflush(stdout);
    printf("DMA transmission time: %lu ms\n", dma_tx_time);
    fflush(stdout);
    printf("DMA total time: %lu ms\n", dma_store_time + dma_tx_time);
    fflush(stdout);
    printf("\nSpeedup Analysis:\n");
    fflush(stdout);
    printf("- Store speedup: %.1fx faster\n", (float)printf_time / dma_store_time);
    fflush(stdout);
    printf("- Real-time benefit: %.1f%% CPU time saved\n",
           (float)dma_store_time * 100.0f / printf_time);
    fflush(stdout);

}

/**
 * @brief 실시간성 테스트 (가장 중요!)
 */
void Realtime_Performance_Test(void)
{
    printf("\n=== Real-time Performance Test ===\n");
    printf("Simulating critical tasks with logging...\n");
    fflush(stdout);
    HAL_Delay(100);

    volatile uint32_t dummy_work = 0;

    /* === printf 방식: 실시간 태스크 + 로깅 === */
    uint32_t printf_start = HAL_GetTick();

    for (int i = 0; i < 50; i++) {
        // 크리티컬 작업 시뮬레이션
        for (int j = 0; j < 1000; j++) {
            dummy_work++;
        }

        // 로깅 (블로킹)
        printf("Critical task %d completed\n", i);

        // 또 다른 크리티컬 작업
        for (int j = 0; j < 1000; j++) {
            dummy_work++;
        }
    }
    fflush(stdout);

    uint32_t printf_realtime = HAL_GetTick() - printf_start;

    HAL_Delay(200);

    /* === DMA 방식: 실시간 태스크 + 로깅 === */
    uint32_t dma_start = HAL_GetTick();

    for (int i = 0; i < 50; i++) {
        // 크리티컬 작업 시뮬레이션
        for (int j = 0; j < 1000; j++) {
            dummy_work++;
        }

        // 로깅 (비동기)
        DMA_Log_Printf("Critical task %d completed\n", i);

        // 또 다른 크리티컬 작업
        for (int j = 0; j < 1000; j++) {
            dummy_work++;
        }

        // 백그라운드 로그 처리
        DMA_Log_Process();
    }

    uint32_t dma_realtime = HAL_GetTick() - dma_start;

    // DMA 전송 완료까지 대기
    DMA_Log_Flush();

    /* === 결과 출력 === */
    printf("\n=== Real-time Results ===\n");
    fflush(stdout);
    printf("Critical tasks: 50 cycles each\n");
    fflush(stdout);
    printf("printf + critical tasks: %lu ms\n", printf_realtime);
    fflush(stdout);
    printf("DMA log + critical tasks: %lu ms\n", dma_realtime);
    fflush(stdout);
    printf("Time saved: %lu ms (%.1f%%)\n",
           printf_realtime - dma_realtime,
           (float)(printf_realtime - dma_realtime) * 100.0f / printf_realtime);
    fflush(stdout);
    printf("Real-time improvement: %.1fx faster\n",
           (float)printf_realtime / dma_realtime);
    fflush(stdout);
}

/**
 * @brief CPU 사용률 테스트
 */
void CPU_Usage_Test(void)
{
    printf("\n=== Pure CPU Usage Test ===\n");
    printf("Only measuring CPU time for log storage\n");
    fflush(stdout);

    volatile uint32_t work_counter = 0;

    /* === 1. 순수 작업만 === */
    uint32_t pure_start = HAL_GetTick();

    for (int i = 0; i < 10000; i++) {
        work_counter++;
    }

    uint32_t pure_time = HAL_GetTick() - pure_start;

    /* === 2. printf 방식 === */
    work_counter = 0;
    uint32_t printf_start = HAL_GetTick();

    for (int i = 0; i < 10000; i++) {
        work_counter++;
        if (i % 100 == 0) {
            printf("Printf progress: %d\n", i);
            fflush(stdout);  // 블로킹 전송
        }
    }

    uint32_t printf_time = HAL_GetTick() - printf_start;

    /* === 3. DMA 방식 (순수 저장만) === */
    work_counter = 0;
    uint32_t dma_start = HAL_GetTick();

    for (int i = 0; i < 10000; i++) {
        work_counter++;
        if (i % 100 == 0) {
            DMA_Log_Printf("DMA progress: %d\n", i);
        }
    }

    uint32_t dma_time = HAL_GetTick() - dma_start;

    /* === 4. 별도로 DMA 전송 시간 측정 === */
    uint32_t transmission_start = HAL_GetTick();
    DMA_Log_Flush();  // 모든 전송 완료
    uint32_t transmission_time = HAL_GetTick() - transmission_start;

    /* === 결과 === */
    printf("\n=== Pure CPU Results ===\n");
    fflush(stdout);
    printf("- Pure work: %lu ms\n", pure_time);
    fflush(stdout);
    printf("- Work + printf: %lu ms\n", printf_time);
    fflush(stdout);
    printf("- Work + DMA store: %lu ms\n", dma_time);
    fflush(stdout);
    printf("- DMA transmission: %lu ms (separate)\n", transmission_time);
    fflush(stdout);

    printf("\nCPU Impact:\n");
    fflush(stdout);
    printf("- printf CPU overhead: %lu ms\n", printf_time - pure_time);
    fflush(stdout);
    printf("- DMA CPU overhead: %lu ms\n", dma_time - pure_time);
    fflush(stdout);
    printf("- CPU time saved: %.1f%%\n",
           (float)(printf_time - dma_time) * 100.0f / printf_time);
    fflush(stdout);
}

/**
 * @brief 종합 성능 테스트
 */
void Comprehensive_Performance_Test(void)
{
    printf("\n");
    printf("========================================\n");
    printf("  STM32F407 Comprehensive Performance\n");
    printf("========================================\n");
    printf("System Clock: %lu Hz\n", HAL_RCC_GetSysClockFreq());
    printf("UART Baud Rate: 115200\n");
    printf("========================================\n");
    fflush(stdout);

    // 1. 기본 성능 테스트
    Safe_Performance_Test();
    HAL_Delay(500);

    // 2. 실시간성 테스트 (가장 중요!)
    Realtime_Performance_Test();
    HAL_Delay(500);

    // 3. CPU 사용률 테스트
    CPU_Usage_Test();
    HAL_Delay(500);

    printf("\n========================================\n");
    printf("  All Performance Tests Completed!\n");
    printf("========================================\n");
    printf("Key Takeaway: DMA logging provides\n");
    printf("non-blocking operation for real-time systems\n");
    printf("========================================\n");
    fflush(stdout);
}

/**
 * @brief 간단한 검증 테스트
 */
void Quick_Validation_Test(void)
{
    printf("=== Quick Validation Test ===\n");

    // 시간 측정만, 출력은 나중에
    uint32_t start, end;

    start = HAL_GetTick();
    for (int i = 0; i < 10; i++) {
        printf("printf %d\n", i);
    }
    fflush(stdout);
    end = HAL_GetTick();
    uint32_t printf_time = end - start;

    HAL_Delay(100);

    start = HAL_GetTick();
    for (int i = 0; i < 10; i++) {
        DMA_Log_Printf("DMA %d\n", i);
    }
    end = HAL_GetTick();
    uint32_t dma_time = end - start;

    DMA_Log_Flush();

    printf("Quick Results:\n");
    fflush(stdout);
    printf("printf (10 msgs): %lu ms\n", printf_time);
    fflush(stdout);
    printf("DMA log (10 msgs): %lu ms\n", dma_time);
    fflush(stdout);
    printf("Speedup: %.1fx\n", (float)printf_time / dma_time);
    fflush(stdout);
}
