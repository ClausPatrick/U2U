//u2u_HAL_lx.h
#ifndef U2U_HAL_LX_H
#define U2U_HAL_LX_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()
#include <pthread.h>
#include "c_logger.h"
#include "u2uclientdef.h"

#define uart0 0


#define BAUD_RATE 115200
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY    UART_PARITY_NONE
#define UART_IN uart1    //to DS9
#define UART_OUT uart0



/*From u2u.h*/
uint8_t uart0_character_processor(char ch);
uint8_t uart1_character_processor(char ch);

void uart0_irq_routine(void);
void uart1_irq_routine(void);
uint8_t write_from_uart(char* buffer, int buffer_lenght);
uint8_t write_from_uart0(char* buffer, int buffer_lenght);
uint8_t write_from_uart1(char* buffer, int buffer_lenght);
void comm_logger(char* buffer, int len, int pdr);
int u2u_uart_setup();
int u2u_uart_close();

//int message_ready;


#endif
