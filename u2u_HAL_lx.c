//u2u_HAL_lx.c

#include "u2u_HAL_lx.h"


pthread_mutex_t     keep_serial_running_mutex   = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t     keep_socket_running_mutex   = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t     is_serial_running_mutex     = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t     is_socket_running_mutex     = PTHREAD_MUTEX_INITIALIZER;
static volatile int keep_serial_running         = 0;
static volatile int keep_socket_running         = 0;
static volatile int is_serial_running           = 0;
static volatile int is_socket_running           = 0;

volatile sig_atomic_t keep_running              = 1;


int serial_port;
int socket_port;

char peer_ip_list[PEER_COUNT][20];
char peer_name_list[PEER_COUNT][32];
int peer_count;


pthread_t thread_id_serial_read;
pthread_t thread_id_socket_read;

//SELF_IP_ADDRESS, PEER_IP_LIST, PEER_NAME_LIST, PEER_COUNT.


bool uart_is_readable(int port){
    int r = 0;
    return r;
}

char log_buffer[1024];
char log_buffer_thread_1[1024];
char log_buffer_thread_2[1024];


void uart0_irq_routine(void){ }

void uart1_irq_routine(void){ }

uint8_t write_from_uart0(char* buffer, int buffer_length){
    write(serial_port, buffer, buffer_length);
    return 0;
}

int write_from_socket(char* ip, char* buffer, int buffer_length){
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    //char buffer[256];
    int ret_val = 0;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        ret_val += 1;
        sprintf(log_buffer, "HAL: write_from_socket: failed to open socket.");
        hal_logger(log_buffer, 2);
        return ret_val;
        //error("ERROR opening socket");
    }
    server = gethostbyname(ip);
    //server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        sprintf(log_buffer, "HAL: write_from_socket: host error.");
        hal_logger(log_buffer, 2);
        ret_val += 2;
        return ret_val;
        //exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(PORT);
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) {
        ret_val += 4;
        sprintf(log_buffer, "HAL: write_from_socket: Connection error.");
        hal_logger(log_buffer, 2);
        return ret_val;
    }
    n = write(sockfd, buffer, buffer_length);
    if (n < 0) {
        sprintf(log_buffer, "HAL: write_from_socket: writing error.");
        hal_logger(log_buffer, 2);
        ret_val += 8;
        return ret_val;
    }
    //n = read(sockfd,buffer,255);
    //if (n < 0)
    //     error("ERROR reading from socket");
    //printf("%s\n",buffer);
    close(sockfd);
    return ret_val;
}

/* Communication over uart1 is effectivly communicating via sockets to peers */
uint8_t write_from_uart1(char* buffer, int buffer_length){
    int ret_val, i;
    for (i=0; i<peer_count; i++){
        ret_val += write_from_socket(peer_ip_list[i], buffer, buffer_length);
    }
    return ret_val;
}

void* listen_task(void* _){
    pthread_mutex_lock(&is_socket_running_mutex);
    if (is_socket_running){
        pthread_mutex_unlock(&is_socket_running_mutex);
        sprintf(log_buffer_thread_2, "THREAD: listen task already started.");
        thread2_logger(log_buffer_thread_2, 2);
        pthread_exit(NULL);
    }else{
        is_socket_running = 1;
        pthread_mutex_unlock(&is_socket_running_mutex);
        sprintf(log_buffer_thread_2, "THREAD: listen task started.");
        thread2_logger(log_buffer_thread_2, 4);

        //int server_fd, read_buffer[245];
        int server_fd;
        char read_buffer[255];
        struct sockaddr_in address;
        int opt = 1;
        int addrlen = sizeof(address);
        char send_buffer[1024] = { 0 };
        static int rx_len = 0;
        int log_l_counter = 0;
        if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("socket failed");
            exit(EXIT_FAILURE);
        }
        sprintf(log_buffer_thread_2, "THREAD: Socket opened @fd:%d.", server_fd);
        thread2_logger(log_buffer_thread_2, 4);

        // Forcefully attaching socket to the port 8080
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
            perror("setsockopt");
            exit(EXIT_FAILURE);
        }
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(PORT);
        sprintf(log_buffer_thread_2, "THREAD: Socket address configuration for port %d.", PORT);
        thread2_logger(log_buffer_thread_2, 4);

        // Forcefully attaching socket to the port 8080
        if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            perror("bind failed");
            exit(EXIT_FAILURE);
        }

        if (listen(server_fd, 1) < 0) {
            perror("listen");
            exit(EXIT_FAILURE);
        }
        sprintf(log_buffer_thread_2, "THREAD: Socket set to listening state.");
        thread2_logger(log_buffer_thread_2, 4);
        while (keep_running){
            //if ((socket_port = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            if ((socket_port = accept(server_fd, (struct sockaddr*) NULL, NULL)) < 0) {
                if (errno == EINTR){
                    if (!keep_running){
                        break;
                    }
                }else{
                    perror("accept");
                    exit(EXIT_FAILURE);
                }
            }

            pid_t pid = fork();

            if (pid<0){
                close(socket_port);
                continue;
            }

            if (pid==0){ // Child process
                close(server_fd);

                int len_new = 0;
                len_new = read(socket_port, read_buffer, 254);

                if (len_new > 0){
                    //rx_len += len_new;
                    int ch, r;
                    for (ch=0; ch<len_new; ch++){
                        r = uart1_character_processor(read_buffer[ch]);
                    }
                }
                close(socket_port);
                exit(0);
            }else{ // Parent process
                close(socket_port);
            }

            pthread_mutex_lock(&keep_socket_running_mutex);
            if (!keep_running){
                break;
            }

            if (keep_socket_running!=1){
                pthread_mutex_unlock(&keep_socket_running_mutex);
                sprintf(log_buffer_thread_2, "THREAD: listen task ended.");
                thread2_logger(log_buffer_thread_2, 4);
                pthread_mutex_lock(&is_socket_running_mutex);
                is_socket_running = 0;
                pthread_mutex_unlock(&is_socket_running_mutex);
                break;
            }
            pthread_mutex_unlock(&keep_socket_running_mutex);
        }
        close(socket_port);
        shutdown(server_fd, SHUT_RDWR);
        pthread_exit(NULL);
    }
}

