//*****************************************************************************
// Authors:     David Nguyen | Michael Stephens
// Group 4:     CoAP Server - Sensor
// Course:		Distributed Systems by Prof. Dr. Boris Boeck
//
// display_handler.c
//
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
#include "CFAF128128B0145T/CFAF128128B0145T.h"

#include "helper_functions/display_handler.h"

// The system clock speed.
extern uint32_t g_ui32SysClock;

char	luxBuffer[9];
char	tempBuffer[9];
float	luxValue, tempValue;

void ioDisplaySetup(void){

	// Header
    CFAF128128B0145T_text(10, 7, "[CoAP] Server", CFAF128128B0145T_color_white, CFAF128128B0145T_color_black, 1, 1);
    CFAF128128B0145T_text(10, 17, "Local Sensor Data", CFAF128128B0145T_color_white, CFAF128128B0145T_color_black, 1, 1);
    CFAF128128B0145T_rectangle(3,0,125,30, CFAF128128B0145T_color_white);

    // Constant Strings
    CFAF128128B0145T_text(5, 70, "Brightness:        ", CFAF128128B0145T_color_white, CFAF128128B0145T_color_black, 1, 1);
    CFAF128128B0145T_text(10, 80, " --- ", CFAF128128B0145T_color_white, CFAF128128B0145T_color_black, 1, 1);
    CFAF128128B0145T_text(5, 100, "Temperature:       ", CFAF128128B0145T_color_white, CFAF128128B0145T_color_black, 1, 1);
	CFAF128128B0145T_text(10, 110, " --- ", CFAF128128B0145T_color_white, CFAF128128B0145T_color_black, 1, 1);
}


void ioDisplayUpdate(uint32_t localIP)
{
    char    localStr[22];
    int     l;

    // IP info
    sprintf(&localStr[0]," %d.%d.%d.%d", localIP & 0xff, (localIP >> 8) & 0xff, (localIP >> 16) & 0xff, (localIP >> 24) & 0xff);
    l = strlen(&localStr[0]);
    memset(&localStr[l], ' ', 21-l);    // fill with SPACEs
    localStr[21] = '\0';                // terminate string

	update_opt3001();
	update_tmp600();

    switch(localIP){
        case 0xFFFFFFFF:
            //                            123456789012345678901
            CFAF128128B0145T_text(5, 40, "Waiting for LINK ... ", CFAF128128B0145T_color_white, CFAF128128B0145T_color_black, 1, 1);
            break;

        case 0:
            //                            123456789012345678901
            CFAF128128B0145T_text(5, 40, "Waiting for IP ...   ", CFAF128128B0145T_color_white, CFAF128128B0145T_color_black, 1, 1);
            break;

        default:

            //                            123456789012345678901
            CFAF128128B0145T_text(5, 40, "IP-Address:        ", CFAF128128B0145T_color_white, CFAF128128B0145T_color_black, 1, 1);
            CFAF128128B0145T_text(10, 50, localStr, CFAF128128B0145T_color_white, CFAF128128B0145T_color_black, 1, 1);


            CFAF128128B0145T_text(10, 80, luxBuffer, CFAF128128B0145T_color_white, CFAF128128B0145T_color_black, 1, 1);


            CFAF128128B0145T_text(10, 110, tempBuffer, CFAF128128B0145T_color_white, CFAF128128B0145T_color_black, 1, 1);
            break;
    }


    return;
}

static void update_opt3001(){
	// OPT3001 Info
	sensorOpt3001Read(&luxValue);
	sprintf(luxBuffer, " %5.2f\0", luxValue); // auto-scale-max: 10^4
}

static void update_tmp600(){
	// TMP600 Info
	tempValue = getTemperature();
	sprintf(tempBuffer," %0.2f �C\0", tempValue);
}

// Converts integer to ASCII for display output
/*
char* itoa(int val, char *a)
{
    char *e=a,*o=a,sign=val<0;
    do *e++="0123456789"[abs(val%10)]; while( val/=10 );
    for( sign?*e++='-':1,*e--=0; a < e; *a ^= *e,*e ^= *a,*a++ ^= *e-- );
    return o;
}*/
