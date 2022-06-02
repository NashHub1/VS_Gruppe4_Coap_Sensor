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

#include "userlib/io.h"

#include "coap_lib/coap_handler.h"
#include "helper_functions/temperature_handler.h"
#include "helper_functions/lightsensor_handler.h"


//const char *DISCOVER_PATH[] = {".well-known/core"};

// Global Variables

uint16_t    localMessageID=0;




// Static funtions
// Prototypes (static functions)
void coapMessage_handler(struct mg_connection *nc, struct mg_coap_message *cm);

static void printCoapMSG(struct mg_coap_message *cm);

// Get URI Path from coap message structure
// Combine URI-Path options to a full URI-Path
void getURIPath(char *pURIPath, struct mg_coap_message *cm);

const char *getCodeStr(struct mg_coap_message *cm);

static void mg_coap_send_by_discover(struct mg_connection *nc, uint16_t msg_id, struct mg_str token);
static void  mg_coap_send_by_temperature(struct mg_connection *nc, uint16_t msg_id, struct mg_str token, uint8_t msg_type, uint8_t detail);
static void mg_coap_send_by_lux(struct mg_connection *nc, uint16_t msg_id, struct mg_str token, uint8_t msg_type, uint8_t detail);


int err = 0;


//*****************************************************************************
// Event-Handler for CoAP Request
//*****************************************************************************
void coap_handler(struct mg_connection *nc, int ev, void *p) {

	// TODO: polling result going through --> switch case! return? blocked?
	// TODO: how to secure connection? dtls??? - alternatively with COAP_CON?

	struct mg_coap_message *cm = (struct mg_coap_message *) p;
	switch (ev) {
		case MG_EV_COAP_CON:       			// CoAP CON received
			UARTprintf("\n[CON RECIEVED]");	//, cm->msg_id);
			// check if request
			coapMessage_handler(nc, cm);
			if (err == 0){
				UARTprintf("\n[CON REPLIED]");
			}else{
				err = 0;
			}
			UARTprintf("\n----------------------------------------");
			break;

		case MG_EV_COAP_NOC:{
			UARTprintf("\n[NOC RECIEVED]"); //, cm->msg_id);
			coapMessage_handler(nc, cm);
			if (err == 0){
				UARTprintf("\n[NOC REPLIED]");
			}else{
				err = 0;
			}
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

    printCoapMSG(cm);

    // If Request, check only after UriPath (and detail)
    if(cm->code_class == MG_COAP_CODECLASS_REQUEST){

    	// clear coap_message buffer (for reply/ACK)
        //memset(&coap_message, 0, sizeof(coap_message));


        /* sensor ... */
		// if No. 11 = UriPath
		if(mg_vcmp(&cm->options->value, DISCOVER_PATH) == 0){	// ...like strcmp
			mg_coap_send_by_discover(nc, cm->msg_id, cm->token);

		}else if(mg_vcmp(&cm->options->value, TMP_BASEPATH) == 0){

			mg_coap_send_by_temperature(nc, cm->msg_id, cm->token, cm->msg_type, cm->code_detail);

		}else if(mg_vcmp(&cm->options->value, LUX_BASEPATH) == 0){

			mg_coap_send_by_lux(nc, cm->msg_id, cm->token, cm->msg_type, cm->code_detail);
		}else{
			// Not Found
			uint32_t res;
			struct mg_coap_message  coap_message;

			coap_message.msg_type   = MG_COAP_MSG_ACK;
			coap_message.msg_id     = cm->msg_id;
			coap_message.token      = cm->token;
			coap_message.code_class = MG_COAP_CODECLASS_CLIENT_ERR; // 4.04
			coap_message.code_detail = 4;

			res = mg_coap_send_message(nc, &coap_message);
			if (res == 0){
				printCoapMSG(&coap_message);
				UARTprintf("\n[ERR REPLIED]\n");
			}else{
				UARTprintf("\n[ERROR: %d]\n", res);
			}
			err = 1;
			printCoapMSG(&coap_message);
			mg_coap_free_options(&coap_message);

		}


    }

}

static void printCoapMSG(struct mg_coap_message *cm)
{
	char payloadBuffer[128];

	 /* Display CoAP-Message */

	/* Note: Version and Token not included!
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|Ver| T |  TKL  |      Code     |          Message ID           |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+*/

	UARTprintf("\n----------------------------------------");
	// Message-Types: Type | Name (not included fot the moment)
    UARTprintf("\n| Type: %d | ", cm->msg_type);
    // Method-Codes X.0Y | X = Code Class, Y = Code Detail
    UARTprintf("Code: %d.0%d | ", cm->code_class, cm->code_detail);
    // Message ID
    UARTprintf("MSG-ID: %d |", cm->msg_id);
	UARTprintf("\n----------------------------------------");

	/*
	if(cm->token.len > 0){
		UARTprintf("0x");
        for(i=0; i<cm->token.len; i++){
            value = *((uint8_t *)(cm->token.p)+i);
            sprintf(&localStr[2*i], "%2.2x", value);
        }
        UARTprintf("%s",localStr);
    }*/

    UARTprintf("\r\n");

    /* Payload */
    UARTprintf("\nPayload:");

    memcpy(&payloadBuffer[0], cm->payload.p, cm->payload.len);
    payloadBuffer[cm->payload.len] = '\0';

    UARTprintf("%s\n", &payloadBuffer[0]);

    return;
}


// Discover ACK-Messager
// Template: uint32_t mg_coap_send_message() + msg_id + token
static void mg_coap_send_by_discover(struct mg_connection *nc, uint16_t msg_id, struct mg_str token){

	struct mg_coap_message coap_message;
	uint32_t res;
	memset(&coap_message, 0, sizeof(coap_message));				// free options at the end!

	// Create ACK message
	coap_message.msg_type   	= MG_COAP_MSG_ACK;
	coap_message.msg_id     	= msg_id;                   	// MSG-ID == Request MSG-ID
	coap_message.token      	= token;                    	// Token == Request Token
	coap_message.code_class 	= MG_COAP_CODECLASS_RESP_OK;    // 2.05
	coap_message.code_detail 	= 5;

	// Content-Formats = application/link-format | 40
	char *ctOpt = 40;

    // Add new option 12 (content format) :
    struct mg_coap_add_option *mg_opt = mg_coap_add_option(&coap_message, 12, &ctOpt, 1);

    /* --------------------------------------------------------------------
     * Content of Discover
     *
     * <path_tree>
     * - rt = resource type
     * - title
     *--------------------------------------------------------------------*/
    char* cont_text =
/*
    		"</temperature>;"
    			"rt=\"temperature-c\";"
    			"title=\"Temperature in the system\";"

    		"</light>;"
    			"title=\"light\";"

    		"</lux>;"
    			"title =\"lux\"";
    */
			"</temperature>;"
    		    "title=\"Temperature in the system\";"
    			"rt=\"temperature-c\";"

    		"</light>;"
    			"title=\"luminous value\";"
    			"rt=\"light-lux\"";
    		/*
    		"</lux>;"
    			"title =\"lux\"";*/

    coap_message.payload.p 		= cont_text;
    coap_message.payload.len	= strlen(cont_text);

	res = mg_coap_send_message(nc, &coap_message);

	if (res != 0){
		err = 1;
		UARTprintf("\n[ERROR: %d]\n", res);
	}
	printCoapMSG(&coap_message);
    mg_coap_free_options(&coap_message);
    return;
}

static void  mg_coap_send_by_temperature(struct mg_connection *nc, uint16_t msg_id, struct mg_str token, uint8_t msg_type, uint8_t detail){

	struct mg_coap_message coap_message;
	uint32_t res;
	memset(&coap_message, 0, sizeof(coap_message));				// free options at the end!


	// Create ACK message
	if(msg_type == MG_COAP_MSG_CON){
		coap_message.msg_type   	= MG_COAP_MSG_ACK;
		coap_message.msg_id     	= msg_id;                   	// MSG-ID == Request MSG-ID
	}else{
		coap_message.msg_type   	= MG_COAP_MSG_NOC;
		coap_message.msg_id			= localMessageID++;
	}

	coap_message.token      	= token;                    	// Token == Request Token
	coap_message.code_class 	= MG_COAP_CODECLASS_RESP_OK;    // 2.05

	coap_message.code_detail 	= detail;

	// Content-Formats = TODO: extra methode
	char *ctOpt = 0;

    // Add new option 12 (content format) :
    struct mg_coap_add_option *mg_opt = mg_coap_add_option(&coap_message, 12, &ctOpt, 1);

    if (coap_message.code_detail == 1){
    	//uint32_t vTemp;

		float vTemp;
		char tempBuffer[200];

		vTemp = getTemperature();
		sprintf(tempBuffer,"Temperatur: %0.2f °C", vTemp);
		coap_message.payload.p 			= &tempBuffer[0];
		coap_message.payload.len		= strlen(&tempBuffer[0]);

    }

	res = mg_coap_send_message(nc, &coap_message);

	if (res != 0){
		err = 1;
		UARTprintf("\n[ERROR: %d]\n", res);
	}
	printCoapMSG(&coap_message);
    mg_coap_free_options(&coap_message);
    return;
}


static void mg_coap_send_by_lux(struct mg_connection *nc, uint16_t msg_id, struct mg_str token, uint8_t msg_type, uint8_t detail){

	struct mg_coap_message coap_message;
	uint32_t res;
	memset(&coap_message, 0, sizeof(coap_message));				// free options at the end!

	// Create ACK message
	if(msg_type == MG_COAP_MSG_CON){
		coap_message.msg_type   	= MG_COAP_MSG_ACK;
		coap_message.msg_id     	= msg_id;                   	// MSG-ID == Request MSG-ID
	}else{
		coap_message.msg_type   	= MG_COAP_MSG_NOC;
		coap_message.msg_id			= localMessageID++;
	}
	coap_message.token      	= token;                    	// Token == Request Token
	coap_message.code_class 	= MG_COAP_CODECLASS_RESP_OK;    // 2.05
	coap_message.code_detail 	= detail; // abruf

	// Content-Formats = TODO: extra methode
	char *ctOpt = 0;

    // Add new option 12 (content format) :
    struct mg_coap_add_option *mg_opt = mg_coap_add_option(&coap_message, 12, &ctOpt, 1);

    if (coap_message.code_detail == 1){

		float readLux;
		char luxBuffer[200];

		//Read and convert OPT values
		sensorOpt3001Read(&readLux);

		//content_format_option = 0x28;
		sprintf(luxBuffer,"Light: %5.2f", readLux);
		coap_message.payload.p 		= &luxBuffer[0];
		coap_message.payload.len	= strlen(&luxBuffer[0]);
    }

	res = mg_coap_send_message(nc, &coap_message);

	if (res != 0){
		err = 1;
		UARTprintf("\n[ERROR: %d]\n", res);
	}

    mg_coap_free_options(&coap_message);
    return;
}





