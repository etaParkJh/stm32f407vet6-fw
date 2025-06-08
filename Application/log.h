#ifndef LOG_H_
#define LOG_H_

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* 설정 */
#define DMA_LOG_BUFFER_SIZE   512    // 작게 시작
#define DMA_LOG_MAX_MESSAGE   128    // 한 번에 보낼 최대 크기

/* 함수 선언 */
void DMA_Log_Init(void);
void DMA_Log_Printf(const char* format, ...);
void DMA_Log_Process(void);
void DMA_Log_Simple_Test(void);
void DMA_Log_Performance_Test(void);
void Safe_Performance_Test(void);
void Realtime_Performance_Test(void);
void CPU_Usage_Test(void);
void Comprehensive_Performance_Test(void);
void Quick_Validation_Test(void);
void DMA_Log_TxComplete(void);
void DMA_Log_Flush(void);
#endif /* LOG_H_ */
