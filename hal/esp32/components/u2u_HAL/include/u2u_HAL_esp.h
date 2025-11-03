/* /home/pi/c_taal/u2u/exporter/exports/u2u_HAL_pico.h - updated: 2024/11/14 13:44:05. */
//u2u_HAL_esp.h
#ifndef U2U_HAL_H
#define U2U_HAL_H

#include "driver/gpio.h"
#include "driver/uart.h"
#include "u2uclientdef.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include <stdio.h>
#include <string.h>
#include "esp32/rom/uart.h"


#define BAUD_RATE 115200
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY    UART_PARITY_NONE
#define UART_IN uart1
#define UART_OUT uart0

#define TXD1_PIN 21   // UART1 TX
#define RXD1_PIN 19   // UART1 RX
#define TXD2_PIN 17   // UART2 TX
#define RXD2_PIN 16   // UART2 RX



/*From u2u.h*/
uint8_t uart0_character_processor(char ch);
uint8_t uart1_character_processor(char ch);

void uart0_irq_routine(void);
void uart1_irq_routine(void);
uint8_t write_from_uart(char* buffer, int len);
uint8_t write_from_uart0(char* buffer, int len);
uint8_t write_from_uart1(char* buffer, int len);
void comm_logger(char* buffer, int len, int pdr);
//void inbound_message_logger(char* buffer, int len);
int u2u_uart_setup();

//int message_ready;
int u2u_uart_close(void);

#endif