/* Reading from serial port and calling uart0_character_processor() for each character. */
void* read_task(void* _){
    static int rx_len = 0;
    int len_new = 0;
    char read_buffer[256];
    int i = 1;
    int j;
    pthread_mutex_lock(&is_serial_running_mutex);
    if (is_serial_running){
        pthread_mutex_unlock(&is_serial_running_mutex);
        sprintf(log_buffer_thread_1, "THREAD: read_task already started.");
        thread1_logger(log_buffer_thread_1, 2);
        pthread_exit(NULL);
    }else{
        is_serial_running = 1;
        pthread_mutex_unlock(&is_serial_running_mutex);
        while (1){
            len_new = read(serial_port, &read_buffer, sizeof(read_buffer));
            read_buffer[len_new] = 0;

            if (rx_len < 0) {
                printf("Error reading: %s", strerror(errno));
                pthread_exit(NULL);
            }

            if (len_new > 0){
                rx_len += len_new;
                int ch, r;
                for (ch=0; ch<len_new; ch++){
                    r = uart0_character_processor(read_buffer[ch]);
                }
            }

            pthread_mutex_lock(&keep_serial_running_mutex);
            if (keep_serial_running!=1){
                pthread_mutex_unlock(&keep_serial_running_mutex);
                pthread_mutex_lock(&is_serial_running_mutex);
                is_serial_running = 0;
                pthread_mutex_unlock(&is_serial_running_mutex);
                break;
            }
            pthread_mutex_unlock(&keep_serial_running_mutex);
        }

        //close(*(int*)serial_port);
        close(serial_port);
        pthread_exit(NULL);
    }
}

uint8_t write_from_uart(char* buffer, int buffer_length){
    int r;
    r = write_from_uart0(buffer, buffer_length);
    r = write_from_uart1(buffer, buffer_length);
    return r;
}


int ip_cmp(const char* ip1, const char* ip2){
    int i, j, ret_val;
// && (ip1!=0 && ip2!=0
    j = 0;
    for (i=0; i<MAX_IP_LEN; i++){
        if (ip1[i]==ip2[i]){
            if(ip1[i]==0 && ip2[i]==0){
                ret_val = 1;
                break;
            }
            j++;
        }else{
            ret_val = 0;
            break;
        }
    }
    return ret_val;
}

//SELF_IP_ADDRESS, PEER_IP_LIST, PEER_NAME_LIST, PEER_COUNT.

int peer_list_setup(){
    int i, j, ret_val;
    ret_val = 0;
    peer_count = 0;

    for (i=0; i<PEER_COUNT; i++){
        j=0;
        if (ip_cmp(SELF_IP_ADDRESS, PEER_IP_LIST[i])==0){
            while (PEER_IP_LIST[i][j]!=0 && j<MAX_IP_LEN){
                peer_ip_list[peer_count][j] = PEER_IP_LIST[i][j];
                j++;
            }
            peer_ip_list[peer_count][j] = 0;
            peer_count++;
        }
    }
    sprintf(log_buffer, "HAL: peer_list_setup list of peers: ");
    hal_logger(log_buffer, 4);
    for (i=0; i<peer_count; i++){
        sprintf(log_buffer, "     %s", peer_ip_list[i]);
        hal_logger(log_buffer, 4);
    }
    sprintf(log_buffer, "HAL: peer_list_setup self IP address: %s", SELF_IP_ADDRESS);
    hal_logger(log_buffer, 4);
    return ret_val;
}

