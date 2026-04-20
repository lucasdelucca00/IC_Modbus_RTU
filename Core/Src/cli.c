/*
 * cli.c
 *
 *  Created on: 20 de abr. de 2026
 *      Author: LucasDeLucca-MGInfo
 */

#include "cli.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "battery_client.h"
#include <stdlib.h>

extern UART_HandleTypeDef huart2;
extern modbus_battery_t batt;
extern uint8_t batt_err_code;

// Variável para controlar o modo
static uint32_t last_activity = 0;
static uint8_t cli_active = 0;

void cli_printf(const char *fmt, ...) {
    char buf[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    HAL_UART_Transmit(&huart2, (uint8_t*)buf, strlen(buf), 100);
    va_end(args);
}

void ProcessarComando(char *cmd) {
    cli_active = 1;
    last_activity = osKernelSysTick();

    if (strcmp(cmd, "help") == 0) {
        cli_printf("\r\nComandos: status, batt, modbus, regs, all, clear\r\n");
    }
    else if (strcmp(cmd, "status") == 0) {
        cli_printf("\r\nComm Status: %s (Err:%d)\r\n", (batt_err_code == 99 ? "OK" : "FALHA"), batt_err_code);
    }
    else if (strcmp(cmd, "batt") == 0) {
        cli_printf("\r\nBatt Voltage: %d.%02d V\r\n", batt.data.voltage_dV/100, batt.data.voltage_dV%100);
    }
    else if (strcmp(cmd, "modbus") == 0) {
        cli_printf("\r\n--- DUMP MODBUS (Primeiros 10 regs) ---\r\n");
        for(int i = 0; i < 10; i++) {
            // Imprime o índice e o valor em Hexadecimal (4 dígitos)
            cli_printf("Reg[%02d]: 0x%04X\r\n", i, batt.regs[i]);
        }
    }
    else if (strcmp(cmd, "regs") == 0) {
        cli_printf("\r\n--- INTERPRETACAO DOS REGISTRADORES ---\r\n");
        cli_printf("Voltagem Total: %d.%02d V\r\n", batt.regs[0]/100, batt.regs[0]%100);
        cli_printf("Corrente:       %d A\r\n", (int16_t)batt.regs[1]);
        cli_printf("Temp. Interna:  %d C\r\n", (int16_t)batt.regs[2]);
        cli_printf("SoC:            %d %%\r\n", batt.regs[3]);
    }
    else if (strcmp(cmd, "all") == 0) {
        cli_printf("\r\n--- RELATORIO COMPLETO ---\r\nSoH:%d%% | SoC:%d%%\r\n", batt.data.soh, batt.data.soc);
    }
    else if (strcmp(cmd, "clear") == 0) {
        cli_printf("\033[2J\033[H");
    }
    else {
        cli_printf("\r\nComando nao encontrado!\r\n");
    }
}

void StartCLITask(void const * argument) {
    char rx_buf[32];
    int rx_idx = 0;
    char c;
    memset(rx_buf, 0, sizeof(rx_buf)); // Garante buffer limpo

    cli_printf("\033[2J\033[H"); // Limpa tela
    cli_printf("MODBUS RTU - Baterias Litio Unipower UPLFP48\r\n");
    cli_printf("\r\nUTFPR CLI - Digite 'help'\r\n> ");
    for(;;) {
        // Recebe 1 caractere com timeout curto (10ms) para não travar a task
        if (HAL_UART_Receive(&huart2, (uint8_t*)&c, 1, 10) == HAL_OK) {

            // Tratamento de Backspace (8 é backspace, 127 é delete)
            if (c == 8 || c == 127) {
                if (rx_idx > 0) {
                    rx_idx--;
                    cli_printf("\b \b"); // Apaga visualmente no terminal
                }
            }
            // Tratamento de Enter
            else if (c == '\r' || c == '\n') {
                rx_buf[rx_idx] = '\0';
                if(rx_idx > 0) { // Só processa se tiver algo
                    ProcessarComando(rx_buf);
                }
                rx_idx = 0;
                memset(rx_buf, 0, sizeof(rx_buf)); // Limpa o buffer pós-comando
                cli_printf("\r\n> ");
            }
            // Tratamento de digitação normal
            else if (rx_idx < 31) {
                rx_buf[rx_idx++] = c;
                HAL_UART_Transmit(&huart2, (uint8_t*)&c, 1, 10); // Echo visual
            }
        }
        osDelay(20); // Delay suave para não consumir CPU desnecessária
    }
}
