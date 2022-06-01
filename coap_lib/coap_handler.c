

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "third_party/mongoose.h"
#include "third_party/mjson/mjson.h"

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

// TODO: aendern

const char *coapTypeStrings[] = {"CON", "NON", "ACK", "RST"};

const char *coapMethodCodeStrings[]             = {"Empty  ", "GET    ", "POST   ", "PUT    ", "DELETE "};
const char *coapResponseCodeSuccessStrings[]    = {"       ", "Created", "Deleted", "Valid  ", "Changed", "Content"};
const char *coapResponseCodeClientErrStrings[]  = {"Bad Req", "Unautho", "Bad opt", "Forbidd", "Not fnd", "       "};
const char *coapResponseCodeServerErrStrings[]  = {"ServErr", "ServErr", "ServErr", "ServErr", "ServErr", "ServErr"};

//const char *DISCOVER_PATH[] = {".well-known/core"};

// Global Variables

uint16_t    localMessageID=0;




// Static funtions
// Prototypes (static functions)
void coapMessage_handler(struct mg_connection *nc, struct mg_coap_message *cm);

static void coapPrintMessageDetails(struct mg_coap_message *cm);

// Get URI Path from coap message structure
// Combine URI-Path options to a full URI-Path
void getURIPath(char *pURIPath, struct mg_coap_message *cm);

const char *getCodeStr(struct mg_coap_message *cm);

static void mg_coap_send_by_discover(struct mg_connection *nc, uint16_t msg_id, struct mg_str token);
static void  mg_coap_send_by_temperature(struct mg_connection *nc, uint16_t msg_id, struct mg_str token, uint8_t msg_type, uint8_t detail);
static void mg_coap_send_by_lux(struct mg_connection *nc, uint16_t msg_id, struct mg_str token, uint8_t msg_type, uint8_t detail);





