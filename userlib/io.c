//*****************************************************************************
// Authors:     David Nguyen | Michael Stephens
// Group 4:     CoAP Server - Sensor
// Course:		Distributed Systems by Prof. Dr. Boris Boeck
//
// io.c  I/O Routine
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



int     io_ledState;
char    textData[TEXT_MAX_NB_OF_TEXTES][TEXT_MAX_NB_OF_CHARS+1];



//*****************************************************************************
// Initialize IO ( Display, RGB )
//*****************************************************************************
void io_init(void)
{

    PinoutSet(true, false); // LED D3 and D4 for Ethernet connection

    // Initialize Display module
    CFAF128128B0145T_init(BOOSTER_PACK_POSITION, g_ui32SysClock, 20000000);


    io_buttonInit();

    io_rgbLedInit();


    io_ledSetState(0);

    return;
}



int io_rgbLedInit(void)
{
    // Enable peripherals
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK||SYSCTL_PERIPH_GPIOM);
    // Set pin LED1_PIN at port LED1_PORT_BASE as output, SW controlled.
    ROM_GPIOPinTypeGPIOOutput(RGB_R_PORT_BASE, RGB_R_PIN);
    ROM_GPIOPinTypeGPIOOutput(RGB_G_PORT_BASE, RGB_G_PIN);
    ROM_GPIOPinTypeGPIOOutput(RGB_B_PORT_BASE, RGB_B_PIN);

    // switch LED off (0)
    ROM_GPIOPinWrite(RGB_R_PORT_BASE, RGB_R_PIN, 0);
    ROM_GPIOPinWrite(RGB_G_PORT_BASE, RGB_G_PIN, 0);
    ROM_GPIOPinWrite(RGB_B_PORT_BASE, RGB_B_PIN, 0);

    return(1);
}



int io_ledGetState(void)
{
    return(io_ledState);
}


int io_ledSetState(int state)
{
    io_ledState = state;

    if(state){
        // switch LED off (0)
        ROM_GPIOPinWrite(RGB_R_PORT_BASE, RGB_R_PIN, RGB_R_PIN);
        ROM_GPIOPinWrite(RGB_G_PORT_BASE, RGB_G_PIN, RGB_G_PIN);
        ROM_GPIOPinWrite(RGB_B_PORT_BASE, RGB_B_PIN, RGB_B_PIN);
    }
    else{
        // switch LED off (0)
        ROM_GPIOPinWrite(RGB_R_PORT_BASE, RGB_R_PIN, 0);
        ROM_GPIOPinWrite(RGB_G_PORT_BASE, RGB_G_PIN, 0);
        ROM_GPIOPinWrite(RGB_B_PORT_BASE, RGB_B_PIN, 0);
    }


    return(state);
}





// textID 0 or 1
void io_textGet(int textID, char *pText)
{
    if(textID < TEXT_MAX_NB_OF_TEXTES){
        strcpy(pText, &textData[textID][0]);
    }
    else{
        pText = '\0';   // set empty text
    }
    return;
}


// textID 0 or 1
void io_textSet(int textID, char *pText)
{
    if(textID < TEXT_MAX_NB_OF_TEXTES){
        strcpy(&textData[textID][0], pText);
    }
    return;
}




int io_buttonInit(void)
{
    BUTTON_ENABLE_PERIPHERAL();

    // Set BUTTON_PORT_BASE, BUTTON_PORT_BASE as input, SW controlled. With internal weak pull-up
    GPIOPinTypeGPIOInput(BUTTON_PORT_BASE,  BUTTON_PIN);                                        // Set BUTTON_PORT_BASE,  BUTTON_PIN as input
    GPIOPadConfigSet(BUTTON_PORT_BASE,  BUTTON_PIN, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);  // Activate weak pull up at BUTTON_PORT_BASE,  BUTTON_PIN

}

int io_buttonGetState(void)
{

    // Read pin BUTTON_PIN
    if((GPIOPinRead(BUTTON_PORT_BASE, BUTTON_PIN) & BUTTON_PIN) != 0){
        // button not pushed (pull-up keeps the line high)
        return(0);
    }
    else{
        return(1);
    }

}


