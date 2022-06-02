//*****************************************************************************
// Authors:     David Nguyen | Michael Stephens
// Group 4:     CoAP Server - Sensor
// Course:		Distributed Systems by Prof. Dr. Boris Boeck
//
// temperature_handler.h
//
//*****************************************************************************

#ifndef HELPER_FUNCTIONS_TEMPERATURE_HANDLER_H_
#define HELPER_FUNCTIONS_TEMPERATURE_HANDLER_H_

#ifdef __cplusplus
extern "C"
{
#endif

int sensorTmp006Setup(void);

void getTemp(char *getTemp);



#ifdef __cplusplus
}
#endif

#endif /* HELPER_FUNCTIONS_TEMPERATURE_HANDLER_H_ */
