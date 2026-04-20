/*
 * modbus_rtuc.c
 *
 *  Created on: Dec 17, 2025
 *      Author: Lukinhas
 *
 */

#include "modbus_rtu.h"
#include "stm32f1xx_hal.h"
#include "rs485.h"

extern UART_HandleTypeDef huart3;   // <<< UPLINK SunSpec (USART3)

uint16_t modbus_regs[MODBUS_MAX_REGS];
static uint8_t slave;

uint8_t rx_buffer[256];
volatile uint16_t rx_len = 0;
volatile uint8_t frame_ready = 0;

static void send_exception(uint8_t func, uint8_t excode)
{
    uint8_t resp[5];
    resp[0] = slave;
    resp[1] = func | 0x80;
    resp[2] = excode;
    uint16_t crc = modbus_crc(resp, 3);
    resp[3] = crc & 0xFF;
    resp[4] = crc >> 8;

    rs485_tx(GPIOB, GPIO_PIN_1); // DE/RE USART3 PREPARA PARA FALAR
    HAL_UART_Transmit(&huart3, resp, sizeof(resp), 100);
    rs485_rx(GPIOB, GPIO_PIN_1);// VOLTA A ESCUTAR
}

void modbus_init(uint8_t slave_id)
{
    slave = slave_id;
    // Inicia a escuta em background. Só vai disparar a interrupção
    // quando o mestre parar de enviar dados (linha IDLE).
    HAL_UARTEx_ReceiveToIdle_IT(&huart3, rx_buffer, sizeof(rx_buffer));
}

void modbus_rtu_rx_complete(uint16_t Size)
{
    // Apenas guarde o tamanho e levante a flag.
    rx_len = Size;
    frame_ready = 1;
}

void modbus_poll(void)
{
    // Se não chegou pacote novo, sai imediatamente
    if (!frame_ready) return;

    frame_ready = 0; // Abaixa a flag, vamos processar

    // Verifica tamanho mínimo e se a mensagem é para este escravo
    if (rx_len < 8 || rx_buffer[0] != slave) {
        goto restart_rx;
    }

    if (rx_buffer[1] != 0x03) {
        goto restart_rx;
    }

    uint16_t crc = modbus_crc(rx_buffer, 6);
    if ((crc & 0xFF) != rx_buffer[6] || (crc >> 8) != rx_buffer[7]) {
        goto restart_rx;
    }

    uint16_t addr = (rx_buffer[2] << 8) | rx_buffer[3];
    uint16_t qty  = (rx_buffer[4] << 8) | rx_buffer[5];

    if (qty == 0 || qty > 125) { send_exception(0x03, 0x03); goto restart_rx; }
    if ((uint32_t)addr + qty > MODBUS_MAX_REGS) { send_exception(0x03, 0x02); goto restart_rx; }

    uint8_t resp[256];
    resp[0] = slave;
    resp[1] = 0x03;
    resp[2] = (uint8_t)(qty * 2);

    for (uint16_t i = 0; i < qty; i++) {
        uint16_t v = modbus_regs[addr + i];
        resp[3 + i*2] = (uint8_t)(v >> 8);
        resp[4 + i*2] = (uint8_t)(v & 0xFF);
    }

    uint16_t rcrc = modbus_crc(resp, 3 + qty*2);
    resp[3 + qty*2] = rcrc & 0xFF;
    resp[4 + qty*2] = rcrc >> 8;

    rs485_tx(GPIOB, GPIO_PIN_1); // DE/RE USART3 PREPARA PARA FALAR
    HAL_UART_Transmit(&huart3, resp, 5 + qty*2, 500);
    rs485_rx(GPIOB, GPIO_PIN_1); // VOLTA A ESCUTAR

restart_rx:
    // Habilita a interrupção novamente para ficar pronto para o próximo pacote
    HAL_UARTEx_ReceiveToIdle_IT(&huart3, rx_buffer, sizeof(rx_buffer));
}
