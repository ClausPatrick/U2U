//main_test.cpp - U2U TESTER
#include <iostream>
#include <cassert>
#include <vector>
#include <ctime>

extern "C"{
    #include "u2u.h"
    #include "c_logger.h"
    #include "u2uclientdef.h"
}

char log_buffer[1024];


/*Forward declarations for testing:*/
int uart_test_setup(char* test_message, int len);
void uart0_irq_routine(void);
void uart1_irq_routine(void);

//
//RQS_r[] = "RS";
//RQS_q[] = "RQ";
//RQS_i[] = "RI";
//RQS_n[] = "RN";
//RQS_a[] = "RA";

char mock_payload_0[64] = "HAIL mock -     ";
char mock_payload_1[64] = "HELP mock -     ";

int receive_test(int port, char* m){
    int len_str = len(m);
    int r = 0;
    if (port==0){
        uart_parse_test(m, len_str, 0);
        uart0_irq_routine();
    }else if (port==1){
        uart_parse_test(m, len_str, 1);
        uart1_irq_routine();
    }else if (port==2){
        return 1;
    }
    struct Message* nm = NULL;

    sprintf(log_buffer, "MAIN: receive_test: waiting for messages.");
    logger(log_buffer, 4);
    while(nm == NULL){
        nm = get_message();
    }
    sprintf(log_buffer, "MAIN: receive_test: Message received.");
    logger(log_buffer, 4);
    sprintf(log_buffer, "  %d  %s  %s  %s  %s  %s  %s  %d  %d  %d  %d  %d  ", nm->Port, nm->Sender, nm->Receiver, nm->RQ, nm->Topic, nm->Chapter, nm->Payload, nm->Index, nm->intCRC_rx, nm->intCRC_cal, nm->Topic_number, nm->Router_val);
    message_logger(log_buffer, 4);
    return r;
}


int send_test(int port, char* rec, char* rqs, char* top, char* cpt, char* pld){
    int r;
    struct Message message_tx;
    message_tx.Receiver = rec;
    message_tx.RQ = rqs;
    message_tx.Topic = top;
    message_tx.intChapter = ascii_to_int(cpt);
    message_tx.Payload = pld;
    
    sprintf(log_buffer, "MAIN: send_test.");
    logger(log_buffer, 4);
    sprintf(log_buffer, "\t %s", message_tx.Receiver);
    logger(log_buffer, 4);
    sprintf(log_buffer, "\t %s", message_tx.RQ);
    logger(log_buffer, 4);
    sprintf(log_buffer, "\t %s", message_tx.Topic);
    logger(log_buffer, 4);
    sprintf(log_buffer, "\t %d", message_tx.intChapter);
    logger(log_buffer, 4);
    sprintf(log_buffer, "\t %s", message_tx.Payload);
    logger(log_buffer, 4);

    if (port==0){
        r = u2u_send_message_uart0(&message_tx);
    }else if (port==1){
        r = u2u_send_message_uart1(&message_tx);
    }else if (port==2){
        r = u2u_send_message_uart0(&message_tx);
        r = u2u_send_message_uart1(&message_tx);
    }
    return r;
}

int main_self_test(int port){
    uint8_t return_val = u2u_self_test(port);
    sprintf(log_buffer, "MAIN: %s:: %d.", __func__, return_val);
    logger(log_buffer, 4);
    return return_val;
}


int main(int argc, char* argv[]){
    int r;
    sprintf(log_buffer, "MAIN: Setting up messages with %d arguments.", argc);
    logger(log_buffer, 4);
    r = u2u_message_setup();
    r = u2u_topic_exchange(mock_payload_0, 0);
    r = u2u_topic_exchange(mock_payload_1, 1);
    mock_payload_0[12] = 'a';
    mock_payload_1[12] = 'b';

    if (argc>2){                // Arguments present.
        int first_arg = ascii_to_int(argv[1]);
        int port = ascii_to_int(argv[2]);
        if (port<0 || port>1){ port = 0; }
        if (first_arg<0 || first_arg>2){ return 999; }

        if (first_arg==0){  // First arg is for send_test.
            if (argc>=6){       // Checking all other args are present.
                sprintf(log_buffer, "MAIN: calling send_test: %d, %s, %s, %s, %s, %s.", port, argv[3], argv[4], argv[5], argv[6], argv[7]);
                logger(log_buffer, 4);
                r = send_test(port, argv[3], argv[4], argv[5], argv[6], argv[7]);
            }
        }else if (first_arg==1){  // First arg is for receive_test.
            sprintf(log_buffer, "MAIN: calling receive_test: %d, %s.", port, argv[3]);
            logger(log_buffer, 4);
            r = receive_test(port, argv[3]);
        }else if (first_arg==2){  // First arg is for self_test.
            sprintf(log_buffer, "MAIN: calling self_test: %d.", port);
            logger(log_buffer, 4);
            r = main_self_test(port);
        }

        sprintf(log_buffer, "MAIN: Result: %d.", r);
        logger(log_buffer, 4);
        r = u2u_close();
        sprintf(log_buffer, "MAIN: Done.");
        logger(log_buffer, 4);
    }else{
        printf("Usage: %s <mode> <port> [additional arguments]\n", argv[0]);
        printf("\n");
        printf("Modes:\n");
        printf("  0 - Send Test\n");
        printf("       Usage: %s 0 <port> <arg1> <arg2> <arg3> <arg4> <arg5>\n", argv[0]);
        printf("       Example: %s 0 1 RECEIVER RQS TOPIC CHAPTER PAYLOAD\n", argv[0]);
        printf("\n");
        printf("  1 - Receive Test\n");
        printf("       Usage: %s 1 <port> <arg1>\n", argv[0]);
        printf("       Example: %s 1 0 MESSAGE (Formatted as per U2U standard.)\n", argv[0]);
        printf("\n");
        printf("  2 - Self Test\n");
        printf("       Usage: %s 2 <port>\n", argv[0]);
        printf("       Example: %s 2 1\n", argv[0]);
        printf("\n");
        printf("Port values:\n");
        printf("  0 - Port 0\n");
        printf("  1 - Port 1\n");
        printf("\n");
        printf("Error: No arguments provided, will quit for now.\n");
        return 1;
    }
    return 0;
}
