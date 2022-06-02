//*****************************************************************************
// Authors:     David Nguyen | Michael Stephens
// Group 4:     CoAP Server - Sensor
// Course:		Distributed Systems by Prof. Dr. Boris Boeck
//
// io.h  I/O Routine
//*****************************************************************************

#ifndef __IO_H__
#define __IO_H__

#ifdef __cplusplus
extern "C"
{
#endif

#define BOOSTER_PACK_POSITION 2

/* BP1
#define BUTTON_PORT_BASE        GPIO_PORTH_BASE
#define BUTTON_PIN              GPIO_PIN_1

#define BUTTON_ENABLE_PERIPHERAL()   {SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOH);}
*/



//*****************************************************************************
// Prototypes.
//*****************************************************************************

void io_init(void);


#ifdef __cplusplus
}
#endif

#endif // __IO_H__
