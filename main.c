//*****************************************************************************
// Authors:     David Nguyen | Michael Stephens
// Group 4:     CoAP Server - Sensor
// Course:		Distributed Systems by Prof. Dr. Boris Boeck
//
// Main.c
//
// License:
// --------
// Copyright (c) 2014-2016 Cesanta Software Limited
// All rights reserved
//
//*****************************************************************************
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "third_party/mongoose.h"

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_nvic.h"
#include "inc/hw_types.h"
#include "driverlib/flash.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/timer.h"
#include "driverlib/rom_map.h"
#include "utils/uartstdio.h"


#include "utils/lwiplib.h"


/* User-Libaries */
#include "userlib/io.h"

/* Helper Functions for RTOS-Tasks and Coap_handler */
#include "coap_lib/coap_handler.h"
#include "helper_functions/temperature_handler.h"
#include "helper_functions/lightsensor_handler.h"
#include "helper_functions/display_handler.h"

//-----------------------------------------------------------------------------
// Variable declarations
//-----------------------------------------------------------------------------
uint32_t g_ui32SysClock;	// system clock frequency
uint32_t g_ui32IPAddress;	// IP address


//-----------------------------------------------------------------------------
// Coap Defaults
//-----------------------------------------------------------------------------
char *s_default_address = "udp://:5683";
struct mg_mgr mgr;



//-----------------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------------

/* Neccessary for connection */
int gettimeofday(struct timeval *tv, void *tzvp) {
  tv->tv_sec = time(NULL);
  tv->tv_usec = 0;
  return 0;
}

void mg_lwip_mgr_schedule_poll(struct mg_mgr *mgr) {
}

/* Task declarations */
void DisplayTask(void *pvParameters);
void CoapTask(void *pvParameters);


//-----------------------------------------------------------------------------
// The error routine that is called if the driver library encounters an error.
//-----------------------------------------------------------------------------
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif

//*****************************************************************************
// Required by lwIP library to support any host-related timer functions.
// This function is called in "lwIPServiceTimers()" from the "lwiplib.c" utility
//   "lwIPServiceTimers()" is called in "lwIPEthernetIntHandler()" from the "lwiplib.c" utility
//     "lwIPEthernetIntHandler()" is registered in the interrupt vector table (in file "..._startup_ccs.c")
//*****************************************************************************
void lwIPHostTimerHandler(void)
{
    uint32_t ui32NewIPAddress;

    // Get the current IP address.
    ui32NewIPAddress = lwIPLocalIPAddrGet();

    // See if the IP address has changed.
    if(ui32NewIPAddress != g_ui32IPAddress)
    {
        // See if there is an IP address assigned.
        if(ui32NewIPAddress == 0xffffffff)
        {
            // Indicate that there is no link.
            UARTprintf("Waiting for link.\n");
        }
        else if(ui32NewIPAddress == 0)
        {
            // There is no IP address, so indicate that the DHCP process is
            // running.
            UARTprintf("Waiting for IP address.\n");
        }
        else
        {
            // Display the new IP address.
			UARTprintf("IP Address: %s\n", ipaddr_ntoa((const ip_addr_t *) &ui32NewIPAddress));

        }

        // Save the new IP address.
        g_ui32IPAddress = ui32NewIPAddress;
    }

}


//-----------------------------------------------------------------------------
// Main function
// Coap Server- Sensor
//-----------------------------------------------------------------------------
int main(void)
{
    uint32_t ui32User0, ui32User1;
    uint8_t pui8MACArray[8];

    //-----------------------------------------------------------------------------
    // Make sure the main oscillator is enabled because this is required by
    // the PHY.  The system must have a 25MHz crystal attached to the OSC
    // pins.  The SYSCTL_MOSC_HIGHFREQ parameter is used when the crystal
    // frequency is 10MHz or higher.
    SysCtlMOSCConfigSet(SYSCTL_MOSC_HIGHFREQ);

    //
    // Run from the PLL at 120 MHz.
    //
    g_ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                             SYSCTL_OSC_MAIN |
                                             SYSCTL_USE_PLL |
                                             SYSCTL_CFG_VCO_480), 120000000);
    //-----------------------------------------------------------------------------


    // GP-/IO Pin Configuration
    io_init();


    // debug terminal
    UARTStdioConfig(0, 115200, g_ui32SysClock);

    // clear terminal and print title
    UARTprintf("\033[2J\033[H");
    UARTprintf("===================================\n");
    UARTprintf("CoAP - Server & Sensor - Gruppe 4\n");
    UARTprintf("===================================\n\n");


    // Configure the hardware MAC address for Ethernet Controller filtering of
    // incoming packets.  The MAC address will be stored in the non-volatile
    // USER0 and USER1 registers.
    // ************************************************************************
    MAP_FlashUserGet(&ui32User0, &ui32User1);
    if((ui32User0 == 0xffffffff) || (ui32User1 == 0xffffffff))
    {
        //
        // Let the user know there is no MAC address
        //
        UARTprintf("No MAC programmed!\n");

        while(1)
        {
        }
    }


    // Convert the 24/24 split MAC address from NV ram into a 32/16 split
    // MAC address needed to program the hardware registers, then program
    // the MAC address into the Ethernet Controller registers.
    // ******************************************************************
    pui8MACArray[0] = ((ui32User0 >>  0) & 0xff);
    pui8MACArray[1] = ((ui32User0 >>  8) & 0xff);
    pui8MACArray[2] = ((ui32User0 >> 16) & 0xff);
    pui8MACArray[3] = ((ui32User1 >>  0) & 0xff);
    pui8MACArray[4] = ((ui32User1 >>  8) & 0xff);
    pui8MACArray[5] = ((ui32User1 >> 16) & 0xff);


    // Initialize the lwIP library, using DHCP. (
    lwIPInit(g_ui32SysClock, pui8MACArray, 0, 0, 0, IPADDR_USE_DHCP);


    mg_mgr_init(&mgr, NULL);

    // Create new task | Stack = (uint16_t)1024 (minimal stack size)
    xTaskCreate(DisplayTask, (const portCHAR *)"displaytask", 1024, NULL, 1, NULL);
    xTaskCreate(CoapTask, (const portCHAR *)"coaptask", 1024, NULL, 1, NULL);

    // Start the created tasks running
    vTaskStartScheduler();


    // Execution should never reach this point as the scheduler is running the tasks
    // If execution reaches here, then there might be insufficient heap memory for creating the idle task
    while(1){};



}

//*****************************************************************************
// Function: Tasks are blocked with different delays. Thereby, they run through
// a continuous loop, which allows the parallel execution of the two tasks.
//*****************************************************************************
void DisplayTask(void *pvParameters)
{
    sensorTmp006Setup();
    sensorOpt3001Setup();
    ioDisplaySetup();

    while(1){
    	ioDisplayUpdate(g_ui32IPAddress);

    	vTaskDelay( pdMS_TO_TICKS( 500 ) );
    }
}



void CoapTask(void *pvParameters)
{
	vTaskDelay( pdMS_TO_TICKS( 1000 ) ); // delay 1000 milliseconds for setup

	struct mg_connection *nc;
    nc =  mg_bind(&mgr, s_default_address, coap_handler);

    if (nc == NULL) {
        UARTprintf("Failed to create listener: %s\r\n");
        while(1){}
    }

    // Set CoAP-Protocoll
    mg_set_protocol_coap(nc);


    while(1){

        mg_mgr_poll(&mgr, 0);
        vTaskDelay( pdMS_TO_TICKS( 100 ) ); // delay 100 milliseconds
    }
}


