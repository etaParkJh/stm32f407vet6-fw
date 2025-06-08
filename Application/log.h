#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "stm32f4xx_hal.h"  // HAL 라이브러리 (MCU에 따라 변경)

/* 매크로 정의 */
#define DMA_LOG_BUFFER_SIZE     2048    // 순환 버퍼 크기
#define DMA_LOG_MAX_MESSAGE     256     // 최대 메시지 크기

/* 함수 선언 */
void log_init(void);
void log_printf(const char* format, ...);
void log_process(void);
void log_tx_complete(void);
void log_status(void);
void log_flush(void);

#endif /* LOG_H */