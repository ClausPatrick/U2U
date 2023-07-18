//u2u.h
#ifndef U2U_H
#define U2U_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "u2u_HAL_pico.h"
#include "u2uclientdef.h"
//#include "c_logger.h"

#define general_name "GEN"
#define uart0 0
#define uart1 1
#define UART0_IRQ 0
#define UART1_IRQ 1

#define MAX_MESSAGE_KEEP 8
#define MAX_MESSAGE_PAYLOAD_SIZE 255
#define MAX_MESSAGE_SIZE 512

//#define BAUD_RATE 115200
//#define DATA_BITS 8
//#define STOP_BITS 1
//#define PARITY UART_PARITY_NONE
//#define UART_IN uart1    //to DS9
//#define UART_OUT uart0

//const char* topic_list[] = { "HAIL", "HELP", "SET LCD", "SET OLED", "GET SENSOR", "SET ENCODER", "GET ENCODER", "SET LED", "SET TIME", "GET TIME", "SET DATE", "GET DATE", "RESERVED 0", "RESERVED 1", "RESERVED 2", "RESERVED 3"};

const uint8_t sg_WAIT_INDEX         = 0;
const uint8_t sg_PREMESSAGE_INDEX   = 0;
const uint8_t sg_SENDER_INDEX       = 1;
const uint8_t sg_RECEIVER_INDEX     = 2;
const uint8_t sg_RQS_INDEX          = 3;
const uint8_t sg_TOPIC_INDEX        = 4;
const uint8_t sg_CHAPTER_INDEX      = 5;
const uint8_t sg_LENGTH_INDEX       = 6;
const uint8_t sg_PAYLOAD_INDEX      = 7;
const uint8_t sg_HOPCOUNT_INDEX     = 8;
const uint8_t sg_CRC_INDEX          = 9;


struct    Message{
//    char  Sender[32];
//    char  Receiver[32];
//    char  RQ[4];
//    char  Topic[32];
//    char  Chapter[4];
    int  ID;
    int   intChapter;
    const char*  Sender;
    const char*  Receiver;
    const char*  RQ;
    const char*  Topic;
    const char*  Chapter;
    int   intLength;

    int   Index;
    int   intRQ;
    char  Segments[12][32];
    char* Payload;
    int   Port;
    int   intCRC;
    char  CRC_buffer[255];
    int   CRC_index;
    bool  CRC_check;
    int   Topic_number;
    bool  For_self;
    bool  For_gen;
    bool  Cleared;
    //bool  RQ_flags[4]; //[0]: R, [1]: Q, [2]: I, [3]: N.
};


const char RQS_r[] = "RS";
const char RQS_q[] = "RQ";
const char RQS_i[] = "RI";
const char RQS_n[] = "NA";

/*Inbound messages are stored in 'messages'*/
struct Message* messages[MAX_MESSAGE_KEEP];
//struct Message* messages_out[MAX_MESSAGE_KEEP];


/*General purpose functions*/
int ascii_to_int(char* str);
void int_to_ascii(char* buf, int i, int p) ;
int len(const char* s);
int cmp(char* s1, const char* s2);
int copy_str(char* buffer, const char* s, int index);
char get_crc(char* buffer, char length);

/*Function to sequence the segments indicated by a ':'.*/
uint8_t message_clear(uint8_t message_index, int port);
uint8_t message_reset(uint8_t message_index, int port);
void colon_parser(uint8_t message_index, int port); //Appending NULL char onto last pos in segment;

bool colon_checker(uint8_t message_index, int port, char ch); //message_index is ignored
uint8_t premessage_setup(uint8_t message_index, int port, char ch); // message_index is ignored and message_counter_global is written into ~counter_port[~];
uint8_t write_into_segment(uint8_t message_index, int port, char ch);
uint8_t write_into_payload_length(uint8_t message_index, int port, char ch);
uint8_t write_into_payload_data(uint8_t message_index, int port, char ch);
uint8_t last_segment(uint8_t message_index, int port, char ch);
uint8_t determine_addressee(uint8_t message_index, int port); //0: Other, 1: Self, 2: General;
int message_topic_checker(uint8_t message_index, int port, char* buffer);
uint8_t append_segment(char* buffer, const char* str, int index);
uint8_t add_crc(char* buffer, int index);
uint8_t compose_transmit_message(struct Message* message_tx, char* buffer);
uint8_t u2u_send_message_uart0(struct Message* message_tx);
uint8_t u2u_send_message_uart1(struct Message* message_tx);
uint8_t u2u_send_message(struct Message* message_tx);
uint8_t compose_response_message(uint8_t message_index, int port, char* buffer);
uint8_t compose_forward_message(uint8_t message_index, int port, char* buffer);
//uint8_t write_from_uart0(char* buffer);
//uint8_t write_from_uart1(char* buffer);
uint8_t self_call(uint8_t message_index, int port);
uint8_t other_call(uint8_t message_index, int port);
uint8_t no_response_call(uint8_t message_index, int port);
uint8_t general_call(uint8_t message_index, int port);

/* Function message_processor() calls to format response */
uint8_t format_return_message(uint8_t message_index);

/* Completion of writing into last segment calls to process message for responses (if any).*/
uint8_t  message_processor(uint8_t message_index);

struct Message* get_message();

uint8_t u2u_topic_exchange(char* custom_payload, uint8_t topic_number);
uint8_t uart_character_processor(char ch);
uint8_t uart0_character_processor(char ch);
uint8_t uart1_character_processor(char ch);
//void uart0_irq_routine(void);
//void uart1_irq_routine(void);
void u2u_message_setup();
//void testfunc(char* s, int port);


#endif
