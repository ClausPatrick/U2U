//c_logger.h
#ifndef LOGGER_C
#define LOGGER_C
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#define LOG_LEVEL 4

static const char LOGFILE[]                 = "logs/main_tester_log.txt";
static const char UART_LOGFILE[]            = "logs/uart_tester_log.txt";
static const char UART0_LOGFILE[]           = "logs/uart0_tester_log.txt";
static const char UART1_LOGFILE[]           = "logs/uart1_tester_log.txt";
static const char U2U_LOGFILE[]             = "logs/u2u_tester_log.txt";
static const char SEGMENT_FLOW_LOGFILE[]    = "logs/segment_tester_log.txt";
static const char QUEUE_LOGFILE[]           = "logs/queue_log.txt";
static const char MESSAGE_LOGFILE[]         = "logs/message_log.txt";
static const char ERROR_LOGFILE[]           = "logs/error_log.txt";
static const char HAL_LOGFILE[]             = "logs/hal_tester_log.txt";
static const char RESULTS_LOGFILE[]         = "logs/results_tester_log.txt";
static const char THREAD_LOGFILE[]          = "logs/thread_log.txt";
static const char THREAD1_LOGFILE[]         = "logs/thread1_log.txt";
static const char THREAD2_LOGFILE[]         = "logs/thread2_log.txt";
static const char GM_THREAD_LOGFILE[]       = "logs/gm_thread_log.txt";
static const char CRC_LOGFILE[]             = "logs/crc_log.txt";
static const char MATRIX_LOG_FILE[]         = "logs/matrix_logs/matrix_log.txt";

int logger(char* str, int level);
int uart0_logger(char* str, int level);
int uart1_logger(char* str, int level);
int u2u_logger(char* str, int level);
int uart_logger(char* str, int level);
int hal_logger(char* str, int level);
int segment_logger(char* str, int level);
int queue_logger(char* str, int level);
int message_logger(char* str, int level);
int error_logger(char* str, int level);
int thread_logger(char* str, int level);
int thread1_logger(char* str, int level);
int thread2_logger(char* str, int level);
int gm_thread_logger(char* str, int level);
int matrix_logger(char* str, int level);
int crc_logger(char* str, int level);

#endif
