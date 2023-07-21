//main_test.cpp
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

//RQS_r[] = "RS";
//RQS_q[] = "RQ";
//RQS_i[] = "RI";
//RQS_n[] = "NA";


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
    struct Message* nm = get_message();
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



int main(int argc, char* argv[]){
    int r;
    sprintf(log_buffer, "MAIN: Setting up messages.");
    logger(log_buffer, 4);
    u2u_message_setup();
    r = u2u_topic_exchange(mock_payload_0, 0);
    r = u2u_topic_exchange(mock_payload_1, 1);
    mock_payload_0[12] = 'a';
    mock_payload_1[12] = 'b';

    if (argc>0){                // Arguments present.
        int a1 = ascii_to_int(argv[1]);
        int port = ascii_to_int(argv[2]);
        if (port<0 || port>1){ port = 0; }
        if (a1<0 || a1>1){ return 1; }

        if (a1==0){  // First arg is for send_test.
            if (argc>=6){       // Checking all other args are present.
                r = send_test(port, argv[3], argv[4], argv[5], argv[6], argv[7]);
            }
        }else if (a1==1){  // First arg is for receive_test.
            r = receive_test(port, argv[3]);
        }
    }


    sprintf(log_buffer, "MAIN: Result: %d.", r);
    logger(log_buffer, 4);
    sprintf(log_buffer, "MAIN: Done.");
    logger(log_buffer, 4);

    return 0;
}
