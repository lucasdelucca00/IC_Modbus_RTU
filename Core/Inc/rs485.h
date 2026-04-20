/*
 * rs485.h
 *
 *  Created on: Dec 17, 2025
 *      Author: Lukinhas
 */

#pragma once
#include "stm32f1xx_hal.h"

void rs485_tx(GPIO_TypeDef *port, uint16_t pin);
void rs485_rx(GPIO_TypeDef *port, uint16_t pin);

