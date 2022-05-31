/*
 * temperature_testApp.c
 *
 *  Created on: 29.05.2022
 *      Author: David Nguyen
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <inc/hw_gpio.h>
#include <inc/hw_i2c.h>
#include <inc/hw_ints.h>
#include <inc/hw_memmap.h>
#include <inc/hw_types.h>
#include <driverlib/gpio.h>
#include <driverlib/i2c.h>
#include <driverlib/interrupt.h>
#include <driverlib/pin_map.h>
#include <driverlib/sysctl.h>
#include <driverlib/timer.h>
#include <sensorlib/hw_tmp006.h>
#include "CFAF128128B0145T/CFAF128128B0145T.h"


/*-----------------------------------------------------------
 * Macro Definition (Enum)
 *----------------------------------------------------------*/
#define TMP006_I2C_ADDRESS      0x40
#define I2C_DEVICE              I2C2_BASE
#define TMP006_MANUFACTURER_ID  0xFE
#define TMP006_DEVICE_ID        0xFF
#define TMP006_CONFIG           0x02  	//Configuration Register
#define TMP006_DieTemp          0x01  	// DIE-Temperature Register
#define RESET_TMP006            0x8000 	// Software Reset Bit[15]=1. (Page.20 TMP006 Data Sheet)
#define ENABLE_DRDY             0x7500 	// Bits [14:12]=111 ->Sensor and die continuous conversion (MOD)

/*-----------------------------------------------------------
 * Structure Template
 *----------------------------------------------------------*/
typedef struct i2c_ids
{
    uint16_t tmp006_mfct_id;
    uint16_t tmp006_dev_id;
} I2C_IDS, *PI2C_IDS; //Variable Namen, durch typedef können jetzt direkt andere variable definiert werden
                      // zB. I2C_IDS first_ID, second_ID; mit dem Pointer PI2C_IDS kann auf eins von beiden gezeigt werden.

I2C_IDS TMP006_IDs;

/*-----------------------------------------------------------
 * Variable Declaration
 *----------------------------------------------------------*/
uint32_t ui32Period;    // Timer value
char strBuffer[20];
volatile float dietemp = 0.0, dietempMin =0.0, dietempHold = 0.0;

// EXTERN
extern uint32_t ui32SysClkFreq;
extern uint32_t ClockCylces;

/*-----------------------------------------------------------
 * Prototypes
 *----------------------------------------------------------*/
uint16_t ReadI2CRegister(uint8_t i2cAdress, uint8_t registerName);
void WriteI2CRegister(uint8_t i2cAdress, uint8_t registerName, uint16_t value);
void Timer1IntHandler(void);    // for debouncing buttons
void GPIOMIntHandler(void);     // for TMP006 temp sensor
void InitHardware();    		// all functions for HW configuration


#ifdef DEBUG
void __error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif



