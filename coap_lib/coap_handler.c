//*****************************************************************************
// Authors:     David Nguyen | Michael Stephens
// Group 4:     CoAP Server - Sensor
// Course:		Distributed Systems by Prof. Dr. Boris Boeck
//
// coap_handler.c
//
// License:
// --------
// Copyright (c) 2014-2016 Cesanta Software Limited
// All rights reserved
//
//*****************************************************************************
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "third_party/mongoose.h"

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "driverlib/flash.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "utils/lwiplib.h"
#include "utils/uartstdio.h"
#include "drivers/pinout.h"

#include "CFAF128128B0145T/CFAF128128B0145T.h"

#include "CFAF128128B0145T/CFAF128128B0145T.h"

#include "userlib/io.h"

#include "coap_lib/coap_handler.h"
#include "helper_functions/temperature_handler.h"
#include "helper_functions/lightsensor_handler.h"


//*****************************************************************************
// Global Variables
//*****************************************************************************
uint16_t    localMessageID=0;
extern float LuxSensorValue;

//*****************************************************************************
// Prototypes
//*****************************************************************************
void coapMessage_handler(struct mg_connection *nc, struct mg_coap_message *cm);
static void uartDisplay(struct mg_coap_message *cm);
static void mg_coap_send_by_discover(struct mg_connection *nc, uint16_t msg_id, struct mg_str token);
uint16_t getAcceptFormat(struct mg_coap_message *cm);


//*****************************************************************************
// Event-Handler for CoAP Request
//*****************************************************************************
void coap_handler(struct mg_connection *nc, int ev, void *p) {

	// Polling result going through --> switch case! -> return if so
	if (ev == MG_EV_POLL){
        return;
    }
	struct mg_coap_message *cm = (struct mg_coap_message *) p;
	switch (ev) {
		case MG_EV_COAP_CON: // CoAP CON received
			UARTprintf("\n[CON RECIEVED]");
			// check if request
			coapMessage_handler(nc, cm);
			UARTprintf("\n----------------------------------------");
			break;

		case MG_EV_COAP_NOC:{
			// TODO: No DTLS but can use CON as alternative
			// to "secure" partly a connection without DTLS, NOC can be ignored.
			UARTprintf("\n[NOC RECIEVED]");
			coapMessage_handler(nc, cm);
			UARTprintf("\n----------------------------------------");
			break;
		}

		case MG_EV_COAP_ACK:
		case MG_EV_COAP_RST: {
		  struct mg_coap_message *cm = (struct mg_coap_message *) p;
		  printf("ACK/RST/NOC with msg_id = %d received\n", cm->msg_id);
		  break;
		}
	}
}


//*****************************************************************************
// Process Message
//*****************************************************************************
void coapMessage_handler(struct mg_connection *nc, struct mg_coap_message *cm)
{
	uint32_t res;

	struct mg_coap_message coap_message;
	memset(&coap_message, 0, sizeof(coap_message));


    uartDisplay(cm);

    // Only Request + Get == 1
    if(cm->code_class == MG_COAP_CODECLASS_REQUEST && cm->code_detail == 1){

    	// if discover == .well_known/core
		if(mg_vcmp(&cm->options->value, DISCOVER_PATH) == 0){	// ...like strcmp
			mg_coap_send_by_discover(nc, cm->msg_id, cm->token);

/*[Temperaturesensor]============================================================*/

		}else if(mg_vcmp(&cm->options->value, TMP_BASEPATH) == 0){

			// Create ACK message
			if(cm->msg_type == MG_COAP_MSG_CON){
				coap_message.msg_type   	= MG_COAP_MSG_ACK;
				coap_message.msg_id     	= cm->msg_id;            // MSG-ID == Request MSG-ID
			}else{
				coap_message.msg_type   	= MG_COAP_MSG_NOC;
				coap_message.msg_id			= localMessageID++;
			}

			coap_message.token      		= cm->token;                 // Token == Request Token
			coap_message.code_class 		= MG_COAP_CODECLASS_RESP_OK; // 2.05
			coap_message.code_detail 		= 5;

			char *ctOpt = getAcceptFormat(cm);

			// Add new option 12 (content format) :
			struct mg_coap_add_option *mg_opt = mg_coap_add_option(&coap_message, 12, &ctOpt, 1);

			float vTemp; // Issue with global variable...
			char tempBuffer[200];
			vTemp = getTemperature();

			if(ctOpt == FORMAT_PLAIN){

				sprintf(tempBuffer,"Temperature: %0.2f %cC", vTemp, 176); // ascii for degree

			}else if(ctOpt == FORMAT_JSON){

				sprintf(tempBuffer, "{\n\"Temperature\" : \"%0.2f\" \n}", vTemp);

			}
			coap_message.payload.p 			= &tempBuffer[0];
			coap_message.payload.len		= strlen(&tempBuffer[0]);

			res = mg_coap_send_message(nc, &coap_message);

/*[Lightsensor]=====================================================================*/

		}else if(mg_vcmp(&cm->options->value, LUX_BASEPATH) == 0){

			// Create ACK message
			if(cm->msg_type == MG_COAP_MSG_CON){
				coap_message.msg_type   	= MG_COAP_MSG_ACK;
				coap_message.msg_id     	= cm->msg_id;                   // MSG-ID == Request MSG-ID
			}else{
				coap_message.msg_type   	= MG_COAP_MSG_NOC;
				coap_message.msg_id			= localMessageID++;
			}

			coap_message.token      		= cm->token;                    	// Token == Request Token
			coap_message.code_class 		= MG_COAP_CODECLASS_RESP_OK;    	// 2.05
			coap_message.code_detail 		= 5;

			char *ctOpt = getAcceptFormat(cm);

			// Add new option 12 (content format) :
			struct mg_coap_add_option *mg_opt = mg_coap_add_option(&coap_message, 12, &ctOpt, 1);

			char luxBuffer[200];

			if (ctOpt == FORMAT_PLAIN){

				sprintf(luxBuffer,"Brightness: %5.2f", LuxSensorValue); // brighness value

			}else if (ctOpt == FORMAT_JSON){

				sprintf(luxBuffer, "{\n\"Brightness\" : \"%5.2f\" \n}\0", LuxSensorValue);

			}

			///=============================================================================
			/// LabTask: JSON-Format
			/// - TODO: Add new format for JSON
			/// -
			/// - LuxSensorValue is an extern float from <helper_functions/light_sensor.h>
			/// =============================================================================

			coap_message.payload.p 		= &luxBuffer[0];
			coap_message.payload.len	= strlen(&luxBuffer[0]);

			res = mg_coap_send_message(nc, &coap_message);



		}else{

			// Not Found
			coap_message.msg_type   = MG_COAP_MSG_ACK;
			coap_message.msg_id     = cm->msg_id;
			coap_message.token      = cm->token;
			coap_message.code_class = MG_COAP_CODECLASS_CLIENT_ERR; // 4.04
			coap_message.code_detail = 4;

			res = mg_coap_send_message(nc, &coap_message);

		}


		uartDisplay(&coap_message);
		mg_coap_free_options(&coap_message);

		if (res == 0){
			UARTprintf("[Message Replied]");
		}else{
			UARTprintf("\n[ERROR: %d]\n", res);
		}

    }

}

