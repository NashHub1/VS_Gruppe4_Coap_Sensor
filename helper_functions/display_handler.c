/*
 * display_handler.c
 *
 *  Created on: 31.05.2022
 *      Author: David Nguyen
 */


#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_pwm.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"      // allows the use of the MAP_ prefix
#include "drivers/pinout.h"

#include "CFAF128128B0145T/CFAF128128B0145T.h"
#include "userlib/io.h"


// The system clock speed.
extern uint32_t g_ui32SysClock;