// TODO: display anzeige auslagern auf externe klasse
void InitHardware()
{
	// TODO: clock freq ueber externe variable aus der main beziehen
    //uint32_t ui32SysClkFreq;
    uint32_t ClockCylces;        // Timer Frequency

    //Timer Clock
    ClockCylces = (ui32SysClkFreq/60);  //ungefähr >15 ms    120M6/60= 2M6 * 8,33ns= 16,6 ms

    // Display, Parameters
    //CFAF128128B0145T_init(2, ui32SysClkFreq, 20000000);

    // Write text
    //CFAF128128B0145T_text(12, 10, "M.Stephens-TMP006", CFAF128128B0145T_color_white, CFAF128128B0145T_color_black, 1, 1);
    //CFAF128128B0145T_text(15, 22, "S1: Reset Min-Temp", CFAF128128B0145T_color_white, CFAF128128B0145T_color_black, 1, 1);
    //CFAF128128B0145T_text(15, 32, "S2: Save Temp", CFAF128128B0145T_color_white, CFAF128128B0145T_color_black, 1, 1);

    //Enable peripheral
    SysCtlPeripheralReset(SYSCTL_PERIPH_I2C2); // Reset I2C
    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C2); // Enable I2C
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION); // SCL:PN5 , SDA:PN4, LED:PN1
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOM); // TMP006 INT:PM6
    //SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOH); // Button 1 (PH1)
    //SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK); // Button 2 (PK6)
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1); // TIMER1
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER1) || !SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOK) ||
          !SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOH) || !SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOM) ||
          !SysCtlPeripheralReady(SYSCTL_PERIPH_GPION) || !SysCtlPeripheralReady(SYSCTL_PERIPH_I2C2));

    SysCtlDelay(3); // Übernommen aus dem alten Laboren,ich denke hier aber nicht mehr nötig. Da while()

    //Timer Configuration
    TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC);
    TimerLoadSet(TIMER1_BASE, TIMER_A, ClockCylces-1); // >15 ms

    //Reset LEDs
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE,GPIO_PIN_1);
    GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, 0x00);

    //TMP006 Interrupt Configuration
    GPIOPinTypeGPIOInput(GPIO_PORTM_BASE, GPIO_PIN_6);
    GPIOIntTypeSet(GPIO_PORTM_BASE, GPIO_INT_PIN_6, GPIO_FALLING_EDGE);
    GPIOIntClear(GPIO_PORTM_BASE, GPIO_INT_PIN_6);

    //Button 1-2 Interrupt Configuration

    GPIOPinTypeGPIOInput(GPIO_PORTH_BASE, GPIO_PIN_1);  //Button1 S1
    GPIOPinTypeGPIOInput(GPIO_PORTK_BASE, GPIO_PIN_6);  //Button2 S2
    GPIOPadConfigSet(GPIO_PORTH_BASE,GPIO_PIN_1, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);// Es fließen 3.3V / 10kOhm. Es ist ein weak Pull-Up
    GPIOPadConfigSet(GPIO_PORTK_BASE,GPIO_PIN_6, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);//== 0,3 mA - nicht gedrückt
    GPIOIntTypeSet(GPIO_PORTH_BASE, GPIO_PIN_1, GPIO_FALLING_EDGE); // Nicht gedrückt= High
    GPIOIntTypeSet(GPIO_PORTK_BASE, GPIO_PIN_6, GPIO_FALLING_EDGE); // Gedrückt= Low

    //I2C Pins configuration
    GPIOPinTypeI2CSCL(GPIO_PORTN_BASE, GPIO_PIN_5); //SCL
    GPIOPinTypeI2C(GPIO_PORTN_BASE, GPIO_PIN_4);    //SDA
    GPIOPinConfigure(GPIO_PN5_I2C2SCL);             //SCL
    GPIOPinConfigure(GPIO_PN4_I2C2SDA);             //SDA


    // Initialize I2C master peripheral, false = 100 kBits/s, true = 400 kBit/s
    I2CMasterInitExpClk(I2C_DEVICE, ui32SysClkFreq, false);
    SysCtlDelay(10000);         // wait a bit



    // Read and Display Register-IDs
    TMP006_IDs.tmp006_mfct_id = ReadI2CRegister(TMP006_I2C_ADDRESS, TMP006_MANUFACTURER_ID);
    TMP006_IDs.tmp006_dev_id = ReadI2CRegister(TMP006_I2C_ADDRESS, TMP006_DEVICE_ID);

    //CFAF128128B0145T_text(10, 50,"Manufct-ID:", CFAF128128B0145T_color_white, CFAF128128B0145T_color_black, 1, 1);
    sprintf(strBuffer, "0x%04x", TMP006_IDs.tmp006_mfct_id);
    CFAF128128B0145T_text(80, 50,strBuffer, CFAF128128B0145T_color_white, CFAF128128B0145T_color_black, 1, 1);
    //CFAF128128B0145T_text(80, 45,id, CFAF128128B0145T_color_white, CFAF128128B0145T_color_black, 1, 1);

    CFAF128128B0145T_text(10, 60,"Device-ID:", CFAF128128B0145T_color_white, CFAF128128B0145T_color_black, 1, 1);
    sprintf(strBuffer, "0x%04x", TMP006_IDs.tmp006_dev_id);
    CFAF128128B0145T_text(80, 60,strBuffer, CFAF128128B0145T_color_white, CFAF128128B0145T_color_black, 1, 1);

    // Set Configuration Register - Reset and enable DRDY
    WriteI2CRegister(TMP006_I2C_ADDRESS, TMP006_CONFIG, RESET_TMP006);
    WriteI2CRegister(TMP006_I2C_ADDRESS, TMP006_CONFIG, ENABLE_DRDY);

    // Interrupts Enable
    GPIOIntEnable(GPIO_PORTM_BASE, GPIO_INT_PIN_6); //  GPIO INT Enable  in PM6
    IntEnable(INT_GPIOM);                           // Enable INT from Port M

    TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);// Timer Timeout enable
    IntEnable(INT_TIMER1A);                         // Enable IntHandler for Timer1A


    IntMasterEnable();                              // Gooo
    TimerEnable(TIMER1_BASE, TIMER_A);              // Goooo

    CFAF128128B0145T_rectangle(5,75,125,125, CFAF128128B0145T_color_blue);

}



