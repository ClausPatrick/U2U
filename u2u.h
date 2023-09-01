//u2u.h
#ifndef U2U_H
#define U2U_H

//#include "u2u_HAL_pico.h"
#include "u2uclientdef.h"

#if U2U_PLATFORM_CHANNELS == 1
#include "u2u_HAL_lx.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "c_logger.h"
#endif

#if U2U_PLATFORM_CHANNELS == 2
#include "pico/stdlib.h"
#include "u2u_HAL_pico.h"
#endif



#define general_name "GEN"

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

//extern const uint8_t sg_WAIT_INDEX         ;
//extern const uint8_t sg_PREMESSAGE_INDEX   ;
//extern const uint8_t sg_SENDER_INDEX       ;
//extern const uint8_t sg_RECEIVER_INDEX     ;
//extern const uint8_t sg_RQS_INDEX          ;
//extern const uint8_t sg_TOPIC_INDEX        ;
//extern const uint8_t sg_CHAPTER_INDEX      ;
//extern const uint8_t sg_LENGTH_INDEX       ;
//extern const uint8_t sg_PAYLOAD_INDEX      ;
//extern const uint8_t sg_HOPCOUNT_INDEX     ;
//extern const uint8_t sg_CRC_INDEX          ;


struct    Message{
    int  ID;
    int   intChapter;
    const char*  Sender;
    const char*  Receiver;
    const char*  RQ;
    const char*  Topic;
    const char*  Chapter;
    const char*  CRC;
    const char*  Hops;
    int   intLength;
    int   intCh_rx;

    int   Index;
    int   intRQ;
    char  Segments[12][32];
    char* Payload;
    int   Port;
    int   intCRC_rx;
    int   intCRC_cal;
    int   intHops;
    char  CRC_buffer[255];
    int   CRC_index;
    bool  CRC_check;
    int   Topic_number;
    int   Router_val;
    bool  For_self;
    bool  For_gen;
    bool  Cleared;

    //bool  RQ_flags[4]; //[0]: R, [1]: Q, [2]: I, [3]: N.
};


struct Message_Queue {
    int buffer[MAX_MESSAGE_KEEP];
    int front;
    int rear;
    int count;
} ;


//extern const char RQS_r[];
//extern const char RQS_q[];
//extern const char RQS_i[];
//extern const char RQS_n[];

/*Inbound messages are stored in 'messages'*/
struct Message* messages[MAX_MESSAGE_KEEP];
//struct Message* messages_out[MAX_MESSAGE_KEEP];


/*General purpose functions*/
int ascii_to_int(char* str);
void int_to_ascii(char* buf, int i, int p) ;
int float_to_ascii(char* buf, float f, int p);
int len(const char* s);
int cmp(char* s1, const char* s2);
int copy_str(char* buffer, const char* s, int index);
char get_crc(char* buffer, char length);

int in_queue(struct Message_Queue *queue, int data);
int out_queue(struct Message_Queue *queue, int *data);

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
uint8_t compose_transmit_message(struct Message* message_tx, char* buffer, int* message_length);
uint8_t u2u_send_message_uart0(struct Message* message_tx);
uint8_t u2u_send_message_uart1(struct Message* message_tx);
uint8_t u2u_send_message(struct Message* message_tx);
uint8_t compose_response_message(uint8_t message_index, int port, char* buffer, int* message_length);
uint8_t compose_forward_message(uint8_t message_index, int port, char* buffer, int* message_length);
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
int u2u_message_setup();
int u2u_close(void);
//void testfunc(char* s, int port);


#endif
