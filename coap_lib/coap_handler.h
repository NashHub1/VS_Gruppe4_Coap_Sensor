
#ifndef COAP_HANDLER_H_
#define COAP_HANDLER_H_

#include "third_party/mongoose.h"


#define OPTION_URI_PATH	11
#define OPTION_CONTENT_FORMAT 12


#define COAP_SERVER_URL "udp://:5683"


#define DISCOVER_PATH ".well-known"
#define TMP_BASEPATH "temperature"
#define LUX_BASEPATH "light"


// Mongoose event handler (CoAP)
void coap_handler(struct mg_connection *nc, int ev, void *ev_data);




#endif /* COAP_HANDLER_H_ */