//*****************************************************************************
// Event-Handler for CoAP Request
//*****************************************************************************
void coap_handler(struct mg_connection *nc, int ev, void *p) {

	// TODO: polling result going through --> switch case! return? blocked?
	// TODO: how to secure connection? dtls??? - alternatively with COAP_CON?

	struct mg_coap_message  *cm = (struct mg_coap_message *) p;
	switch (ev) {
		case MG_EV_COAP_CON:       // CoAP CON received
			UARTprintf("CON received\n");//, cm->msg_id);
			// check if request
			coapMessage_handler(nc, cm);
			break;

		case MG_EV_COAP_NOC:{
			UARTprintf("NOC received\n"); //, cm->msg_id);
			coapMessage_handler(nc, cm);
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


// Static functions
// ***********************************************************************************************

// Combine URI-Path options to a full URI-Path
void getURIPath(char *pURIPath, struct mg_coap_message *cm)
{
struct mg_coap_option *pOption;

    pOption = cm->options;
    while(pOption != NULL){
        if(pOption->number == COAP_OPTION_URIPATH){ // 11 |
            memcpy(pURIPath, pOption->value.p, pOption->value.len);
            pURIPath = pURIPath + pOption->value.len; // ++ len  | space of value | offset <-- now
            *pURIPath= '/'; // | space of value | 47
            pURIPath++;	// 48
        }
        pOption = pOption->next;
    }
    pURIPath--;
    *pURIPath= '\0';

}

const char *getCodeStr(struct mg_coap_message *cm)
{
    if(cm->code_class == MG_COAP_CODECLASS_REQUEST){
        return(coapMethodCodeStrings[cm->code_detail]);
    }
    else if(cm->code_class == MG_COAP_CODECLASS_RESP_OK){
        return(coapResponseCodeSuccessStrings[cm->code_detail]);
    }
    else if(cm->code_class == MG_COAP_CODECLASS_CLIENT_ERR){
        return(coapResponseCodeClientErrStrings[cm->code_detail]);
    }
    else if(cm->code_class == MG_COAP_CODECLASS_SRV_ERR){
        return(coapResponseCodeServerErrStrings[cm->code_detail]);
    }
}


//*****************************************************************************
// Process Message
//*****************************************************************************
void coapMessage_handler(struct mg_connection *nc, struct mg_coap_message *cm)
{
    uint32_t                res;
    struct mg_coap_message  coap_message;       // New message (for reply)
    char                    URIPath[50];

    coapPrintMessageDetails(cm);

    // Analyze message
    // If Request, check only after UriPath
    if(cm->code_class == MG_COAP_CODECLASS_REQUEST){

    	// clear coap_message buffer (for reply/ACK)
        //memset(&coap_message, 0, sizeof(coap_message));

        // Extract full URI-Path into tmp buffer URIPath
        getURIPath(URIPath, cm);


        /* sensor ... */
		// if No. 11 = UriPath
		if(strcmp(URIPath, DISCOVER_PATH) == 0){
			mg_coap_send_by_discover(nc, cm->msg_id, cm->token);

		}else if(strcmp(URIPath, TMP_BASEPATH) == 0){

			mg_coap_send_by_temperature(nc, cm->msg_id, cm->token, cm->msg_type, cm->code_detail);

		}else if(strcmp(URIPath, LUX_BASEPATH) == 0){

			mg_coap_send_by_lux(nc, cm->msg_id, cm->token, cm->msg_type, cm->code_detail);
		}else{

			coap_message.msg_type   = MG_COAP_MSG_ACK;
			coap_message.msg_id     = cm->msg_id;                   // the message id of the ACK is the same as the message that it answers to
			coap_message.token      = cm->token;                    // the token of the ACK is the same as the message that it answers to
			coap_message.code_class = MG_COAP_CODECLASS_CLIENT_ERR; // Not found 4.04
			coap_message.code_detail = 4;

			res = mg_coap_send_message(nc, &coap_message);          // send message
			coapPrintMessageDetails(&coap_message);

		}


    }

}





static void coapPrintMessageDetails(struct mg_coap_message *cm)
{
	char                  localStr[MAX_PAYLOAD_SIZE];
	int                   value;
	struct mg_coap_option *pOption;
	int                     i;

	 /* Display CoAP-Message */

	/* Note: Version and Token not included!
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|Ver| T |  TKL  |      Code     |          Message ID           |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+*/

	UARTprintf("\n\n--------------------");
	// Message-Types: Type | Name (not included fot the moment)
    UARTprintf("\n| %d | ", cm->msg_type);
    // Method-Codes X.0Y | X = Code Class, Y = Code Detail
    UARTprintf("%d.0%d | ", cm->code_class, cm->code_detail);
    // Message ID
    UARTprintf("%d |", cm->msg_id);
	UARTprintf("\n--------------------");

	if(cm->token.len > 0){
		UARTprintf("0x");
        for(i=0; i<cm->token.len; i++){
            value = *((uint8_t *)(cm->token.p)+i);
            sprintf(&localStr[2*i], "%2.2x", value);
        }
        UARTprintf("%s",localStr);
    }

    UARTprintf("\r\n");

    /* Payload */
    UARTprintf("\nPayload:");
    memcpy(&localStr[0], cm->payload.p, cm->payload.len);
    localStr[cm->payload.len] = '\0';
    UARTprintf("  %s\n\r",   &localStr[0]);


    UARTprintf("Options:   ");
    pOption = cm->options;
    while(pOption != NULL){

        UARTprintf("%2d: ",pOption->number);
        switch(pOption->number){

            case COAP_OPTION_URIPATH:    // URI-Path
                UARTprintf("URI-Path:      ");
                memcpy(&localStr[0], pOption->value.p, pOption->value.len);
                localStr[pOption->value.len] = '\0';
                UARTprintf("%s\n\r", &localStr[0]);
                break;

            case COAP_OPTION_CONTENTFORMAT:    // Content format
                UARTprintf("Content format:");
                if(pOption->value.len == 0){
                    value = 0;
                }
                else if(pOption->value.len == 1){
                    value = *(uint8_t *)pOption->value.p;
                }
                else{
                    value = *(uint16_t *)pOption->value.p;
                }
                UARTprintf("%d\n\r", value);
                break;


            case COAP_OPTION_ACCEPT:            // Accept
                UARTprintf("Accept:        ");
                if(pOption->value.len == 0){
                    value = 0;
                }
                else if(pOption->value.len == 1){
                    value = *(uint8_t *)pOption->value.p;
                }
                else{
                    value = *(uint16_t *)pOption->value.p;
                }
                UARTprintf("%d\n\r", value);
                break;

            case COAP_OPTION_ETAG:              // ETAG
                UARTprintf("ETAG:          ");
                if(pOption->value.len > 0){
                    UARTprintf("0x");
                    for(i=0; i<pOption->value.len; i++){
                        value = *((uint8_t *)(pOption->value.p)+i);
                        sprintf(&localStr[2*i], "%2.2x", value);
                    }
                    UARTprintf("%s",localStr);
                }
                UARTprintf("\n\r");
                break;

            default:
                UARTprintf("\n\r");
                break;
        }

        pOption = pOption->next;
        UARTprintf("             ");
    }


    UARTprintf("\n");


    // Forward message details to system for visualization in the display
    // ******************************************************************
    // To do

    return;
}


// Discover ACK-Messager
// Template: uint32_t mg_coap_send_message() + msg_id + token
static void mg_coap_send_by_discover(struct mg_connection *nc, uint16_t msg_id, struct mg_str token){

	struct mg_coap_message coap_message;
	uint32_t res;
	memset(&coap_message, 0, sizeof(coap_message));				// free options at the end!
	UARTprintf("hier bin ich");
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

    		"</temperature>;"
    			"rt=\"temperature-c\";"
    			"title=\"Temperature in the system\";"

    		"</light>;"
    			"title=\"light\";"

    		"</lux>;"
    			"title =\"lux\"";

    coap_message.payload.p 		= cont_text;
    coap_message.payload.len	= strlen(cont_text);

	res = mg_coap_send_message(nc, &coap_message);

	if (res == 0){
		//CODE
		UARTprintf("\nGut");
	}

    mg_coap_free_options(&coap_message);
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


	/*

			coap_message.code_class  = MG_COAP_CODECLASS_RESP_OK;   //  2.05
			coap_message.code_detail = COAP_CODEDETAIL_CONTENT;     //

			load_buffer.p   = &payload_buffer[0];
			load_buffer.len = strlen(&payload_buffer[0]);
			coap_message.payload    = load_buffer;
*/



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

	if (res == 0){
		//CODE
		UARTprintf("\nGut");
	}

    mg_coap_free_options(&coap_message);

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

	if (res == 0){
		//CODE
		UARTprintf("\nGut");
	}

    mg_coap_free_options(&coap_message);

}





