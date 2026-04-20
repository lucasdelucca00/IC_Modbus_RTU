/*
 * modbus_rtu.h
 *
 *  Created on: Dec 17, 2025
 *      Author: Lukinhas
 */

#pragma once
#include <stdint.h>

#define MODBUS_MAX_REGS 256
#define MODBUS_OK 0
#define MODBUS_ERR 1

void modbus_rtu_rx_complete(uint16_t Size);
void modbus_init(uint8_t slave_id);
void modbus_poll(void);

extern uint16_t modbus_regs[MODBUS_MAX_REGS];

static inline uint16_t modbus_crc(uint8_t *buf, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= buf[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1) crc = (crc >> 1) ^ 0xA001;
            else crc >>= 1;
        }
    }
    return crc;
}