uint16_t ReadI2CRegister(uint8_t i2cAdress, uint8_t registerName)

{
    uint16_t Return_16bit;

        I2CMasterSlaveAddrSet(I2C_DEVICE, i2cAdress, false);
        I2CMasterDataPut(I2C_DEVICE, registerName);
        I2CMasterControl(I2C_DEVICE, I2C_MASTER_CMD_SINGLE_SEND);
        SysCtlDelay(500);                           // wait 500
        while (I2CMasterBusy(I2C_DEVICE)) { }        // check if I2C still busy

        SysCtlDelay(1000);

        // Read content (2x8 bit) of selected register
        I2CMasterSlaveAddrSet(I2C_DEVICE, i2cAdress, true);
        // Receive first data byte
        I2CMasterControl(I2C_DEVICE, I2C_MASTER_CMD_BURST_RECEIVE_START);
        SysCtlDelay(500);                           // wait 500
        while (I2CMasterBusy(I2C_DEVICE)) { }        // check if I2C still busy


        Return_16bit= I2CMasterDataGet(I2C_DEVICE);                                 // first data byte (MSB)
        //id[0]=I2CMasterDataGet(I2C_DEVICE);
        Return_16bit <<= 8 ;
        // Receive second/last data byte
        I2CMasterControl(I2C_DEVICE, I2C_MASTER_CMD_BURST_RECEIVE_FINISH);
        SysCtlDelay(500);                           // wait 500
        while (I2CMasterBusy(I2C_DEVICE)) { }        // check if I2C still busy

        Return_16bit|= I2CMasterDataGet(I2C_DEVICE);// second data byte (LSB)
        //id[1]=I2CMasterDataGet(I2C_DEVICE);
        return Return_16bit;
}

void WriteI2CRegister(uint8_t i2cAdress, uint8_t registerName, uint16_t value)
{
	uint8_t byte[2];
	byte[0] = (value >> 8);
	byte[1] = value; // Eventuell geht es nicht, aber spielt keine Rolle, Byte 0 isch wichtig

	I2CMasterSlaveAddrSet(I2C_DEVICE, i2cAdress, false);
	I2CMasterDataPut(I2C_DEVICE, registerName);
	I2CMasterControl(I2C_DEVICE, I2C_MASTER_CMD_BURST_SEND_START);
	SysCtlDelay(500);
	while(I2CMasterBusy(I2C_DEVICE)){}

	I2CMasterDataPut(I2C_DEVICE, byte[0]);
	I2CMasterControl(I2C_DEVICE, I2C_MASTER_CMD_BURST_SEND_CONT);
	SysCtlDelay(500);
	while(I2CMasterBusy(I2C_DEVICE)){}

	I2CMasterDataPut(I2C_DEVICE, byte[1]);
	I2CMasterControl(I2C_DEVICE, I2C_MASTER_CMD_BURST_SEND_FINISH);
	SysCtlDelay(500);
	while (I2CMasterBusy(I2C_DEVICE)){}
}

