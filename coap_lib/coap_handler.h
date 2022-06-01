/*
 * event_handler.h
 *
 *  Created on: 7.12.2021
 *      Author: bboeck
 */

#ifndef COAP_HANDLER_H_
#define COAP_HANDLER_H_

#include "third_party/mongoose.h"


#define COAP_CODEDETAIL_GET         1
#define COAP_CODEDETAIL_POST        2
#define COAP_CODEDETAIL_PUT         3
#define COAP_CODEDETAIL_DELETE      4
#define COAP_CODEDETAIL_CONTENT     5

#define COAP_OPTION_ETAG                4
#define COAP_OPTION_URIPATH             11
#define COAP_OPTION_CONTENTFORMAT       12
#define COAP_OPTION_ACCEPT              17


#define OPTION_URI_PATH	11

#define MAX_PAYLOAD_SIZE        (200)


#define COAP_SERVER_URL         "udp://:5683"


#define DISCOVER_PATH ".well-known/core"
#define LED_BASEPATH "light"
#define TMP_BASEPATH "temperature"
#define LUX_BASEPATH "lux"






// Mongoose event handler
void coap_handler(struct mg_connection *nc, int ev, void *ev_data);      // handler where all the network interrupts are handled




#endif /* EVENT_HANDLER_H_ */
