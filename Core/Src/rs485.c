/*
 * rs485.c
 *
 *  Created on: Dec 17, 2025
 *      Author: Lukinhas
 *
 *      Aqui vamos controlar o DE do transceptor, nivel alto Tx (transmite) , nivel baixo Rx (recebe)
 */

#include "rs485.h"

void rs485_tx(GPIO_TypeDef *port, uint16_t pin)
{
    HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
}

void rs485_rx(GPIO_TypeDef *port, uint16_t pin)
{
    HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);
}

