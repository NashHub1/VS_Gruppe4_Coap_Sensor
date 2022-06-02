/*
 * lightsensor_handler.h
 *
 *  Created on: 30.05.2022
 *      Author: David Nguyen
 */

#ifndef HELPER_FUNCTIONS_LIGHTSENSOR_HANDLER_H_
#define HELPER_FUNCTIONS_LIGHTSENSOR_HANDLER_H_

//*****************************************************************************
// Macro Defines for opt3001 bit fields
//*****************************************************************************

/* Slave address */
#define OPT3001_I2C_ADDRESS				0x44		// Datasheet P.22
#define I2C_DEVICE              		I2C2_BASE	// I2C0..C4

/* Register addresses */
#define REG_RESULT                      0x00		// 16Bit-Register
#define REG_CONFIGURATION               0x01
#define REG_LOW_LIMIT                   0x02
#define REG_HIGH_LIMIT                  0x03
/* Identify the device */
#define REG_MANUFACTURER_ID             0x7E		// Manufacturer ID: 5449h
#define REG_DEVICE_ID                   0x7F		// Device ID: 		3001h

/* Register values */
#define CONFIG_RESET                    0xC810
#define CONFIG_ENABLE                   0xCC10		// Scale: 40.95 | Continous... C010 = 100ms (issue)

/* Bit values */
#define DATA_RDY_BIT                    0x0080  	// Data ready


//*****************************************************************************
// Event-Handler
//*****************************************************************************

// Initialize Configuration for OPT3001 Sensor
void sensorOpt3001Setup(void);

// I2C - Event Handler
void WriteI2CRegister(uint8_t i2cAdress, uint8_t registerName, uint16_t value);
uint16_t ReadI2CRegister(uint8_t i2cAdress, uint8_t registerName);

// Get-Sensor-Data
void sensorOpt3001Read();

#endif /* HELPER_FUNCTIONS_LIGHTSENSOR_HANDLER_H_ */
