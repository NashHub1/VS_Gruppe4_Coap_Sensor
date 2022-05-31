//*****************************************************************************
//
// io.h - Prototypes for I/O routines
//
//*****************************************************************************

#ifndef __IO_H__
#define __IO_H__

#ifdef __cplusplus
extern "C"
{
#endif

//*****************************************************************************
//
// Hardware connection for the user LED.
//
//*****************************************************************************
#define LED1_PORT_BASE          GPIO_PORTN_BASE
#define LED1_PIN                GPIO_PIN_1

#define LED2_PORT_BASE          GPIO_PORTN_BASE
#define LED2_PIN                GPIO_PIN_0



/** @brief Select booster pack position (1 or 2)    */
#define BOOSTER_PACK_POSITION   2



#if (BOOSTER_PACK_POSITION == 1)

    // RGB-LED
    //  Red:   PF2
    #define RGB_R_PORT_BASE          GPIO_PORTF_BASE
    #define RGB_R_PIN                GPIO_PIN_2
    //  Green: PF3
    #define RGB_G_PORT_BASE          GPIO_PORTF_BASE
    #define RGB_G_PIN                GPIO_PIN_3
    //  Blue:  PG0
    #define RGB_B_PORT_BASE          GPIO_PORTG_BASE
    #define RGB_B_PIN                GPIO_PIN_0

    #define RGB_ENABLE_PERIPHERAL()   {SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);\
                                   SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);}


    // Button S1: PL1
    #define BUTTON_PORT_BASE        GPIO_PORTL_BASE
    #define BUTTON_PIN              GPIO_PIN_1

    #define BUTTON_ENABLE_PERIPHERAL()   {SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL);\


#elif (BOOSTER_PACK_POSITION == 2)

    // RGB-LED
    //  Red:   PK4
    #define RGB_R_PORT_BASE          GPIO_PORTK_BASE
    #define RGB_R_PIN                GPIO_PIN_4
    //  Green: PK5
    #define RGB_G_PORT_BASE          GPIO_PORTK_BASE
    #define RGB_G_PIN                GPIO_PIN_5
    //  Blue:  PM0
    #define RGB_B_PORT_BASE          GPIO_PORTM_BASE
    #define RGB_B_PIN                GPIO_PIN_0


    #define RGB_ENABLE_PERIPHERAL()   {SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);\
                                       SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOM);}


    // Button S1: PH1
    #define BUTTON_PORT_BASE        GPIO_PORTH_BASE
    #define BUTTON_PIN              GPIO_PIN_1

    #define BUTTON_ENABLE_PERIPHERAL()   {SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOH);}


#else

    #error("Booster Pack position not defined!")

#endif






#define TEXT_MAX_NB_OF_TEXTES   3
#define TEXT_MAX_NB_OF_CHARS    50


typedef struct{
    unsigned char r;
    unsigned char g;
    unsigned char b;
} rgb_t;



//*****************************************************************************
//
// Exported function prototypes.
//
//*****************************************************************************

/** @brief Initialize IOs (ports, display, etc.)
*/
void io_init(void);


/** @brief Print data to display
* @param  localIP    : lokal IP address
*/
void io_display(uint32_t localIP);




/** @brief Get state of test LED
* @return  state 0(off) or 1(on)
*/
int io_ledGetState(void);

/** @brief Set state of test LED
* @param  state         : state 0(off) or 1(on)
* @return state 0(off) or 1(on)
*/
int io_ledSetState(int state);



/** @brief Get demo text
* @param  textID        : ID of text (0 or 1)
* @param  pText         : Pointer to char array (Destination). Requested string will be copied to this address
*/
void io_textGet(int textID, char *pText);

/** @brief Set demo text
* @param  textID        : ID of text (0 or 1)
* @param  pText         : Pointer to char array (Source). String will be read from this address
*/
void io_textSet(int textID, char *pText);



/** @brief Get demo rgb values
* @return structure of type rgb_t with the current rgb values
*/
rgb_t io_ledRGBGet(void);

/** @brief Get demo rgb values
* @param  rgbValue: structure of type rgb_t with the current rgb values
*/
void io_ledRGBSet(rgb_t rgbValue);






/** @brief Get state of button
* @return  state 0(not pushed) or 1(pushed)
*/
int io_buttonGetState(void);



float io_temperatureGet(void);



#ifdef __cplusplus
}
#endif

#endif // __IO_H__
