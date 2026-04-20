/*
 * sunspec.c
 *
 *  Created on: Dec 17, 2025
 *      Author: Lukinhas
 */

#include "sunspec.h"
#include "modbus_rtu.h"
#include "battery_client.h"
#include <string.h>

modbus_battery_t batt;

static void write_str(uint16_t addr, const char *s, uint8_t words)
{
    for (uint8_t i = 0; i < words; i++) {
        uint16_t v = 0;
        char c0 = s[2*i];
        char c1 = s[2*i + 1];
        if (c0) v |= ((uint16_t)(uint8_t)c0) << 8;
        if (c1) v |= ((uint16_t)(uint8_t)c1);
        modbus_regs[addr + i] = v;
    }
}

/* -----------------------------
 * SunSpec layout (0-based):
 * 0..1     : "SunS"
 * 2..69    : Model 1 (L=66)  => data at 4..69
 * 70..135  : Model 802 (L=64)=> data at 72..135
 * 136..137 : End Model
 * ----------------------------- */

void sunspec_init(uint8_t modbus_id)
{
    // SunS signature
    modbus_regs[0] = 0x5375;
    modbus_regs[1] = 0x6E53;

    // ---------------- Model 1 (Common) ----------------
    modbus_regs[2] = 1;     // ID
    modbus_regs[3] = 66;    // L

    write_str(4,  "UNIPOWER", 16);
    write_str(20, "UPLFP48", 16);
    write_str(36, "", 8);
    write_str(44, "", 8);
    write_str(52, "BP_NODE_01", 16);

    // DA (uint16) e Pad (pad16)
    // No common model, DA e Pad ficam no final do bloco (últimos 2 regs do payload)
    // Payload do model 1 começa em 4 e tem 66 regs => termina em 69
    modbus_regs[68] = modbus_id;  // DA
    modbus_regs[69] = 0x0000;     // Pad

    // ---------------- Model 802 (Battery Base) ----------------
    modbus_regs[70] = 802;  // ID
    modbus_regs[71] = 64;   // L

    // Preencher nameplate mínimo (fixo) - pode ajustar depois:
    // AHRtg (Ah) / WHRtg (Wh)
    modbus_regs[72] = 100;   // 100 Ah (exemplo)
    modbus_regs[73] = 5000;  // 5000 Wh (exemplo)

    // WChaRteMax / WDisChaRteMax (W)
    modbus_regs[74] = 3000;
    modbus_regs[75] = 3000;

    // Typ = LITHIUM_ION (enum 4)
    modbus_regs[93] = 4;

    // Scale Factors (mandatórios)
    // SoC_SF, V_SF, A_SF, W_SF
    modbus_regs[126] = -1;  // SoC_SF => valor = x * 10^-1
    modbus_regs[129] = -2;  // V_SF   => valor = x * 10^-2 (bateria já vem em 10mV => 0.01V)
    modbus_regs[131] = -1;  // A_SF   => corrente em 0.1 A (bateria vem em 10mA, mas vamos manter 0.1A por segurança)
    modbus_regs[133] = -1;  // W_SF   => potência em 0.1 W (vamos calcular simples)

    // ---------------- End Model ----------------
    modbus_regs[136] = 0xFFFF;
    modbus_regs[137] = 0x0000;
}

static int16_t clamp_i16(int32_t x)
{
    if (x > 32767) return 32767;
    if (x < -32768) return -32768;
    return (int16_t)x;
}

void sunspec_update(void)
{
    // Passamos o endereço da union inteira para a função preencher
    if (!battery_read_basic(&batt)) {
        modbus_regs[81]  = 0;
        modbus_regs[83]  = 0;
        modbus_regs[104] = 0;
        modbus_regs[114] = 0;
        modbus_regs[117] = 0;
        return;
    }

    // A MÁGICA: Os dados já estão formatados nos tipos e tamanhos certos!
    modbus_regs[81] = batt.data.soc * 10;
    modbus_regs[83] = batt.data.soh * 10;
    modbus_regs[104] = batt.data.voltage_dV;
    modbus_regs[114] = (int16_t)(batt.data.current_dA / 10);

    int32_t V = (int32_t)modbus_regs[104];
    int32_t I = (int32_t)modbus_regs[114];
    int32_t P = (V * I) / 100;
    int16_t p16 = clamp_i16(P);
    modbus_regs[117] = (uint16_t)p16;

    if (batt.data.status == 4 || batt.data.protection != 0) modbus_regs[92] = 99;
    else if (batt.data.status == 0)                         modbus_regs[92] = 4;
    else                                                    modbus_regs[92] = 3;

    modbus_regs[96] = batt.data.alarm;
}

