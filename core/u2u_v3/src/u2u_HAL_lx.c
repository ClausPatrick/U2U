//u2u_HAL_lx.c

#include "u2u_HAL_lx.h"

pthread_mutex_t keep_running_mutex = PTHREAD_MUTEX_INITIALIZER;
static volatile int keep_running = 0;
pthread_mutex_t is_running_mutex = PTHREAD_MUTEX_INITIALIZER;
static volatile int is_running = 0;

int serial_port;

pthread_t thread_id;

bool uart_is_readable(int port){
    int r = 0;
    return r;
}

char log_buffer[1024];
char log_buffer_thread[1024];

//message_ready = 0;

void uart0_irq_routine(void){
    int r;
    //gpio_put(LED_2, 1);
    uint8_t ch;
    while (uart_is_readable(uart0)){
        //ch = uart_getc(uart0);
//        if (ch==0){ //%%TEST%%
//            sprintf(log_buffer, "HAL: uart0_irq_routine ch: (%d).", ch); //%%TEST%%
//        }else{ //%%TEST%%
//            sprintf(log_buffer, "HAL: uart0_irq_routine ch: '%c'.", ch); //%%TEST%%
//        } //%%TEST%%
        //hal_logger(log_buffer, 4); //%%TEST%%
        r = uart0_character_processor(ch);
    }
}

uint8_t write_from_uart0(char* buffer, int buffer_lenght){
    //printf("UART_0 gmc: %d, lmc: %d, m: %s\n", message_counter_global, message_counter_port[0], buffer);
    sprintf(log_buffer, "HAL: write_from_uart0: %d, %s.", sizeof(buffer), buffer);
    hal_logger(log_buffer, 4);
    write(serial_port, buffer, buffer_lenght);
    //int r = uart_puts(uart0, buffer);
    //printf("%s\n", log_buffer);
    return 0;
}

uint8_t write_from_uart1(char* buffer, int buffer_length){
    //write(serial_port, buffer, sizeof(buffer));
    write_from_uart0(buffer, buffer_length);
    return 0;
}

void* read_task(void* _serial_port){
    static int rx_len = 0;
    int len_new = 0;
    char read_buf[256];
    int i = 1;
    int j;
    pthread_mutex_lock(&is_running_mutex);
    if (is_running){
        pthread_mutex_unlock(&is_running_mutex);
        sprintf(log_buffer_thread, "THREAD: read task already started."); //%%TEST%%
        thread_logger(log_buffer_thread, 2); //%%TEST%%
        pthread_exit(NULL);
    }else{
        is_running = 1;
        pthread_mutex_unlock(&is_running_mutex);
        sprintf(log_buffer_thread, "THREAD: read task started. kr: %d.", keep_running); //%%TEST%%
        thread_logger(log_buffer_thread, 4); //%%TEST%%
        while (1){
            len_new = read(serial_port, &read_buf, sizeof(read_buf));
            //len_new = read(*(int*)serial_port, &read_buf, sizeof(read_buf));
            read_buf[len_new] = 0;
        
            if (rx_len < 0) {
                printf("Error reading: %d", strerror(errno));
                sprintf(log_buffer_thread, "THREAD: read task  error reading serial: %d.", strerror(errno)); //%%TEST%%
                thread_logger(log_buffer_thread, 2); //%%TEST%%
                pthread_exit(NULL);
            }
          

            if (len_new > 0){
                //printf("Read %i bytes of %i. Received: %s\n", len_new, rx_len, read_buf);
                rx_len += len_new;
                int ch, r;
                for (ch=0; ch<len_new; ch++){
                    r = uart0_character_processor(read_buf[ch]);
                    //sprintf(log_buffer_thread, "THREAD: read task  uart0_character_processor ch: %c.", read_buf[ch]); //%%TEST%%
                    //thread_logger(log_buffer_thread, 4); //%%TEST%%
                }
            }

            pthread_mutex_lock(&keep_running_mutex);
            
            if (keep_running!=1){ 
                sprintf(log_buffer_thread, "THREAD: read task ended."); //%%TEST%%
                thread_logger(log_buffer_thread, 4); //%%TEST%%
                pthread_mutex_unlock(&keep_running_mutex);
                pthread_mutex_lock(&is_running_mutex);
                is_running = 0;
                pthread_mutex_unlock(&is_running_mutex);
                break;
            }
            pthread_mutex_unlock(&keep_running_mutex);
        }

        sprintf(log_buffer_thread, "THREAD: leaving read task."); //%%TEST%%
        thread_logger(log_buffer_thread, 4); //%%TEST%%
        //close(*(int*)serial_port);
        close(serial_port);
        pthread_exit(NULL);
    }
}

uint8_t write_from_uart(char* buffer){
    int r;
    r = write_from_uart0(buffer);
    return r;
}

int u2u_uart_setup(){
    serial_port = open("/dev/ttyS0", O_RDWR);
    if (serial_port < 0) {
        printf("Error %i from open: %d\n", errno, strerror(errno));
    }
    struct termios tty;
    if(tcgetattr(serial_port, &tty) != 0) {
        printf("Error %i from tcgetattr: %d\n", errno, strerror(errno));
        return 1;
    }
    tty.c_cflag &= ~PARENB; // Clear parity bit, disabling parity (most common)
    tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication (most common)
    tty.c_cflag &= ~CSIZE; // Clear all bits that set the data size
    tty.c_cflag |= CS8; // 8 bits per byte (most common)
    tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common)
    tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)
    tty.c_lflag &= ~ICANON;
    tty.c_lflag &= ~ECHO; // Disable echo
    tty.c_lflag &= ~ECHOE; // Disable erasure
    tty.c_lflag &= ~ECHONL; // Disable new-line echo
    tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
    tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes
    tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
    tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed
    // tty.c_oflag &= ~OXTABS; // Prevent conversion of tabs to spaces (NOT PRESENT ON LINUX)
    // tty.c_oflag &= ~ONOEOT; // Prevent removal of C-d chars (0x004) in output (NOT PRESENT ON LINUX)
    tty.c_cc[VTIME] = 10;    // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
    tty.c_cc[VMIN] = 10;
    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);
    
    // Save tty settings, also checking for error
    if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
        printf("Error %i from tcsetattr: %d\n", errno, strerror(errno));
        return 1;
    }

    keep_running = 1;
    pthread_create(&thread_id, NULL, read_task, &serial_port);

    //close(serial_port);
    return 0;
}


int u2u_uart_close(){
    sprintf(log_buffer, "HAL: u2u_uart_close: closing u2u_uart.");
    hal_logger(log_buffer, 4);
    pthread_mutex_lock(&keep_running_mutex);
    keep_running = 0;
    pthread_mutex_unlock(&keep_running_mutex);
    sprintf(log_buffer, "HAL: waiting for thread to collapse.");
    hal_logger(log_buffer, 4);
    pthread_join(thread_id, NULL);
    sprintf(log_buffer, "HAL: u2u_uart_close: thread collapsed.");
    hal_logger(log_buffer, 4);
    return 0;
}

