#ifndef CLIENT_DEF_C
#define CLIENT_DEF_C


#define self_name "SOMENAME"

#define UART_RX_IN 1
#define UART_TX_IN 0
#define UART_RX_OUT 5
#define UART_TX_OUT 4



#define TOPIC_AMOUNT 7
#define msg_HAIL 0
#define msg_HELP 1
#define msg_SET_LCD 2
#define msg_SET_OLED 3
#define msg_GET_SENSOR 4
#define msg_SET_ENCODER 5
#define msg_SET_LED 6


const char* topic_list[] = { "HAIL", "HELP", "SET LCD", "SET OLED", "GET SENSOR", "SET LED", "SET ENCODER"};

volatile int command_requested = 0;
char msg_hail_response[] = "Hail from MY_NAME. Featuring LCD PCD8544, 84*48 pixels.";
char msg_help_response[] = "SET LCD: Print  84*48 pixel.";
char msg_set_lcd_response[] = "LCD is SET";
char msg_set_oled_response[] = "Function not supported, no OLED";
char msg_get_sensor_response[] = "Function not implemented. Sensors not set up yet.";
char msg_set_encoder_response[] = "Received encoder data but not sure what to do with it.";
char msg_get_encoder_response[] = "Function not supported, no encoder hardware.";
char msg_set_led_response[] = "Function not supported, LED hardware.";

char* msg_responses[] = {msg_hail_response, msg_help_response, msg_set_lcd_response, msg_set_oled_response, msg_get_sensor_response, msg_set_encoder_response, msg_get_encoder_response, msg_set_led_response} ;


#endif
