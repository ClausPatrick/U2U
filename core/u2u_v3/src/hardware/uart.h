//hardware/uart.h
#ifndef HARDWARE_UART_H
#define HARDWARE_UART_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "../c_logger.h"

#define uart0 0
#define uart1 1

#define UART0_IRQ 0
#define UART1_IRQ 1

#define GPIO_FUNC_UART 0 
#define UART_PARITY_NONE 0 

typedef void (*irq_handler_t)(void);

void uart_init(int uart, int buadrate);
void uart_set_format(int uart, int data_bits, int stop_bits, int parity);
void uart_set_hw_flow(int uart, bool uart_setting, bool uart_setting2);
void uart_set_fifo_enabled(int uart, bool uart_setting);
void irq_set_exclusive_handler(int UART_IRQ_, irq_handler_t uart1_irq_routine);
void irq_set_enabled(int UART_IRQ, bool uart_setting);
void uart_set_irq_enables(int uart, bool uart_setting1, bool uart_setting);

static bool readable[2] = {0, 0};

int uart_parse_test(const char* test_message, int len_, int uart);
uint8_t uart_getc(uint8_t uart);
bool uart_is_readable(uint8_t uart);
uint8_t uart_puts(uint8_t uart, char* str);

/* TEST forward declaration */
void uart0_irq_routine(void);
void uart1_irq_routine(void);


#endif
