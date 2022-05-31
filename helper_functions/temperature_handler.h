/*
 * temperature_handler.h
 *
 *  Created on: 28.05.2022
 *      Author: David Nguyen
 */

#ifndef HELPER_FUNCTIONS_TEMPERATURE_HANDLER_H_
#define HELPER_FUNCTIONS_TEMPERATURE_HANDLER_H_

#ifdef __cplusplus
extern "C"
{
#endif

int setupTempHardware(void);

void getTemp(char *getTemp);



#ifdef __cplusplus
}
#endif

#endif /* HELPER_FUNCTIONS_TEMPERATURE_HANDLER_H_ */
