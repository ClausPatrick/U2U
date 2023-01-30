#ifndef SOURCE_H
#define SOURCE_H
#include "hardware/irq.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"

#define BAUD_RATE 115200
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY    UART_PARITY_NONE
#define UART_IN uart1    //to DS9
#define UART_OUT uart0

#define ofs 3
#define MESSAGE_MAX_LENGTH 255

#define MESSAGE_SEGMENTS  9

#define MESSAGE_RECEIVER_GENERAL  "GEN"
#define MESSAGE_REQUEST  "RQ"
#define MESSAGE_RESPONSE  "RS"
#define MESSAGE_NO_ACK  "NA"

#define known_topics 7
#define msg_HAIL 0
#define msg_HELP 1
#define msg_SET_LCD 2
#define msg_SET_OLED 3
#define msg_GET_SENSOR 4
#define msg_SET_ENCODER 5
#define msg_SET_LED 6

#define TEST_ 1

typedef struct Message{
    char Sender[16+ofs];
    char Receiver[16+ofs];
    char RQS[3+ofs];
    char Topic[24+ofs];
    char Chapter[6+ofs];
    char Length[6+ofs];
    char Payload[MESSAGE_MAX_LENGTH+ofs];
    char Hopcount[6];
    char CRC[3+ofs];
    bool Port;
    int intLength;
} Message;
//} static Message;



static Message messages[2];
int  message_component_length[MESSAGE_SEGMENTS] = {16+ofs, 16+ofs, 3+ofs, 24+ofs, 6+ofs, 6+ofs, MESSAGE_MAX_LENGTH+ofs, 6+ofs, 3+ofs};
volatile static char *message_components[2][MESSAGE_SEGMENTS]; //This array is a generator to be indexed during uart irq. Its content will be filled in message_setup().

uint8_t u2u_flag_register;

static int segment_counter[2] = {0, 0};
volatile static int message_counter[2] = {0, 0};
volatile static int ch_counter[2] = {0, 0};
static bool marked_for_length[2] = {0, 0};
static bool marked_for_data[2] = {0, 0};
static char payload_buffer[2][MESSAGE_MAX_LENGTH];
static char payload_response_buffer[2][MESSAGE_MAX_LENGTH] = {0};
static char payload_forward_buffer[2][MESSAGE_MAX_LENGTH] = {0};
//static char forward_buffer[MESSAGE_MAX_LENGTH+100] = {0};
//static char response_buffer[MESSAGE_MAX_LENGTH+100] = {0};
static char sensor_data_frame[255];
static int rx_text_len[2] = {0, 0};    //Payload length indicated by sender converted into int via ascii_to_int().
//static char temp_buffer[255];
//static int temp_ch_counter = 0;

volatile static bool  rx_lcd_buffer_flag[2] = {0, 0};
volatile static bool  rx_buffer_full_flag[2] = {0, 0};
// To verify incoming bytes do not exceed predetermined space in Message.
const char general_call[] = "GEN";
const char RQS_r[] = "RS";
const char RQS_n[] = "NA";


volatile int topic_function_selector = -1;

//const char* topic_list[7] = { "HAIL", "HELP", "SET LCD", "SET OLED", "GET SENSOR", "SET LED", "SET ENCODER"};
void uart_character_processor(uint8_t ch, bool port);
int u2u_source_test(void);
void uart0_irq_routine(void);
void uart1_irq_routine(void);
void message_setup();
void compose_response_message(char* buffer);
void compose_forward_message(char* buffer);
void message_forwarder(bool port);
void message_responder(bool port);
void message_processor(bool port);
void message_handler(bool port);
void print_messages(void);

//TEST comment...

#endif
