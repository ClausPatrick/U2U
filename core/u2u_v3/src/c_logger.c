//c_logger.c

#include "c_logger.h"

time_t timer;
struct tm* tm_info;
char time_stamp[26];


int logger(char* str, int level){
    static int index = 0;
    int return_val = 0;
    FILE* fptr;
    if (level<=LOG_LEVEL){
        const char* levels[] = {"CRITICAL", "ERROR", "WARNING", "INFO", "DEBUG", "NOTSET"};
        timer = time(NULL);
        tm_info = localtime(&timer);
        
        strftime(time_stamp, 26, "%Y-%m-%d %H:%M:%S", tm_info);
        fptr = fopen(LOGFILE, "a");
        if (fptr==NULL){
            printf("%s:: File write error: %s.\n", __func__, LOGFILE);
            return_val = -1;
        }else{
            fprintf(fptr, "%d [%s] %s: %s\n", index, time_stamp, levels[level], str);
            index++;
            //printf("write file: %s: %s\n", time_stamp, str);
        }
        fclose(fptr);
    }
    error_logger(str, level);
    return return_val;
}



int uart0_logger(char* str, int level){
    int return_val = 0;
    FILE* fptr;
    fptr = fopen(UART0_LOGFILE, "a");
    if (fptr==NULL){
        printf("%s:: File write error: %s.\n", __func__, UART0_LOGFILE);
        return_val = -1;
    }else{
        fprintf(fptr, "%s\n", str);
    }
    fclose(fptr);
    logger(str, level);
    return return_val;
}


int uart1_logger(char* str, int level){
    int return_val = 0;
    FILE* fptr;
    fptr = fopen(UART1_LOGFILE, "a");
    if (fptr==NULL){
        printf("%s:: File write error: %s.\n", __func__, UART1_LOGFILE);
        return_val = -1;
    }else{
        fprintf(fptr, "%s\n", str);
    }
    fclose(fptr);
    logger(str, level);
    return return_val;
}


int u2u_logger(char* str, int level){
    static int u2u_index = 0;
    int return_val = 0;
    FILE* fptr;
    if (level<=LOG_LEVEL){
        const char* levels[] = {"CRITICAL", "ERROR", "WARNING", "INFO", "DEBUG", "NOTSET"};
        fptr = fopen(U2U_LOGFILE, "a");
        if (fptr==NULL){
            printf("%s:: File write error: %s.\n", __func__, U2U_LOGFILE);
            return_val = -1;
        }else{
            fprintf(fptr, "(%d) %s\n", u2u_index, str);
            u2u_index++;
        }
        fclose(fptr);
    }
    logger(str, level);
    return return_val;
}


int uart_logger(char* str, int level){
    static int uart_index = 0;
    int return_val = 0;
    FILE* fptr;
    if (level<=LOG_LEVEL){
        const char* levels[] = {"CRITICAL", "ERROR", "WARNING", "INFO", "DEBUG", "NOTSET"};
        fptr = fopen(UART_LOGFILE, "a");
        if (fptr==NULL){
            printf("%s:: File write error: %s.\n", __func__, UART_LOGFILE);
            return_val = -1;
        }else{
            fprintf(fptr, "(%d) %s\n", uart_index, str);
            uart_index++;
        }
        fclose(fptr);
    }
    logger(str, level);
    return return_val;
}


int hal_logger(char* str, int level){
    static int hal_index = 0;
    int return_val = 0;
    FILE* fptr;
    if (level<=LOG_LEVEL){
        const char* levels[] = {"CRITICAL", "ERROR", "WARNING", "INFO", "DEBUG", "NOTSET"};
        fptr = fopen(HAL_LOGFILE, "a");
        if (fptr==NULL){
            printf("%s:: File write error: %s.\n", __func__, HAL_LOGFILE);
            return_val = -1;
        }else{
            fprintf(fptr, "(%d) %s\n", hal_index, str);
            hal_index++;
        }
        fclose(fptr);
    }
    logger(str, level);
    return return_val;
}


int message_logger(char* str, int level){
    static int message_index = 0;
    int return_val = 0;
    FILE* fptr;
    if (level<=LOG_LEVEL){
        const char* levels[] = {"CRITICAL", "ERROR", "WARNING", "INFO", "DEBUG", "NOTSET"};
        fptr = fopen(MESSAGE_LOGFILE, "a");
        if (fptr==NULL){
            printf("%s:: File write error: %s.\n", __func__, MESSAGE_LOGFILE);
            return_val = -1;
        }else{
            fprintf(fptr, "(%d) %s\n", message_index, str);
            message_index++;
        }
        fclose(fptr);
    }
    logger(str, level);
    return return_val;
}

int segment_logger(char* str, int level){
    static int segment_index = 0;
    int return_val = 0;
    FILE* fptr;
    if (level<=LOG_LEVEL){
        const char* levels[] = {"CRITICAL", "ERROR", "WARNING", "INFO", "DEBUG", "NOTSET"};
        fptr = fopen(SEGMENT_FLOW_LOGFILE, "a");
        if (fptr==NULL){
            printf("%s:: File write error: %s.\n", __func__, SEGMENT_FLOW_LOGFILE);
            return_val = -1;
        }else{
            fprintf(fptr, "(%d) %s\n", segment_index, str);
            segment_index++;
        }
        fclose(fptr);
    }
    logger(str, level);
    return return_val;
}

