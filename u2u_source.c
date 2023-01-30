#ifndef SOURCE_C
#define SOURCE_C
#include <stdio.h>
#include "hardware/irq.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"
#include "u2u_source.h"
#include "u2u_aux.h"
#include "u2u_aux.c"
#include "u2uclientdef.h"



int u2u_source_test(void){
    printf("u2u_source activated\n");
    return 1;
}

void message_setup(){
    //printf("message setup\n");
    u2u_flag_register = 0;
    uart_init(uart0, BAUD_RATE);
    uart_init(uart1, BAUD_RATE);
    gpio_set_function(UART_TX_IN, GPIO_FUNC_UART);
    gpio_set_function(UART_TX_OUT, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_IN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_OUT, GPIO_FUNC_UART);
    uart_set_format(uart0, DATA_BITS, STOP_BITS, PARITY);
    uart_set_format(uart1, DATA_BITS, STOP_BITS, PARITY);
    uart_set_hw_flow(uart0, false, false);
    uart_set_hw_flow(uart1, false, false);
    uart_set_fifo_enabled(uart0, false);
    uart_set_fifo_enabled(uart1, false);
    irq_set_exclusive_handler(UART0_IRQ, uart0_irq_routine);
    irq_set_exclusive_handler(UART1_IRQ, uart1_irq_routine);
    irq_set_enabled(UART0_IRQ, true);
    irq_set_enabled(UART1_IRQ, true);
    uart_set_irq_enables(uart0, true, false);
    uart_set_irq_enables(uart1, true, false);
    message_components[0][0] = messages[0].Sender;
    message_components[0][1] = messages[0].Receiver;
    message_components[0][2] = messages[0].RQS;
    message_components[0][3] = messages[0].Topic;
    message_components[0][4] = messages[0].Chapter;
    message_components[0][5] = messages[0].Length;
    message_components[0][6] = messages[0].Payload;
    message_components[0][7] = messages[0].Hopcount;
    message_components[0][8] = messages[0].CRC;
    message_components[1][0] = messages[1].Sender;
    message_components[1][1] = messages[1].Receiver;
    message_components[1][2] = messages[1].RQS;
    message_components[1][3] = messages[1].Topic;
    message_components[1][4] = messages[1].Chapter;
    message_components[1][5] = messages[1].Length;
    message_components[1][6] = messages[1].Payload;
    message_components[1][7] = messages[1].Hopcount;
    message_components[1][8] = messages[1].CRC;
}




void uart0_irq_routine(void){
    u2u_flag_register = u2u_flag_register | 0b00000001;
    //gpio_put(LED_2, 1);
    while (uart_is_readable(uart0)){
        uint8_t ch = uart_getc(uart0);
        //putchar_raw(ch);
        //if (uart_is_writable(uart0)){
        //    uart_putc(uart0, ch);
        //}
        uart_character_processor(ch, 0);
    }
}

void uart1_irq_routine(void){
    u2u_flag_register = u2u_flag_register | 0b00000010;
    //gpio_put(LED_3, 1);
    while (uart_is_readable(uart1)){
        uint8_t ch = uart_getc(uart1);
        //putchar_raw(ch);
        //if (uart_is_writable(uart1)){
        //    uart_putc(uart1, ch);
        //}
        uart_character_processor(ch, 1);
    }
}


