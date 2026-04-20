/*
 * battery_client.c
 */

#include "battery_client.h"
#include "stm32f1xx_hal.h"
#include "rs485.h"
#include "modbus_rtu.h"
#include "FreeRTOS.h"
#include "task.h" // Para usar vTaskDelay

extern UART_HandleTypeDef huart1;
static uint8_t batt_id;

#define BATT_RX_SIZE 80
uint8_t batt_rx_buf[BATT_RX_SIZE];
uint8_t batt_err_code = 0;

void battery_client_init(uint8_t slave_id)
{
    batt_id = slave_id;
}

uint8_t battery_read_basic(modbus_battery_t *batt_union)
{
    uint8_t req[8];
    uint16_t crc;

    req[0] = batt_id;
    req[1] = 0x03;
    req[2] = 0x00; req[3] = 0x00;
    req[4] = 0x00; req[5] = 28;

    crc = modbus_crc(req, 6);
    req[6] = crc & 0xFF;
    req[7] = crc >> 8;

    for(int i = 0; i < BATT_RX_SIZE; i++) batt_rx_buf[i] = 0;

    rs485_tx(GPIOA, GPIO_PIN_11);

    // Limpa a UART
    __HAL_UART_CLEAR_OREFLAG(&huart1);
    __HAL_UART_CLEAR_FEFLAG(&huart1);
    __HAL_UART_CLEAR_NEFLAG(&huart1);
    volatile uint32_t dummy = huart1.Instance->DR;
    (void)dummy;

    // ARMA O DMA
    HAL_UART_Receive_DMA(&huart1, batt_rx_buf, BATT_RX_SIZE);

    // FALA
    HAL_UART_Transmit(&huart1, req, sizeof(req), 50);

    // ESCUTA
    rs485_rx(GPIOA, GPIO_PIN_11);

    // DORME POR 100ms.
    // O FreeRTOS vai cuidar da vida dele enquanto o DMA pega a resposta da bateria.
    vTaskDelay(pdMS_TO_TICKS(100));

    // Acorda e desliga a gravação
    HAL_UART_DMAStop(&huart1);

    // PROCURA A MENSAGEM (Ignora qualquer ruído inicial)
    int start_idx = -1;
    for (int i = 0; i <= (BATT_RX_SIZE - 61); i++) {
        if (batt_rx_buf[i] == batt_id && batt_rx_buf[i+1] == 0x03 && batt_rx_buf[i+2] == 56) {
            start_idx = i;
            break;
        }
    }

    if (start_idx == -1) {
        batt_err_code = 3; // Bateria não respondeu ou timeout
        return 0;
    }

    // CALCULA O CRC (Agora garantidamente os 61 bytes estarao la)
    uint16_t calc = modbus_crc(&batt_rx_buf[start_idx], 59);
    uint16_t recv = (uint16_t)batt_rx_buf[start_idx + 59] | ((uint16_t)batt_rx_buf[start_idx + 60] << 8);
    if (calc != recv) {
        batt_err_code = 4; // Erro real de cabo
        return 0;
    }

    batt_err_code = 99; // SUCESSO ABSOLUTO!

    // COPIA PARA A UNION
    int payload_idx = start_idx + 3;
    for (int i = 0; i < 28; i++) {
        batt_union->regs[i] = (batt_rx_buf[payload_idx] << 8) | batt_rx_buf[payload_idx + 1];
        payload_idx += 2;
    }

    return 1;
}
