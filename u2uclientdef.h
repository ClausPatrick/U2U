//u2uclientdef.h
#ifndef CLIENT_DEF_C
#define CLIENT_DEF_C

//#define UART_RX_IN 13
//#define UART_TX_IN 12
//#define UART_RX_OUT 5
//#define UART_TX_OUT 4

#define TOPIC_AMOUNT 15
#define U2U_PLATFORM_CHANNELS 1

//extern const char self_name[];
//const char self_name[] = "TEST_RCVR";
#define self_name                   "MYNAME"

#define self_name_TEST              "MYNAME_TEST"
//
//#define UART_RX_IN 1
//#define UART_TX_IN 0
//#define UART_RX_OUT 5
//#define UART_TX_OUT 4
//
//

#define PORT 8080

//#define SELF_IP_ADDRESS "192.168.0.155"
const char* SELF_IP_ADDRESS;
#define MAX_IP_LEN (3*4)+3+1 // (3*4 for bytes, 3 for dots and 1 NULL)
#define PEER_COUNT  3 // Self included.
const char PEER_IP_LIST[PEER_COUNT][MAX_IP_LEN];
const char PEER_NAME_LIST[PEER_COUNT][32];


//char* msg_responses[] = {msg_hail_response, msg_help_response, msg_set_lcd_response, msg_set_oled_response, msg_get_sensor_response, msg_set_encoder_response, msg_get_encoder_response, msg_set_led_response} ;

/* Default responses, issued on specific TOPIC and RQ (REQUEST). */
#define msg_HAIL 0
#define  msg_hail_response          "Hail from MYNAME. Testfunction {[(!@#$%^&*_-,.::')]}."

#define msg_HELP 1
#define  msg_help_response          "Some helpful information maybe later."

#define msg_SET_LCD 2
#define  msg_set_lcd_response       "Nope, got nothing."

#define msg_SET_OLED 3
#define  msg_set_oled_response      "Function not supported, no OLED"

#define msg_GET_SENSOR 4
#define  msg_get_sensor_response    "Function not implemented. Sensors not set up yet."

#define msg_SET_ENCODER 5
#define  msg_set_encoder_response   "Received encoder data but not sure what to do with it."

#define msg_GET_ENCODER 6
#define  msg_get_encoder_response   "Function not supported, no encoder hardware."

#define msg_SET_LED 7
#define  msg_set_led_response       "Function not supported, LED hardware."

#define msg_SET_TIME 8
#define  msg_set_time_response      "Function not supported, no time keeping."

#define msg_GET_TIME 9
#define  msg_get_time_response      "Function not supported, no time keeping."

#define msg_SET_DATE 10
#define  msg_set_date_response      "Function not supported, no date keeping."

#define msg_GET_DATE 11
#define  msg_get_date_response      "Function not supported, no date keeping."

#define msg_RESERVED0 12
#define  msg_RESERVED_0             "Topic not implemented, index 12."

#define msg_RESERVED1 13
#define  msg_RESERVED_1             "Topic not implemented, index 13."

#define msg_RESERVED2 14
#define  msg_RESERVED_2             "Topic not implemented, index 14."

#define msg_RESERVED3 15
#define  msg_RESERVED_3             "Topic not implemented, index 15."

#define msg_RESERVED4 16
#define  msg_RESERVED_4             "Topic not implemented, index 16."

// 99 - Excluded from normal indexing.
#define  msg_RESET                  "U2U stack reset received."

#endif
