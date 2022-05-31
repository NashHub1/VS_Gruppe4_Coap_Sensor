/*
 * event_handler.h
 *
 *  Created on: 7.12.2021
 *      Author: bboeck
 */

#ifndef EVENT_HANDLER_H_
#define EVENT_HANDLER_H_

#include "third_party/mongoose/mongoose.h"


#define COAP_CODEDETAIL_GET         1
#define COAP_CODEDETAIL_POST        2
#define COAP_CODEDETAIL_PUT         3
#define COAP_CODEDETAIL_DELETE      4
#define COAP_CODEDETAIL_CONTENT     5

#define COAP_OPTION_ETAG                4
#define COAP_OPTION_URIPATH             11
#define COAP_OPTION_CONTENTFORMAT       12
#define COAP_OPTION_ACCEPT              17


#define MAX_PAYLOAD_SIZE        (200)


#define COAP_SERVER_URL         "udp://:5683"


#define COAP_QUERY_WELLKNOWNCORE    ".well-known/core"


#define COAP_QUERY_LIGHT            "light"
#define COAP_QUERY_TEMP 			"temperature"
#define COAP_QUERY_LUX				"lux"






// Mongoose event handler
void ev_handler(struct mg_connection *nc, int ev, void *ev_data);      // handler where all the network interrupts are handled




#endif /* EVENT_HANDLER_H_ */
