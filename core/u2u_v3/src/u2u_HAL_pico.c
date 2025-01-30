//u2u_HAL_pico.c
#include "u2u_HAL_pico.h"

//#include "u2uclientdef.h"


static char log_buffer[1024];    //    %%TEST%%

/* Forward declarations from u2u.h */
uint8_t uart0_character_processor(char ch);
uint8_t uart1_character_processor(char ch);


//message_ready = 0;

void uart0_irq_routine(void){
    int r;
    //gpio_put(LED_2, 1);
    uint8_t ch;
    while (uart_is_readable(uart0)){
        ch = uart_getc(uart0);
        //if (ch==0){ //%%TEST%%
        //    sprintf(log_buffer, "HAL: uart0_irq_routine ch: (%d).", ch); //%%TEST%%
        //}else{ //%%TEST%%
        //    sprintf(log_buffer, "HAL: uart0_irq_routine ch: '%c'.", ch); //%%TEST%%
        //} //%%TEST%%
        //hal_logger(log_buffer, 4); //%%TEST%%
        r = uart0_character_processor(ch);
    }
}

void uart1_irq_routine(void){
    int r;
    //gpio_put(LED_3, 1);
    uint8_t ch;
    while (uart_is_readable(uart1)){
        ch = uart_getc(uart1);
        //if (ch==0){ //%%TEST%%
        //    sprintf(log_buffer, "HAL: uart1_irq_routine ch: (%d).", ch); //%%TEST%%
        //}else{ //%%TEST%%
        //    sprintf(log_buffer, "HAL: uart1_irq_routine ch: '%c'.", ch); //%%TEST%%
        //} //%%TEST%%
        //hal_logger(log_buffer, 4); //%%TEST%%
        r = uart1_character_processor(ch);
    }
}

uint8_t write_from_uart0(char* buffer, int len){
    //printf("UART_0 gmc: %d, lmc: %d, m: %s\n", message_counter_global, message_counter_port[0], buffer);    //    %%TEST%%
    sprintf(log_buffer, "HAL: write_from_uart0 %d, buffer: %s.", len, buffer); //%%TEST%%
    hal_logger(log_buffer, 4); //%%TEST%%
    uart_puts(uart0, buffer);
    //printf("%s\n", log_buffer);    //    %%TEST%%
    return 0;
}

uint8_t write_from_uart1(char* buffer, int len){
    //printf("UART_1 gmc: %d, lmc: %d, m: %s\n", message_counter_global, message_counter_port[1], buffer);    //    %%TEST%%
    sprintf(log_buffer, "HAL: write_from_uart1 %d, buffer: %s.", len, buffer); //%%TEST%%
    hal_logger(log_buffer, 4); //%%TEST%%
    uart_puts(uart1, buffer);
    return 0;
}

uint8_t write_from_uart(char* buffer, int len){
    int r;
    r = write_from_uart0(buffer, len);
    r = write_from_uart1(buffer, len);
    return r;
}

/* Flag for comm logging pdr:
 * bit 0 [1]: PORT, port value.
 * bit 1 [2]: DIR, {0: inbound, 1: outbound}.
 * bit 2 [4]: RFLAG, {For DIR = 0: 0: self or gen addressed, 1: other and forwarded}.
 *               {For DIR = 1: 0: message response,      1: message forward}.
 * bit 3 [8]: ORG, {0: auto,    1: external}.
 *
 * */


void comm_logger(char* buffer, int len, int pdr){
    sprintf(log_buffer, "%d - %s.", pdr, buffer);     //    %%TEST%%
    u2u_logger(log_buffer, 4);              //    %%TEST%%
    return;
}

int u2u_uart_setup(void){
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
    sprintf(log_buffer, "HAL: u2u_uart_setup completed.");    //    %%TEST%%
    hal_logger(log_buffer, 4);    //    %%TEST%%
    return 0;
}

int u2u_uart_close(void){
    int r = 0;
    return r;
}


