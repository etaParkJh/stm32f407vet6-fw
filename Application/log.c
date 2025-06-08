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
static uint32_t get_data_count(void)
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
static uint32_t write_to_buffer(const uint8_t *data, uint32_t len)
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
static uint32_t read_from_buffer(uint8_t *data, uint32_t max_len)
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
void log_init(void)
{
    write_pos = 0;
    read_pos = 0;
    dma_busy = 0;

    memset(log_buffer, 0, sizeof(log_buffer));
}

/**
 * @brief DMA 로그 출력
 */
void log_printf(const char* format, ...)
{
    char temp[DMA_LOG_MAX_MESSAGE];
    va_list args;

    va_start(args, format);
    int len = vsnprintf(temp, DMA_LOG_MAX_MESSAGE, format, args);
    va_end(args);

    if (len > 0) {
        write_to_buffer((uint8_t*)temp, len);
    }
}

/**
 * @brief DMA 전송 시작
 */
static void start_dma_transmission(void)
{
    if (dma_busy) {
        return;  // 이미 전송 중
    }

    uint32_t data_count = get_data_count();
    if (data_count == 0) {
        return;  // 전송할 데이터 없음
    }

    // 전송할 크기 결정
    uint32_t tx_size = (data_count > DMA_LOG_MAX_MESSAGE) ?
                       DMA_LOG_MAX_MESSAGE : data_count;

    // 버퍼에서 데이터 읽기
    uint32_t read_size = read_from_buffer(tx_buffer, tx_size);

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
void log_process(void)
{
    if (!dma_busy) {
        start_dma_transmission();
    }
}

/**
 * @brief DMA 전송 완료 콜백
 */
//void DMA_Log_TxComplete(void)
void log_tx_complete(void)
{
    dma_busy = 0;

    // 더 전송할 데이터가 있으면 바로 시작
    start_dma_transmission();
}

/**
 * @brief 상태 확인 (디버깅용)
 */
//void DMA_Log_Status(void)
void log_status(void)
{
    printf("Buffer: %lu/%d bytes, DMA: %s\n",
           get_data_count(), DMA_LOG_BUFFER_SIZE,
           dma_busy ? "BUSY" : "IDLE");
}

/**
 * @brief 강제 플러시 (모든 데이터 전송 완료까지 대기)
 */
//void DMA_Log_Flush(void)
void log_flush(void)
{
    uint32_t timeout = HAL_GetTick() + 5000;  // 5초 타임아웃

    while (get_data_count() > 0 && HAL_GetTick() < timeout) {
        log_process();
        HAL_Delay(10);
    }

    if (get_data_count() > 0) {
        printf("WARNING: %lu bytes not transmitted\n", get_data_count());
    }
}