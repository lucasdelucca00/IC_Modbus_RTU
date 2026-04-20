/*
 * battery_client.h
 *
 * Created on: Abril 16, 2026
 * Author: Lukinhas
 */

#pragma once
#include <stdint.h>

// Força o compilador a não colocar espaços vazios na memória (padding)
#pragma pack(push, 1)

// Mapeamento exato dos 28 registradores (0 a 27) da sua bateria
typedef struct {
    uint16_t voltage_dV;        // reg 0
    int16_t  current_dA;        // reg 1
    uint16_t cel_voltage[16];   // reg 2 ao 17 (Preenchimento para alinhar a memória)
    int16_t  bms_cooling_temp;  // reg 18
    int16_t  battery_internal;  // reg 19
    int16_t  temp_max;          // reg 20
    uint16_t remaining_cap;     // reg 21
    uint16_t max_recharge;      // reg 22
    uint16_t soh;               // reg 23
    uint16_t soc;               // reg 24
    uint16_t status;            // reg 25
    uint16_t alarm;             // reg 26
    uint16_t protection;        // reg 27
} battery_data_t;

#pragma pack(pop)

// A UNION MÁGICA
typedef union {
    uint16_t regs[28];        // Gavetas brutas (O DMA do Modbus joga os bytes aqui)
    battery_data_t data;      // Gavetas nomeadas (Lê os dados por aqui no SunSpec)
} modbus_battery_t;

// Protótipos das funções
void battery_client_init(uint8_t slave_id);

// A função recebe o ponteiro para a Union inteira
uint8_t battery_read_basic(modbus_battery_t *batt_union);
