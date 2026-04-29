#ifndef U2UCLIENTPROFILE_H
#define U2UCLIENTPROFILE_H

/* Indicates the port capabilities (Linux: 1, PICO: 2, ESP32: 3). */
#define U2U_PLATFORM_CHANNELS 1

/* Defines this Node's name. Must be unique across the network */
#define SELF_NAME  "NODE_NAME"

/* Set the payload as a response to a specific topic. Implemented in *.c */
extern const char* topic_default_responses[];


#endif //U2UCLIENTPROFILE_H
