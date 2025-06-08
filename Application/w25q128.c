/**
 * @file w25q128_simple.c
 * @brief W25Q128 Simple API 구현
 */

#include "w25q128.h"

// w25q128_simple.c 상단
static W25Q128_Handle_t w25q_handle_instance;  // 실제 변수
static W25Q128_Handle_t *w25q_handle = &w25q_handle_instance;  // 포인터


/* CS 핀 제어 */
static void CS_Low(void) {
    HAL_GPIO_WritePin(w25q_handle->cs_port, w25q_handle->cs_pin, GPIO_PIN_RESET);
}

static void CS_High(void) {
    HAL_GPIO_WritePin(w25q_handle->cs_port, w25q_handle->cs_pin, GPIO_PIN_SET);
}

/* 상태 확인 */
static bool IsReady(void) {
    uint8_t cmd = W25Q128_CMD_READ_STATUS;
    uint8_t status;

    CS_Low();
    HAL_SPI_Transmit(w25q_handle->hspi, &cmd, 1, 100);
    HAL_SPI_Receive(w25q_handle->hspi, &status, 1, 100);
    CS_High();

    return !(status & W25Q128_STATUS_BUSY);
}

/* 쓰기 활성화 */
static void WriteEnable(void) {
    uint8_t cmd = W25Q128_CMD_WRITE_ENABLE;

    CS_Low();
    HAL_SPI_Transmit(w25q_handle->hspi, &cmd, 1, 100);
    CS_High();
}

/* 준비될 때까지 대기 */
static void WaitReady(void) {
    while (!IsReady()) {
        HAL_Delay(1);
    }
}

/**
 * @brief W25Q128 초기화
 */
void W25Q128_Init(void)
{
	w25q_handle->hspi = &hspi2;
	w25q_handle->cs_pin = SPI_CS_Pin;
	w25q_handle->cs_port = SPI_CS_GPIO_Port;
    CS_High();  // CS 핀을 HIGH로 설정
    HAL_Delay(10);
}

/**
 * @brief 데이터 읽기
 */
void W25Q128_ReadData(uint32_t addr, uint8_t *data, uint32_t size) {
    uint8_t cmd[4];

    // 명령어 + 주소 준비
    cmd[0] = W25Q128_CMD_READ_DATA;
    cmd[1] = (addr >> 16) & 0xFF;
    cmd[2] = (addr >> 8) & 0xFF;
    cmd[3] = addr & 0xFF;

    CS_Low();
    HAL_SPI_Transmit(w25q_handle->hspi, cmd, 4, 100);
    HAL_SPI_Receive(w25q_handle->hspi, data, size, 1000);
    CS_High();
}

/**
 * @brief 데이터 쓰기 (한 페이지만)
 */
void W25Q128_WriteData(uint32_t addr, uint8_t *data, uint32_t size) {
    uint8_t cmd[4];

    // 최대 256바이트까지만
    if (size > 256) size = 256;

    WriteEnable();

    // 명령어 + 주소 준비
    cmd[0] = W25Q128_CMD_PAGE_PROGRAM;
    cmd[1] = (addr >> 16) & 0xFF;
    cmd[2] = (addr >> 8) & 0xFF;
    cmd[3] = addr & 0xFF;

    CS_Low();
    HAL_SPI_Transmit(w25q_handle->hspi, cmd, 4, 100);
    HAL_SPI_Transmit(w25q_handle->hspi, data, size, 1000);
    CS_High();

    WaitReady();  // 완료 대기
}

/**
 * @brief 섹터 지우기 (4KB)
 */
void W25Q128_EraseSector(uint32_t addr) {
    uint8_t cmd[4];

    WriteEnable();

    // 명령어 + 주소 준비
    cmd[0] = W25Q128_CMD_SECTOR_ERASE;
    cmd[1] = (addr >> 16) & 0xFF;
    cmd[2] = (addr >> 8) & 0xFF;
    cmd[3] = addr & 0xFF;

    CS_Low();
    HAL_SPI_Transmit(w25q_handle->hspi, cmd, 4, 100);
    CS_High();

    WaitReady();  // 완료 대기
}

/**
 * @brief 사용 예제
 */
void Test_W25Q128(void)
{
    // 테스트 데이터
    uint8_t write_data[16] = "Hello W25Q128!";
    uint8_t read_data[16] = {0};

    // 1. 섹터 지우기
    W25Q128_EraseSector(0x000000);

    // 2. 데이터 쓰기
    W25Q128_WriteData(0x000000, write_data, 16);

    // 3. 데이터 읽기
    W25Q128_ReadData(0x000000, read_data, 16);

    // 4. 결과 확인
    // read_data와 write_data가 같은지 확인
#if 1
    printf("write data -> %s", write_data);
    fflush(stdout);
	printf("  read data  -> %s", read_data);
	fflush(stdout);
#else
    printf("write data -> %s\\", write_data);
    printf("read data  -> %s\\", read_data);
#endif
}
