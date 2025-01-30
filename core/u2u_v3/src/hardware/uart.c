//hardware/uart.c
#include "uart.h"
#include "../c_logger.h"

static char log_buffer[1024];

static int ch_counter = 0;
static const char* test_message_p;
static char test_message_[1024];

static int test_string_length;


/* Implementation of hardware specific mock functions on pico. */
void uart_init(int uart, int baudrate){ return; }
void uart_set_format(int uart, int data_bits, int stop_bits, int parity){ return; }
void uart_set_hw_flow(int uart, bool uart_setting, bool uart_setting2){ return; }
void uart_set_fifo_enabled(int uart, bool uart_setting){ return; }
//void irq_set_exclusive_handler(int UART0_IRQ, void uart0_irq_routine){ return; }
void irq_set_exclusive_handler(int UART_IRQ_, irq_handler_t uart_irq_routine){ return; }
void irq_set_enabled(int UART_IRQ_, bool uart_setting){ return; }
void uart_set_irq_enables(int uart, bool uart_setting1, bool uart_setting){ return; }



int uart_parse_test(const char* test_message, int len_, int port){
    int i;
    //int port = 0;
    sprintf(log_buffer, "UART: uart%d uart_parse_test readable: %d, ch_counter: %d, test_message: %s, len: %d.", port, readable[port], ch_counter, test_message, len_);
    uart_logger(log_buffer, 4);
    //char log_buffer[255];
    for (i=0; i<len_; i++){
        test_message_[i] = test_message[i];
    }
    test_message_p = test_message_;
    //test_message_p = test_message;
    test_string_length = len_;
    ch_counter = 0;
    readable[port] = 1;
    return 0;
}


int str_len(const char* s){
    int i = 0;
    while (s[i] != 0){
        i++;
    }
    return i;
}

uint8_t uart_getc(uint8_t uart){
    char return_ch;
    //sprintf(log_buffer, "UART: uart_getc START.");
    //uart_logger(log_buffer, 4);
    if (ch_counter<test_string_length && readable[uart]){
        return_ch = test_message_p[ch_counter];
        ch_counter++;
    }else if (ch_counter==test_string_length && readable[uart]){
        readable[uart] = 0;
        return_ch = test_message_p[ch_counter];
        //printf("uart_getc message completed. 0\n" );
        ch_counter++;
    }else{
        readable[uart] = 0;
        return_ch = 0;
        //printf("uart_getc message completed. 1\n" );
    }
    if (return_ch==0){
        sprintf(log_buffer, "(uart_getc)(%d) (%d)", uart, return_ch);
    }else{
        sprintf(log_buffer, "(uart_getc)(%d) '%c'", uart, return_ch);
    }
    uart_logger(log_buffer, 4);
    sprintf(log_buffer, "%d", return_ch);
    if (uart==0){
        //uart0_logger(log_buffer, 4);
    }else{
        //uart1_logger(log_buffer, 4);
    }
    return return_ch;
}

uint8_t uart_puts(uint8_t uart, char* str){
    //printf("uart_puts: %d, %s\n", uart, str);
    int r;
    sprintf(log_buffer, "(uart_puts)(%d) \"%s\"", uart, str);
    uart_logger(log_buffer, 4);
    //sprintf(log_buffer, "UART%d: uart_puts ch_counter: %d, str: %s.", uart,  ch_counter, str);
    //uart_logger(log_buffer, 4);
    int len_str = str_len(str);
    if (uart==0){
        //uart_test_setup(str, len_str);
    }
    sprintf(log_buffer, "%s", str);
    if (uart==0){
        uart0_logger(log_buffer, 4);
    }else{
        uart1_logger(log_buffer, 4);
    }
    return 0;
}

/* uartN_irq_routine() loops around this function until readable == 0.*/
bool uart_is_readable(uint8_t uart){
    //sprintf(log_buffer, "UART%d: uart_is_readable ch_counter: %d, uart: %d, test_string_length %d, readable[uart]: %d.", uart, ch_counter, uart, test_string_length, readable[uart]);
    //uart_logger(log_buffer, 4);
    return readable[uart];
}



