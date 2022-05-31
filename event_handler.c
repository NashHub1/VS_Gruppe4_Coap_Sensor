/*
 * event_handler.c
 *
 *  Created on: 26.11.2020
 *      Author: bboeck
 */




#include <event_handler.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "third_party/mongoose/mongoose.h"
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



#include "CFAF128128B0145T/CFAF128128B0145T.h"      // Display

#include "userlib/io.h"


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




// Static funtions
// Prototypes (static functions)
void coapProcessMessage(struct mg_connection *nc, struct mg_coap_message *cm);

static void coapPrintMessageDetails(struct mg_coap_message *cm);

// Get URI Path from coap message structure
// Combine URI-Path options to a full URI-Path
void getURIPath(char *pURIPath, struct mg_coap_message *cm);

const char *getCodeStr(struct mg_coap_message *cm);






// The main event handler.
void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {

struct mg_coap_message  *cm = (struct mg_coap_message *) ev_data;      // CoAP message

    if (ev == MG_EV_POLL){
        return;                                                       // wenn Event Polling
    }
    else{
        //UARTprintf("USER HANDLER GOT EVENT %d\r\n", ev);

        switch (ev) {

            case MG_EV_ACCEPT: {    // 1 : New connection accepted. union socket_address *
                char addr[32];
                    mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr), MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
                    UARTprintf("%p: Connection from %s\r\n", nc, addr);
                }
                break;

            case MG_EV_RECV:        // 3  : Data has been received. int *num_bytes
                UARTprintf("%p: Data has been received\r\n", nc);
                break;

            case MG_EV_SEND:        // 4  : Data has been written to a socket. int *num_bytes
                UARTprintf("%p: Data has been sent\r\n", nc);
                break;

            case MG_EV_CLOSE:       // 5   : Connection is closed. NULL
                UARTprintf("%p: Connection closed\r\n", nc);
                break;


            case MG_EV_COAP_CON:       // CoAP CON received
                UARTprintf("CoAP CON received:\n");
                coapProcessMessage(nc, cm);
                break;

            case MG_EV_COAP_NOC:
                UARTprintf("CoAP NON received:\n");
                coapProcessMessage(nc, cm);
                break;

            case MG_EV_COAP_ACK:
                UARTprintf("CoAP ACK received for message with msg_id = %d\n", cm->msg_id);
                break;

            case MG_EV_COAP_RST:
                UARTprintf("CoAP RST received with msg_id = %d received\n", cm->msg_id);
                break;


            default:
                UARTprintf("Event handler got event: %d\r\n", ev);
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



void coapProcessMessage(struct mg_connection *nc, struct mg_coap_message *cm)
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

    // Print message details to UART
    // *****************************

    UARTprintf("  Type|Code|ID|Token:  ");
    UARTprintf(" %1d(%s)|",    cm->msg_type, coapTypeStrings[cm->msg_type]);
    sprintf(&localStr[0], "%d.%2.2d,%s|",cm->code_class, cm->code_detail, getCodeStr(cm));
    UARTprintf(&localStr[0]);
    UARTprintf(" %5d|",    cm->msg_id);
    // Token
    if(cm->token.len > 0){
        UARTprintf("0x");
        for(i=0; i<cm->token.len; i++){
            value = *((uint8_t *)(cm->token.p)+i);
            sprintf(&localStr[2*i], "%2.2x", value);
        }
        UARTprintf("%s",localStr);
    }
    UARTprintf("\r\n");



    UARTprintf("  Payload:       ");
    memcpy(&localStr[0], cm->payload.p, cm->payload.len);
    localStr[cm->payload.len] = '\0';
    UARTprintf("  %s\n\r",   &localStr[0]);

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







