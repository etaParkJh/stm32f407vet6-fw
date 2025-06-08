#ifndef W25Q128_H
#define W25Q128_H

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>
#include "spi.h"

/* 기본 명령어들 */
#define W25Q128_CMD_READ_DATA       0x03
#define W25Q128_CMD_PAGE_PROGRAM    0x02
#define W25Q128_CMD_SECTOR_ERASE    0x20
#define W25Q128_CMD_WRITE_ENABLE    0x06
#define W25Q128_CMD_READ_STATUS     0x05

/* 상태 비트 */
#define W25Q128_STATUS_BUSY         0x01

/* 설정 구조체 */
typedef struct {
    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef *cs_port;
    uint16_t cs_pin;
} W25Q128_Handle_t;

/* 함수 선언 */
void W25Q128_Init(void);
void W25Q128_ReadData(uint32_t addr, uint8_t *data, uint32_t size);
void W25Q128_WriteData(uint32_t addr, uint8_t *data, uint32_t size);
void W25Q128_EraseSector(uint32_t addr);
void Test_W25Q128(void);

#endif