void uart_character_processor(uint8_t ch, bool port){
            //temp_buffer[temp_ch_counter] = ch;
            //temp_ch_counter = (temp_ch_counter + 1) % 255;
            if (marked_for_data[port]){

                u2u_flag_register = u2u_flag_register | 0b00001000;
                //printf("D: %c\n", ch);
                //if (ch_counter[port] < rx_text_len[port]){
                if (ch_counter[port] < messages[port].intLength){
                    //gpio_put(LED_1, 1);
                    //printf("#\n");
                    payload_buffer[port][ch_counter[port]] = ch;
                    //printf("p: %c\n", payload_buffer[port][ch_counter[port]]);
                    ch_counter[port] +=1; // Keeping track of each character per segment.
                    //printf("h: %d\n", ch_counter);
                }
                else{   //If full payload is received:
                    marked_for_data[port] = 0;

                    payload_buffer[port][ch_counter[port]] = 0;
                    segment_counter[port] += 1; // Each segment is seperated by a ':'.
                    ch_counter[port] = 0;
                    //printf("pl: %c\n", payload_buffer[port]);
                }
            }
            else{   // If not marked for data
                switch (ch){
                    case 58: // Character ':'.

                        u2u_flag_register = u2u_flag_register | 0b00000100;
                        //printf("message_handler()\n");
                        //printf("c: %c\n", ch);
                        //gpio_put(LED_0, 1);
                        if (segment_counter[port] > 0){  // Adding sting terminator at end of each segment.
                            message_components[port][segment_counter[port]-1][ch_counter[port]] = 0;
                        }
                        segment_counter[port] += 1; // Each segment is seperated by a ':'.
                        ch_counter[port] = 0;
                        if (segment_counter[port] == 7){
                            marked_for_length[port] = 1;
                        }
                        break;
//                    case 13 || 10 || 0:
//                        rx_buffer_full_flag[port] = 1;
//                        break;
                    default:
                        if (segment_counter[port] > 0){ // Before first ':' some rubbish is expected on uart reception.
                            //printf("d: %c\n", ch);
                            if (ch_counter[port] <= message_component_length[segment_counter[port]-1]){
                                //gpio_put(LED_0, 0);
                                //printf("!\n");
                                message_components[port][segment_counter[port]-1][ch_counter[port]] = ch;
                                ch_counter[port] +=1; // Keeping track of each character per segment.
                                //printf("m: %c\n", message_components[port][ch_counter[port]]);
                                //printf("chc: %d, %d\n", ch_counter[port], segment_counter[port]);
                            }else{
                                ch_counter[port] = 0;
                            }
                        }
                        break;
                }
            }

        if (segment_counter[port] > MESSAGE_SEGMENTS){ //i.e. == 9?
            segment_counter[port] = 0;
            rx_buffer_full_flag[port] = 1;
            message_counter[port] += 1;
            messages[port].Port = port;
        }

        if (marked_for_length[port]){
            marked_for_length[port] = 0;
            marked_for_data[port]  = 1;
            messages[port].intLength = ascii_to_int(messages[port].Length);
            //uart_puts(uart0, mess);
        }
}

void compose_response_message(char* buffer, bool port){
    u2u_flag_register = u2u_flag_register | 0b00010000;
    int index = 0;

    index = copy_str(buffer, self_name, index);
    //printf("sender: %s, i: %d, buffer: %s\n", self_name, index, buffer);
    buffer[index] = ':';
    index++;

    index = copy_str(buffer, messages[port].Sender, index);
    //printf("receiver: %s, i: %d, buffer: %s\n", messages[port].Sender, index, buffer);
    buffer[index] = ':';
    index++;

    if (topic_function_selector == -1){
        index = copy_str(buffer, RQS_n, index);
    }else{
        index = copy_str(buffer, RQS_r, index);
    }
    //printf("RQS: %s, i: %d, buffer: %s\n", messages[port].RQS, index, buffer);
    buffer[index] = ':';
    index++;

    index = copy_str(buffer, messages[port].Topic, index);
    //printf("topic: %s, i: %d, buffer: %s\n", messages[port].Topic, index, buffer);
    buffer[index] = ':';
    index++;

    index = copy_str(buffer, messages[port].Chapter, index);
    //printf("chapter: %s, i: %d, buffer: %s\n", messages[port].Chapter, index, buffer);
    buffer[index] = ':';
    index++;

    char len_buf[4];
    int_to_ascii(len_buf, len(payload_response_buffer[port]), 3);
    index = copy_str(buffer, len_buf, index);
    //printf("payload len: %s, i: %d, buffer: %s\n", messages[port].Length, index, buffer);
    buffer[index] = ':';
    index++;

    index = copy_str(buffer, payload_response_buffer[port], index);
    //printf("payload: %s, i: %d, buffer: %s\n", payload_response_buffer[port], index, buffer);
    buffer[index] = ':';
    index++;

    index = copy_str(buffer, messages[port].CRC, index);
    //printf("crc: %s, i: %d, buffer: %s\n", messages[port].CRC, index, buffer);
    buffer[index] = ':';
    index++;
    buffer[index] = '\0';
    index++;
    //printf("crm: i: %d, buffer: %s\n",  index, buffer);
}