int u2u_uart_setup(){
    sprintf(log_buffer, "HAL: u2u_uart_setup: loading peer_list.");
    hal_logger(log_buffer, 4);
    peer_list_setup();
    serial_port = open("/dev/ttyS0", O_RDWR);
    if (serial_port < 0) {
        //printf("Error %i from open: %s\n", errno, strerror(errno));
        sprintf(log_buffer, "HAL: u2u_uart_setup: Error %i from open: %s\n", errno, strerror(errno));
        hal_logger(log_buffer, 2);
    }
    struct termios tty;
    if(tcgetattr(serial_port, &tty) != 0) {
        //printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
        sprintf(log_buffer, "HAL: u2u_uart_setup: Error %i from tcgetattr: %s\n", errno, strerror(errno));
        hal_logger(log_buffer, 2);
        return 1;
    }
    tty.c_cflag &= ~PARENB; // Clear parity bit, disabling parity (most common)
    tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication (most common)
    tty.c_cflag &= ~CSIZE;  // Clear all bits that set the data size
    tty.c_cflag |= CS8;     // 8 bits per byte (most common)
    tty.c_cflag &= ~CRTSCTS;// Disable RTS/CTS hardware flow control (most common)
    tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)
    tty.c_lflag &= ~ICANON;
    tty.c_lflag &= ~ECHO;   // Disable echo
    tty.c_lflag &= ~ECHOE;  // Disable erasure
    tty.c_lflag &= ~ECHONL; // Disable new-line echo
    tty.c_lflag &= ~ISIG;   // Disable interpretation of INTR, QUIT and SUSP
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
    tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received byte
    tty.c_oflag &= ~OPOST;  // Prevent special interpretation of output bytes (e.g. newline chars)
    tty.c_oflag &= ~ONLCR;  // Prevent conversion of newline to carriage return/line feed
    // tty.c_oflag &= ~OXTABS; // Prevent conversion of tabs to spaces (NOT PRESENT ON LINUX)
    // tty.c_oflag &= ~ONOEOT; // Prevent removal of C-d chars (0x004) in output (NOT PRESENT ON LINUX)
    tty.c_cc[VTIME] = 32;   // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
    tty.c_cc[VMIN] = 0;
    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);

    // Save tty settings, also checking for error
    if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
        //printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
        sprintf(log_buffer, "HAL: u2u_uart_setup: Error %i from tcsetattr: %s\n", errno, strerror(errno));
        hal_logger(log_buffer, 2);
        return 1;
    }

    keep_running = 1;

    sprintf(log_buffer, "HAL: u2u_uart_setup: Starting serial thread.");
    hal_logger(log_buffer, 4);
    keep_serial_running = 1;
    pthread_create(&thread_id_serial_read, NULL, read_task, &serial_port);
    sprintf(log_buffer, "HAL: u2u_uart_setup:  Serial thread started.");
    hal_logger(log_buffer, 4);

    sprintf(log_buffer, "HAL: u2u_uart_setup: Starting socket thread.");
    hal_logger(log_buffer, 4);
    /* Socket setup here: */
    keep_socket_running = 1;
    pthread_create(&thread_id_socket_read, NULL, listen_task, &socket_port);

    sprintf(log_buffer, "HAL: u2u_uart_setup:  Socket thread started.");
    hal_logger(log_buffer, 4);
    //close(serial_port);
    sprintf(log_buffer, "HAL: u2u_uart_setup:  Setup completed.");
    hal_logger(log_buffer, 4);
    return 0;
}



int u2u_uart_close(){
    sprintf(log_buffer, "HAL: u2u_uart_close: closing u2u_uart.");
    hal_logger(log_buffer, 4);

    keep_running = 0;

    pthread_mutex_lock(&keep_serial_running_mutex);
    keep_serial_running = 0;
    pthread_mutex_unlock(&keep_serial_running_mutex);

    pthread_mutex_lock(&keep_socket_running_mutex);
    keep_socket_running = 0;
    pthread_mutex_unlock(&keep_socket_running_mutex);

    sprintf(log_buffer, "HAL: waiting for serial thread to collapse.");
    hal_logger(log_buffer, 4);
    pthread_join(thread_id_serial_read, NULL);
    sprintf(log_buffer, "HAL: waiting for socket thread to collapse.");
    hal_logger(log_buffer, 4);
    pthread_join(thread_id_socket_read, NULL);
    sprintf(log_buffer, "HAL: u2u_uart_close: threads collapsed.");
    hal_logger(log_buffer, 4);
    return 0;
}
