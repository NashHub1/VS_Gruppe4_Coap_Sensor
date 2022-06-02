//*****************************************************************************
// Authors:     David Nguyen | Michael Stephens
// Group 4:     CoAP Server - Sensor
// Course:		Distributed Systems by Prof. Dr. Boris Boeck
//
// io.c  I/O Routine (based on free_rtos_task from lecture)
//*****************************************************************************
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
#include "driverlib/rom_map.h"
#include "drivers/pinout.h"

#include "CFAF128128B0145T/CFAF128128B0145T.h"      // Display

#include "io.h"

#include "coap_lib/coap_handler.h"

// The system clock speed.
extern uint32_t g_ui32SysClock;



//*****************************************************************************
// Initialize IO ( Display, RGB )
//*****************************************************************************
void io_init(void)
{

    PinoutSet(true, false); // LED D3 and D4 for Ethernet connection

    // Initialize Display module
    CFAF128128B0145T_init(BOOSTER_PACK_POSITION, g_ui32SysClock, 20000000);

    return;
}