void Timer1IntHandler(void)
{
    TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);


    static bool pushedB1 = false;       // debounced button states
    static bool pushedB2 = false;
    static uint8_t CounterPressedB1 = 0, CounterPressedB2 = 0;               // Counter for pressed state
    static uint8_t CounterReleasedB1 = 0, CounterReleasedB2 = 0;               // Counter for released state


    uint8_t Button1 = GPIOPinRead(GPIO_PORTH_BASE, GPIO_PIN_1);
    uint8_t Button2 = GPIOPinRead(GPIO_PORTK_BASE, GPIO_PIN_6);

    if(Button1 == false) //counting inputs
    {
        CounterPressedB1++;
    }
    CounterReleasedB1++;

    if(Button2 == false)
    {
        CounterPressedB2++;
    }

    CounterReleasedB2++;

    // two consecutive counts = button pushed
    if(CounterPressedB1 == 2 && pushedB1 == false) //S1 pushed
    {
        pushedB1 = true;
        CounterReleasedB1 = 0;
    }
    if(CounterReleasedB1 == 2 && pushedB1 == true) //S1 release
    {
        pushedB1 = false;
        CounterPressedB1 = 0;
    }
    if(CounterPressedB2 == 2 && pushedB2 == false) //S2 pushed
    {
        pushedB2 = true;
        CounterReleasedB2 = 0;
    }
    if(CounterReleasedB2 == 2 && pushedB2 == true) //S2 release
    {
        pushedB2 = false;
        CounterPressedB2 = 0;
    }
    if(dietempMin == 0)
    {
        dietempMin = dietemp;
    }
    if(pushedB1 == true)
    {
        dietempMin = dietemp;
    }
    if(pushedB2 == true)
    {
        dietempHold = dietemp;
    }
}

void GPIOMIntHandler(void)
{
    uint16_t DieTempRaw; //Raw
    uint8_t DRDY_Check; //Brauchen nur den siebten bit
    float DIE32;

    GPIOIntClear(GPIO_PORTM_BASE, GPIO_INT_PIN_6);

    GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, 0x2); // Licht an, new Temp

    // Check ConfigRegister - Data Ready Bit
    while(1)
    {
        SysCtlDelay(1000);
        DRDY_Check = ReadI2CRegister(TMP006_I2C_ADDRESS, TMP006_CONFIG);
        if (DRDY_Check & TMP006_CONFIG_DRDY) // Bit 7 wird abgefragt
            break;
    } // Sollte nicht hier rein, in einen Int HAndler... könnte was hängen bleiben zBdas I2C ist ziemlich lansgam... sollte alles schnell gemacht machen in ein Int HAndler
    /// lieber in Zeile 85 rein machen. Allgemein das InterruptHandler sollte nicht arbeiten.

    //GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, 0x00); // Licht aus

    // Convert DieTempRaw to celsius
    DieTempRaw = ReadI2CRegister(TMP006_I2C_ADDRESS, TMP006_DieTemp);

    if (DieTempRaw & (1 << 15))
    {
        DIE32 = (((DieTempRaw ^ 0xFFFF) >> 2) + 1); // (Alternative calculation) Page.15 TMP006 Data Sheet
        dietemp = DIE32 / -32;
    }
    else
    {
        DIE32 = (DieTempRaw >> 2);
        dietemp = DIE32 / 32;
    }

    CFAF128128B0145T_text(10,85,"Current Temp:", CFAF128128B0145T_color_white,  CFAF128128B0145T_color_black, 1, 1);
    sprintf(strBuffer, "%.2f", dietemp);
    CFAF128128B0145T_text(90,85, strBuffer, CFAF128128B0145T_color_white,  CFAF128128B0145T_color_black, 1, 1);

    CFAF128128B0145T_text(10,95, "Min Temp:", CFAF128128B0145T_color_white,  CFAF128128B0145T_color_black, 1, 1);

    if(dietempMin >= dietemp)
    {
    dietempMin = dietemp;
    sprintf(strBuffer, "%.2f", dietempMin);
    CFAF128128B0145T_text(90, 95, strBuffer, CFAF128128B0145T_color_white, CFAF128128B0145T_color_black, 1, 1);
    }

    CFAF128128B0145T_text(10,105, "Saved Temp:", CFAF128128B0145T_color_white,  CFAF128128B0145T_color_black, 1, 1);
    sprintf(strBuffer, "%.2f", dietempHold);
    CFAF128128B0145T_text(90,105, strBuffer, CFAF128128B0145T_color_white,  CFAF128128B0145T_color_black, 1, 1);

    GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, 0x00); // Licht aus
}

// Converts integer to ASCII for display output
char* itoa(int val, char *a)
{
    char *e=a,*o=a,sign=val<0;
    do *e++="0123456789"[abs(val%10)]; while( val/=10 );
    for( sign?*e++='-':1,*e--=0; a < e; *a ^= *e,*e ^= *a,*a++ ^= *e-- );
    return o;
}