void compose_forward_message(char* buffer, bool port){
    u2u_flag_register = u2u_flag_register | 0b00100000;
    int index = 0;

    index = copy_str(buffer, messages[port].Sender, index);
    buffer[index] = ':';
    index++;

    index = copy_str(buffer, messages[port].Receiver, index);
    buffer[index] = ':';
    index++;

    index = copy_str(buffer, messages[port].RQS, index);
    buffer[index] = ':';
    index++;

    index = copy_str(buffer, messages[port].Topic, index);
    buffer[index] = ':';
    index++;

    index = copy_str(buffer, messages[port].Chapter, index);
    buffer[index] = ':';
    index++;

    index = copy_str(buffer, messages[port].Length, index);
    buffer[index] = ':';
    index++;

    //index = copy_str(buffer, message.Payload, index);
    index = copy_str(buffer, payload_buffer[port], index);

    buffer[index] = ':';
    index++;

    int hopvalue = ascii_to_int(messages[port].Hopcount);
    hopvalue++;
    char hopstr[6];

    int_to_ascii(hopstr, hopvalue, 3);
    index = copy_str(buffer, hopstr, index);
    buffer[index] = ':';
    index++;

    index = copy_str(buffer, messages[port].CRC, index);
    buffer[index] = ':';
    index++;
    buffer[index] = '\0';
    index++;
}

void message_forwarder(bool port){
    printf("message_forwarder()\n");
    char forward_buffer[255];
    compose_forward_message(forward_buffer, port);
    if (port==0){
        uart_puts(uart1, forward_buffer);
    }
    if (port==1){
        uart_puts(uart0, forward_buffer);
    }
    printf("Receiver GEN or OTHERS:: forwarder, %s, %d\n", forward_buffer, 1-port);
}

void message_responder(bool port){
    printf("message_responder()\n");
    char response_buffer[255];
    compose_response_message(response_buffer, port);
    if (port==0){
        uart_puts(uart0, response_buffer);
    }
    if (port==1){
        uart_puts(uart1, response_buffer);
    }
    printf("Receiver GEN or US:: message_responder: %s, %d\n", response_buffer, port);
}

void message_processor(bool port){
    int t;
    for (t=0; t<TOPIC_AMOUNT; t++){
        if (cmp(messages[port].Topic, topic_list[t])){
            topic_function_selector = t;
            copy_str(payload_response_buffer[port], msg_responses[t], 0);
            break;
        }
    }
    printf("message_processor(), topic selected: %d\n", topic_function_selector);
   // if (cmp(messages[port].Topic, topic_list[msg_HAIL])){
   //     topic_function_selector = 0;
   //     copy_str(payload_response_buffer[port], msg_hail_response, 0);
   // }
    //topic_function_selector = message_topic_parser(messages[port].Topic, payload_response_buffer, port);
    message_responder(port);
}


void message_handler(bool port){
    printf("message_handler()\n");
   // for (int i=0; i<temp_ch_counter; i++){
   //     printf("%c", temp_buffer[i]);
   // }
   // printf("\n");
    if (cmp(messages[port].Receiver, self_name)){
        message_processor(port);
    }
    else{
        printf("message_hander::Receiver: %s\n", messages[port].Receiver);
        //gpio_put(LED_0, 1);
        if (cmp(messages[port].Receiver, general_call)){
            message_processor(port);
        }
        message_forwarder(port);
    }
    //gpio_put(LED_0, 0);
    //gpio_put(LED_1, 0);
}


void print_messages(bool port){
    if (port==0){
        printf("---Message0:---\n");
        printf("Sender: %s\n", messages[0].Sender);
        printf("Receiver: %s\n", messages[0].Receiver);
        printf("RQS: %s\n", messages[0].RQS);
        printf("Payload, length: %s, %d\n", payload_buffer[0], messages[0].intLength);
        printf("\n");
    }
    if (port==1){
        printf("---Message1:---\n");
        printf("Sender: %s\n", messages[1].Sender);
        printf("Receiver: %s\n", messages[1].Receiver);
        printf("RQS: %s\n", messages[1].RQS);
        printf("Payload, length: %s, %d\n", payload_buffer[1], messages[1].intLength);
        printf("\n");
    }
}



#endif
