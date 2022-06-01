

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

#include "third_party/coap_handler.h"
#include "helper_functions/temperature_handler.h"
#include "helper_functions/lightsensor_handler.h"

// TODO: aendern

const char *coapTypeStrings[] = {"CON", "NON", "ACK", "RST"};

const char *coapMethodCodeStrings[]             = {"Empty  ", "GET    ", "POST   ", "PUT    ", "DELETE "};
const char *coapResponseCodeSuccessStrings[]    = {"       ", "Created", "Deleted", "Valid  ", "Changed", "Content"};
const char *coapResponseCodeClientErrStrings[]  = {"Bad Req", "Unautho", "Bad opt", "Forbidd", "Not fnd", "       "};
const char *coapResponseCodeServerErrStrings[]  = {"ServErr", "ServErr", "ServErr", "ServErr", "ServErr", "ServErr"};



// Global Variables

uint16_t    localMessageID=0;
char *pBuffer;



// Static funtions
// Prototypes (static functions)
void coapMessage_handler(struct mg_connection *nc, struct mg_coap_message *cm);

static void coapPrintMessageDetails(struct mg_coap_message *cm);

// Get URI Path from coap message structure
// Combine URI-Path options to a full URI-Path
void getURIPath(char *pURIPath, struct mg_coap_message *cm);

