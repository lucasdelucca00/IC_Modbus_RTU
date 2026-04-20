/*
 * sunspec.h
 *
 *  Created on: Dec 17, 2025
 *      Author: Lukinhas
 */

#pragma once
#include <stdint.h>

#define SUNSPEC_BASE 0

void sunspec_init(uint8_t modbus_id);
void sunspec_update(void);
