/*
 * lightsensor_handler.c
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
#include <math.h>
#include <helper_functions/lightsensor_handler.h>

//*****************************************************************************
// I2C Pointer-Template
//*****************************************************************************
typedef struct i2c_ids
{
	// to verify sensor (more information: <lightsensor_handler.h>)
	uint16_t OPT3001_mfct_id;
    uint16_t OPT3001_dev_id;

    //[Debug Purpose]
    //uint16_t OPT3001_univ;	// universal debug pointer for register

} I2C_IDS, *PI2C_IDS;


//*****************************************************************************
// Variable-Declaration
//*****************************************************************************
extern uint32_t g_ui32SysClock; // get clock frequency from main
I2C_IDS OPT3001_IDs;			// init pointer struct

float LuxSensorValue = 0;		// global sensor variable


//*****************************************************************************
// Prototypes
//*****************************************************************************
void sensorOpt3001Setup(void);
void WriteI2CRegister(uint8_t i2cAdress, uint8_t registerName, uint16_t value);
uint16_t ReadI2CRegister(uint8_t i2cAdress, uint8_t registerName);
//void sensorOpt3001Read();


//*****************************************************************************
// I2C & Sensor Setup
//*****************************************************************************
void sensorOpt3001Setup(void)
{
	/* I2C Setup */
	SysCtlPeripheralReset(SYSCTL_PERIPH_I2C2); 		// Reset I2C if already enabled
	SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C2); 	// Enable I2C

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION); 	// SCL:PN5 | SDA:PN4 (Enable)
    //SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOP);	// Booster-Pack 1

	// Wait for GPIO
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION)){
	}

	//I2C Pin Configuration (Booster Pack 1)
	/*
	GPIOPinTypeI2CSCL(GPIO_PORTB_BASE, GPIO_PIN_2);
	GPIOPinTypeI2C(GPIO_PORTB_BASE, GPIO_PIN_3);
	GPIOPinConfigure(GPIO_PB2_I2C0SCL);				// SCL
	GPIOPinConfigure(GPIO_PB3_I2C0SDA);				// SDA
	*/

	//I2C Pin Configuration
    GPIOPinTypeI2CSCL(GPIO_PORTN_BASE, GPIO_PIN_5);
    GPIOPinTypeI2C(GPIO_PORTN_BASE, GPIO_PIN_4);
    GPIOPinConfigure(GPIO_PN5_I2C2SCL);             //SCL
    GPIOPinConfigure(GPIO_PN4_I2C2SDA);             //SDA

	// The conversion time, from start of conversion until the data
    // are ready to be read, is the integration time plus 3 ms.
	SysCtlDelay(3);

	/*-------------------------------------------------------------------------------------------*/

	// Initialize I2C master peripheral, false = 100 kBits/s, true = 400 kBit/s
    I2CMasterInitExpClk(I2C_DEVICE, g_ui32SysClock, false);


	// Sensor Register Configuration: Reset and then set Register to Configuration Register
	WriteI2CRegister(OPT3001_I2C_ADDRESS, REG_CONFIGURATION, CONFIG_RESET);
	WriteI2CRegister(OPT3001_I2C_ADDRESS, REG_CONFIGURATION, CONFIG_ENABLE);


	// [Debug Purpose] Read and Display Register-IDs
    //OPT3001_IDs.OPT3001_mfct_id = ReadI2CRegister(OPT3001_I2C_ADDRESS, REG_MANUFACTURER_ID);
    //OPT3001_IDs.OPT3001_dev_id = ReadI2CRegister(OPT3001_I2C_ADDRESS, REG_DEVICE_ID);
}



//*****************************************************************************
// Read I2C-Register:
// Mostly for reading configuration, if result register has new value
//*****************************************************************************
uint16_t ReadI2CRegister(uint8_t i2cAdress, uint8_t registerName)
{
	uint16_t Return_16bit;

	I2CMasterSlaveAddrSet(I2C_DEVICE, i2cAdress, false);
	I2CMasterDataPut(I2C_DEVICE, registerName);
	I2CMasterControl(I2C_DEVICE, I2C_MASTER_CMD_SINGLE_SEND);
	SysCtlDelay(500);                           	// wait 500 cycles
	while (I2CMasterBusy(I2C_DEVICE)){}        		// wait for I2C not busy

	// Read content (2x8 Bit) of selected register
	I2CMasterSlaveAddrSet(I2C_DEVICE, i2cAdress, true);

	SysCtlDelay(8*(g_ui32SysClock/30)); 			// wait 800ms for conversion

	// Receive first data byte
	I2CMasterControl(I2C_DEVICE, I2C_MASTER_CMD_BURST_RECEIVE_START);
	SysCtlDelay(500);
	while (I2CMasterBusy(I2C_DEVICE)){}

	Return_16bit = I2CMasterDataGet(I2C_DEVICE);	// first data byte (MSB)
	Return_16bit <<= 8;

	// Receive second/last data byte
	I2CMasterControl(I2C_DEVICE, I2C_MASTER_CMD_BURST_RECEIVE_FINISH);
	SysCtlDelay(500);
	while (I2CMasterBusy(I2C_DEVICE)){}

	Return_16bit |= I2CMasterDataGet(I2C_DEVICE);	// second data byte (LSB)

	return Return_16bit;
}


void WriteI2CRegister(uint8_t i2cAdress, uint8_t registerName, uint16_t value)
{
	uint8_t byte[2];
	byte[0] = (uint8_t)((value & 0xFF00) >> 8);
	byte[1] = (value & 0x00FF);

	I2CMasterSlaveAddrSet(I2C_DEVICE, i2cAdress, false); // false = write
	I2CMasterDataPut(I2C_DEVICE, registerName);
	I2CMasterControl(I2C_DEVICE, I2C_MASTER_CMD_BURST_SEND_START);
	SysCtlDelay(500); 									// wait 500 cycles
	while(I2CMasterBusy(I2C_DEVICE)){} 					// wait for I2C not busy

	I2CMasterDataPut(I2C_DEVICE, byte[0]);
	I2CMasterControl(I2C_DEVICE, I2C_MASTER_CMD_BURST_SEND_CONT);
	SysCtlDelay(500);
	while(I2CMasterBusy(I2C_DEVICE)){}

	I2CMasterDataPut(I2C_DEVICE, byte[1]);
	I2CMasterControl(I2C_DEVICE, I2C_MASTER_CMD_BURST_SEND_FINISH);
	SysCtlDelay(500);
	while (I2CMasterBusy(I2C_DEVICE)){}
}


void sensorOpt3001Read()
{
	uint16_t data_rdy, raw_data;
	// exponent | mantissa
	uint16_t e, m;

	// [Debug Purpose]
	//UARTprintf("\n Manufacture-ID: 0x%04x", OPT3001_IDs.OPT3001_mfct_id);
	//UARTprintf("\n Device-ID: 0x%04x", OPT3001_IDs.OPT3001_dev_id);

    // Check conversion ready field if done (BIT = 0)
	data_rdy = ReadI2CRegister(OPT3001_I2C_ADDRESS, REG_CONFIGURATION);

	// check if result register can be read for update
	if(data_rdy & DATA_RDY_BIT){
		raw_data = ReadI2CRegister(OPT3001_I2C_ADDRESS, REG_RESULT);
		// convert raw data
		m = raw_data & 0x0FFF;
		e = (raw_data & 0xF000) >> 12;

		LuxSensorValue = m * (0.01 * exp2(e));
	}// if not, check next time.

	// Conversion read field will reset after reading!

}