const char *getCodeStr(struct mg_coap_message *cm);







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
        if(pOption->number == COAP_OPTION_URIPATH){
            memcpy(pURIPath, pOption->value.p, pOption->value.len);
            pURIPath = pURIPath + pOption->value.len;
            *pURIPath= '/';
            pURIPath++;
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




void coapMessage_handler(struct mg_connection *nc, struct mg_coap_message *cm)
{
    uint32_t                res;
    struct mg_coap_message  coap_message;       // New message (for reply)
    char                    err = 0;
    char                    URIPath[50];
    struct mg_str           load_buffer;          // (string) buffer (pointer to memory + length)
    char                    payload_buffer[MAX_PAYLOAD_SIZE];
    char                    content_format_option = 0;         // plain text

    coapPrintMessageDetails(cm);

    // Analyze message
    // ~~~~~~~~~~~~~~~
    if(cm->code_class == MG_COAP_CODECLASS_REQUEST){
        // Request Message
        // ***************

        memset(&coap_message, 0, sizeof(coap_message));         // clear coap_message buffer (for reply/ACK)

        // Extract full URI-Path into tmp buffer URIPath
        getURIPath(URIPath, cm);




        if(cm->code_detail == COAP_CODEDETAIL_GET){
            // GET request
            // ***********

            if(strcmp(URIPath, COAP_QUERY_WELLKNOWNCORE) == 0){
                // .WellKnown/core
                content_format_option = 0x28;        // 0x28 : application/link-format = 2;

                //                       1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890
                //sprintf(payload_buffer, "</temperature>;rt=\"temperature-c\";title =\"Temperature in the system\";</test>;title=\"name\"");
                sprintf(payload_buffer, "</temperature>;rt=\"temperature-c\";title=\"Temperature in the system\";</light>;title=\"light\";</lux>;title =\"lux\"");
                sprintf(pBuffer, "</temperature>;rt=\"temperature-c\";title=\"Temperature in the system\";</light>;title=\"light\";</lux>;title =\"lux\"");
                //sprintf(payload_buffer, "</light>;title =\"light\"");


            }else if(strcmp(URIPath, COAP_QUERY_TEMP) == 0){
                //uint32_t vTemp;
                float vTemp;

                vTemp = getTemperature();

                //getTemp(vTemp);

                content_format_option = 0x28;
                sprintf(payload_buffer, "Temperatur: %0.2f °C", vTemp);

            }else if(strcmp(URIPath, COAP_QUERY_LUX) == 0){

				float    readLux;

				//Read and convert OPT values
				sensorOpt3001Read(&readLux);

				content_format_option = 0x28;
				sprintf(payload_buffer, "Light: %5.2f", readLux);

            }

            else{
                err = 1;        // Bad option
            }


            if(err == 0){
                // OK

                // CON or NON request?
                if(cm->msg_type == MG_COAP_MSG_CON){
                    // CON
                    // Create ACK message with piggyback response data
                    coap_message.msg_type   = MG_COAP_MSG_ACK;
                    coap_message.msg_id     = cm->msg_id;               // the message id of the ACK is the same as the message that it answers to
                    coap_message.token      = cm->token;                // the token of the ACK is the same as the message that it answers to

                }
                else{
                    // NON
                    coap_message.msg_type   = MG_COAP_MSG_NOC;
                    coap_message.msg_id     = localMessageID++;         // the message id is the local one
                    coap_message.token      = cm->token;                // the token of the ACK is the same as the message that it answers to
                }

                coap_message.code_class  = MG_COAP_CODECLASS_RESP_OK;   //  2.05
                coap_message.code_detail = COAP_CODEDETAIL_CONTENT;     //

                load_buffer.p   = &payload_buffer[0];
                load_buffer.len = strlen(&payload_buffer[0]);
                coap_message.payload    = load_buffer;
                load_buffer.len = strlen(&pBuffer[0]);
                coap_message.payload    = load_buffer;

                // Add Option content format, if required
                //if(content_format_option != 0){
                    // Set option
                    // Add option 12 (content format) :
                    mg_coap_add_option(&coap_message, 12, &content_format_option, 1);
                //}


                res = mg_coap_send_message(nc, &coap_message);          // send message
                coapPrintMessageDetails(&coap_message);


                //if(content_format_option != 0){
                    // Frees the memory allocated for options. If the cm parameter doesn't contain any option it does nothing.
                    mg_coap_free_options(&coap_message);
                //}

            }
            else{
                // Not found

                coap_message.msg_type   = MG_COAP_MSG_ACK;
                coap_message.msg_id     = cm->msg_id;                   // the message id of the ACK is the same as the message that it answers to
                coap_message.token      = cm->token;                    // the token of the ACK is the same as the message that it answers to
                coap_message.code_class = MG_COAP_CODECLASS_CLIENT_ERR; // Not found 4.04
                coap_message.code_detail = 4;

                res = mg_coap_send_message(nc, &coap_message);          // send message
                coapPrintMessageDetails(&coap_message);

            }



        }
        else if((cm->code_detail == COAP_CODEDETAIL_PUT) || (cm->code_detail == COAP_CODEDETAIL_POST)){
            // PUT request or POST request
            // ****************************

            if(strcmp(URIPath, COAP_QUERY_LIGHT) == 0){

                // Evaluate payload
                if(memcmp("on", cm->payload.p, cm->payload.len) == 0){
                    // "on"
                    io_ledSetState(1);
                }
                else{
                    // other
                    io_ledSetState(0);
                }

                if(cm->msg_type == MG_COAP_MSG_CON){
                    // CON
                    // Create ACK message
                    coap_message.msg_type   = MG_COAP_MSG_ACK;
                    coap_message.msg_id     = cm->msg_id;                   // the message id of the ACK is the same as the message that it answers to
                    coap_message.token      = cm->token;                    // the token of the ACK is the same as the message that it answers to
                    coap_message.code_class = MG_COAP_CODECLASS_RESP_OK;    // changed 2.04
                    coap_message.code_detail = 4;                           //

                    res = mg_coap_send_message(nc, &coap_message);          // send message
                    coapPrintMessageDetails(&coap_message);

                }
            }
            else{
                // Bad option

                coap_message.msg_type   = MG_COAP_MSG_ACK;
                coap_message.msg_id     = cm->msg_id;                   // the message id of the ACK is the same as the message that it answers to
                coap_message.token      = cm->token;                    // the token of the ACK is the same as the message that it answers to
                coap_message.code_class = MG_COAP_CODECLASS_CLIENT_ERR; // Not found 4.04
                coap_message.code_detail = 4;

                res = mg_coap_send_message(nc, &coap_message);          // send message
                coapPrintMessageDetails(&coap_message);

            }


        }


        if (res == 0) {
            UARTprintf("CoAP reply sent\n");
        }
        else {
            UARTprintf("  Error: %i\n", res);
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



	/* [Header]
	-----------------------------------------------------------------
	| Message-Types: Type | Name (not included fot the moment)		|
	-----------------------------------------------------------------*/
    UARTprintf("\n\n[Header] | %d | ", cm->msg_type);
    // Response-Codes X.YY - X = Class, YY = Details
    UARTprintf("%d.0%d | ", cm->code_class, cm->code_detail);
    // Message ID
    UARTprintf("%d |", cm->msg_id);


    /* Payload */
    UARTprintf("\nPayload: ");

    // store payload
    if (cm->payload.p != NULL)
    {
    	// create pointer for payload with size of payload
    	pBuffer = calloc(cm->payload.len + 1, sizeof(char));
    	memcpy(pBuffer,cm->payload.p,cm->payload.len);
    }
    // set Null for terminate array for printing payload
    UARTprintf("%s\n", &pBuffer[0]);

    UARTprintf("  Options:   ");
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