//*****************************************************************************
// UART Print of CoAP-Message (not all information)
//*****************************************************************************
static void uartDisplay(struct mg_coap_message *cm)
{
	char payloadBuffer[128];

	/* Display CoAP-Message

	Note: not all information are included in header print
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	| Type  |    Code     |          Message ID           |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+*/

	UARTprintf("\n----------------------------------------");

	// Message-Types: Type | Name (not included fot the moment)
    UARTprintf("\n| Type: %d | ", cm->msg_type);

    // Method-Codes X.0Y | X = Code Class, Y = Code Detail
    UARTprintf("Code: %d.0%d | ", cm->code_class, cm->code_detail);

    // Message ID
    UARTprintf("MSG-ID: %d |", cm->msg_id);
	UARTprintf("\n----------------------------------------\r\n");

    /* Payload CoAP-Message */
    UARTprintf("\nPayload:");

    memcpy(&payloadBuffer[0], cm->payload.p, cm->payload.len);
    payloadBuffer[cm->payload.len] = '\0';

    UARTprintf("%s\n", &payloadBuffer[0]);

    return;
}


//*********************************************************************************
// Discover Response (outsourcing for better overview only, it was not neccessary)
//*********************************************************************************
static void mg_coap_send_by_discover(struct mg_connection *nc, uint16_t msg_id, struct mg_str token){

	uint32_t res;
	struct mg_coap_message coap_message;

	memset(&coap_message, 0, sizeof(coap_message));				// free options at the end!

	// Create ACK message
	coap_message.msg_type   	= MG_COAP_MSG_ACK;
	coap_message.msg_id     	= msg_id;                   	// MSG-ID == Request MSG-ID
	coap_message.token      	= token;                    	// Token == Request Token
	coap_message.code_class 	= MG_COAP_CODECLASS_RESP_OK;    // 2.05
	coap_message.code_detail 	= 5;

	// Content-Formats = application/link-format | 40
	char *ctOpt = FORMAT_LINK;

    // Add new option 12 (content format) :
    struct mg_coap_add_option *mg_opt = mg_coap_add_option(&coap_message, 12, &ctOpt, 1);

    /* ====================================================================
     * Content of Discover
     *
     * <sensor>
     * - rt 	= resource type
     * - title	= title
     *=====================================================================*/
    char* cont_text =

			"</temperature>;"
    		    "title=\"Temperature in the system\";"
    			"rt=\"temperature-c\";"

    		"</light>;"
    			"title=\"luminous value\";"
    			"rt=\"light-lux\"";

    coap_message.payload.p 		= cont_text;
    coap_message.payload.len	= strlen(cont_text);

	res = mg_coap_send_message(nc, &coap_message);

	uartDisplay(&coap_message);
    mg_coap_free_options(&coap_message);

	if (res == 0){
		UARTprintf("[Message Replied]");
	}else{
		UARTprintf("\n[ERROR: %d]\n", res);
	}
    return;
}


///*****************************************************************************************
/// Accept-Handler for LabTask
/// TODO: set the correct Macro for JSON
///
/// - FORMAT_DUMMY has value 99 <helper_functions/coap_handler.h>, fill the correct value
/// -
///*****************************************************************************************
uint16_t getAcceptFormat(struct mg_coap_message *cm){

	struct mg_coap_option *opt = (struct mg_coap_option *) cm->options->next;
	struct mg_str val = (struct mg_str) opt->value;

	if(opt->number == OPTION_ACCEPT){					// Option 17 (for client) format request
		///******************************
		if(*(val.p) == FORMAT_JSON){
			return FORMAT_JSON;
		///******************************
		}else if(*(val.p) == FORMAT_PLAIN){				// Format 0
			return FORMAT_PLAIN;
		}
	}else if (opt->number == OPTION_CONTENT_FORMAT){	// Option 12 for payload

		if(*(val.p) == FORMAT_JSON){
			return FORMAT_JSON;

		}else if(*(val.p) == FORMAT_PLAIN){				// Format 0

			return FORMAT_PLAIN;
		}
	}
	// No Accept Format
	return NULL;
}