int queue_logger(char* str, int level){
    static int queue_index = 0;
    int return_val = 0;
    FILE* fptr;
    if (level<=LOG_LEVEL){
        const char* levels[] = {"CRITICAL", "ERROR", "WARNING", "INFO", "DEBUG", "NOTSET"};
        fptr = fopen(QUEUE_LOGFILE, "a");
        if (fptr==NULL){
            printf("%s:: File write error: %s.\n", __func__, LOGFILE);
            return_val = -1;
        }else{
            fprintf(fptr, "(%d) %s\n", queue_index, str);
            queue_index++;
        }
        fclose(fptr);
    }
    logger(str, level);
    return return_val;
}


int error_logger(char* str, int level){
    static int error_index = 0;
    int return_val = 0;
    FILE* fptr;
    if (level<=2){
        const char* levels[] = {"CRITICAL", "ERROR", "WARNING", "INFO", "DEBUG", "NOTSET"};
        fptr = fopen(ERROR_LOGFILE, "a");
        if (fptr==NULL){
            printf("%s:: File write error: %s.\n", __func__, ERROR_LOGFILE);
            return_val = -1;
        }else{
            fprintf(fptr, "(%d) %s\n", error_index, str);
            error_index++;
        }
        fclose(fptr);
    }
    return return_val;
}


int thread_logger(char* str, int level){
    static int thread_index = 0;
    int return_val = 0;
    FILE* fptr;
    if (level<=LOG_LEVEL){
        const char* levels[] = {"CRITICAL", "ERROR", "WARNING", "INFO", "DEBUG", "NOTSET"};
        fptr = fopen(THREAD_LOGFILE, "a");
        if (fptr==NULL){
            printf("%s:: File write error: %s.\n", __func__, THREAD_LOGFILE);
            return_val = -1;
        }else{
            fprintf(fptr, "(%d) %s\n", thread_index, str);
            thread_index++;
        }
        fclose(fptr);
    }
    return return_val;
}

int thread1_logger(char* str, int level){
    static int thread1_index = 0;
    int return_val = 0;
    FILE* fptr;
    if (level<=LOG_LEVEL){
        const char* levels[] = {"CRITICAL", "ERROR", "WARNING", "INFO", "DEBUG", "NOTSET"};
        fptr = fopen(THREAD1_LOGFILE, "a");
        if (fptr==NULL){
            printf("%s:: File write error: %s.\n", __func__, THREAD1_LOGFILE);
            return_val = -1;
        }else{
            fprintf(fptr, "(%d) %s\n", thread1_index, str);
            thread1_index++;
        }
        fclose(fptr);
    }
    return return_val;
}

int thread2_logger(char* str, int level){
    static int thread2_index = 0;
    int return_val = 0;
    FILE* fptr;
    if (level<=LOG_LEVEL){
        const char* levels[] = {"CRITICAL", "ERROR", "WARNING", "INFO", "DEBUG", "NOTSET"};
        fptr = fopen(THREAD2_LOGFILE, "a");
        if (fptr==NULL){
            printf("%s:: File write error: %s.\n", __func__, THREAD2_LOGFILE);
            return_val = -1;
        }else{
            fprintf(fptr, "(%d) %s\n", thread2_index, str);
            thread2_index++;
        }
        fclose(fptr);
    }
    return return_val;
}

int gm_thread_logger(char* str, int level){
    static int gm_thread_index = 0;
    int return_val = 0;
    FILE* fptr;
    if (level<=LOG_LEVEL){
        const char* levels[] = {"CRITICAL", "ERROR", "WARNING", "INFO", "DEBUG", "NOTSET"};
        fptr = fopen(GM_THREAD_LOGFILE, "a");
        if (fptr==NULL){
            printf("%s:: File write error: %s.\n", __func__, GM_THREAD_LOGFILE);
            return_val = -1;
        }else{
            fprintf(fptr, "(%d) %s\n", gm_thread_index, str);
            gm_thread_index++;
        }
        fclose(fptr);
    }
    return return_val;
}

int matrix_logger(char* str, int level){
    static int matrix_log_index = 0;
    int return_val = 0;
    FILE* fptr;
    if (level<=LOG_LEVEL){
        const char* levels[] = {"CRITICAL", "ERROR", "WARNING", "INFO", "DEBUG", "NOTSET"};
        fptr = fopen(MATRIX_LOG_FILE, "a");
        if (fptr==NULL){
            printf("%s:: File write error: %s.\n", __func__, MATRIX_LOG_FILE);
            return_val = -1;
        }else{
            fprintf(fptr, "(%d) %s\n", matrix_log_index, str);
            matrix_log_index++;
        }
        fclose(fptr);
    }
    return return_val;
}

int crc_logger(char* str, int level){
    static int crc_index = 0;
    int return_val = 0;
    FILE* fptr;
    if (level<=LOG_LEVEL){
        const char* levels[] = {"CRITICAL", "ERROR", "WARNING", "INFO", "DEBUG", "NOTSET"};
        fptr = fopen(CRC_LOGFILE, "a");
        if (fptr==NULL){
            printf("%s:: File write error: %s.\n", __func__, CRC_LOGFILE);
            return_val = -1;
        }else{
            fprintf(fptr, "(%d) %s\n", crc_index, str);
            crc_index++;
        }
        fclose(fptr);
    }
    return return_val;
}

