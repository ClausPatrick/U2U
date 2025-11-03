//u2u_HAL_lx.c

#include "u2u_HAL_lx.h"


const char* comm_log_file_location = "/home/pi/c_taal/u2u/hal/linux/logs"; // Todo:  check path via pwd and ammend

pthread_mutex_t     keep_serial_running_mutex   = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t     keep_socket_running_mutex   = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t     is_serial_running_mutex     = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t     is_socket_running_mutex     = PTHREAD_MUTEX_INITIALIZER;
static volatile int keep_serial_running         = 0;
static volatile int keep_socket_running         = 0;
static volatile int is_serial_running           = 0;
static volatile int is_socket_running           = 0;

volatile sig_atomic_t keep_running              = 1;


static int serial_port;
static int socket_port;
static int server_fd;

char peer_ip_list[PEER_COUNT][20];
char peer_name_list[PEER_COUNT][32];
int peer_count;


pthread_t thread_id_serial_read;
pthread_t thread_id_socket_read;

//SELF_IP_ADDRESS, PEER_IP_LIST, PEER_NAME_LIST, PEER_COUNT.

struct U2U_Options{
    bool port_enable[2][2];
    bool non_support_topic_response;
};

struct U2U_Options options;

bool uart_is_readable(int port){
    int r = 0;
    return r;
}

static char log_buffer[1024];
static char log_buffer_thread_1[1024];
static char log_buffer_thread_2[1024];


void uart0_irq_routine(void){ }
void uart1_irq_routine(void){ }


//void status_log(char* str, char* fid, char* label, int index){
//    FILE* fptr;
//    char file_name[255];
//    //pid_t thread_id = gettid();
//    pid_t thread_id = syscall(__NR_gettid); // Logging to unique file to avoid race condition.
//    sprintf(file_name, "/home/pi/c_taal/u2u/hal_lx_tester/logs/status_%s_id_%d%d.log", fid, 0, thread_id);
//    fptr = fopen(file_name, "a");
//    if (fptr==NULL){
//        printf("%s:: File write error\n.", __func__);
//        return;
//    }else{
//        fprintf(fptr, "(%d)(%d)(%s)[%s]  %s\n", index, thread_id, fid, label, str);
//        //printf("write file: %s: %s\n", time_stamp, str);
//    }
//    fclose(fptr);
//    return;
//}

uint8_t write_from_uart0(char* buffer, int buffer_length){
    if (options.port_enable[0][1]==1){
        //char fid[] = "write_from_uart0";
        //char label[] = "uart0";
        //static int s_index = 0;
        //status_log(buffer, fid, label, s_index);
        //s_index++;
        write(serial_port, buffer, buffer_length);
        sprintf(log_buffer, "HAL: %s:: %d, %s.", __func__, buffer_length, buffer);    //    %%TEST%%
        hal_logger(log_buffer, 4);    //    %%TEST%%
    }
    return 0;
}


int write_from_socket(char* ip, char* buffer, int buffer_length){
    if (options.port_enable[1][1]==1){
        int sockfd, n;
        struct sockaddr_in serv_addr;
        struct hostent *server;
        //char buffer[256];
        int ret_val = 0;
        //char fid[] = "write_from_socket";
        //char label[] = "socket";
        //static int s_index = 0;
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            ret_val += 1;
            sprintf(log_buffer, "HAL: %s:: write_from_socket: failed to open socket.", __func__);
            hal_logger(log_buffer, 2);
            return ret_val;
            //error("ERROR opening socket");
        }
        server = gethostbyname(ip);
        //server = gethostbyname(argv[1]);
        if (server == NULL) {
            fprintf(stderr,"ERROR, no such host\n");
            sprintf(log_buffer, "HAL: %s:: host error.", __func__);
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
            sprintf(log_buffer, "HAL: %s:: Connection error: %s.", __func__, ip);
            hal_logger(log_buffer, 2);
            return ret_val;
        }
        n = write(sockfd, buffer, buffer_length);
        //status_log(buffer, fid, label, s_index);
        //s_index++;
    
        if (n < 0) {
            sprintf(log_buffer, "HAL: %s:: writing error.", __func__);
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
    return 0;
}



/* Communication over uart1 is effectivly communicating via sockets to peers */
uint8_t write_from_uart1(char* buffer, int buffer_length){
    int ret_val, i;
    sprintf(log_buffer, "HAL: %s::  %d, %s.", __func__, buffer_length, buffer);    //    %%TEST%%
    hal_logger(log_buffer, 4);    //    %%TEST%%
    for (i=0; i<peer_count; i++){
        ret_val += write_from_socket(peer_ip_list[i], buffer, buffer_length);
    }
    return ret_val;
}

/* Read in from socket (a.k.a uart1)*/
//void* listen_task(void* _){ // Changed name to socket_in_task.
void* socket_in_task(void* _){
    pthread_mutex_lock(&is_socket_running_mutex);
    if (is_socket_running){
        pthread_mutex_unlock(&is_socket_running_mutex);
        sprintf(log_buffer_thread_2, "THREAD: %s:: listen task already started.", __func__);
        thread2_logger(log_buffer_thread_2, 2);
        pthread_exit(NULL);
    }else{
        is_socket_running = 1;
        pthread_mutex_unlock(&is_socket_running_mutex);
        sprintf(log_buffer_thread_2, "THREAD: %s:: listen task started.", __func__);
        thread2_logger(log_buffer_thread_2, 4);

        //int server_fd, read_buffer[245];
        char read_buffer[1024];
        struct sockaddr_in address;
        int opt = 1;
        //int addrlen = sizeof(address);
        //static int rx_len = 0;
        //int log_l_counter = 0;

        //char fid[] = "socket_in_task";
        //char label[] = "socket";
        //static int s_index = 0;


        if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("socket failed");
            exit(EXIT_FAILURE);
        }
        sprintf(log_buffer_thread_2, "THREAD: %s:: Socket opened @fd:%d.", __func__, server_fd);
        thread2_logger(log_buffer_thread_2, 4);
  
        // Forcefully attaching socket to the port 8080
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
            perror("setsockopt");
            exit(EXIT_FAILURE);
        }
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(PORT);
        sprintf(log_buffer_thread_2, "THREAD: %s:: Socket address configuration for port %d.", __func__, PORT);
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
        sprintf(log_buffer_thread_2, "THREAD: %s:: Socket set to listening state. Entering main task loop.", __func__);
        thread2_logger(log_buffer_thread_2, 4);
        while (keep_running){
            if (options.port_enable[1][0]==1){
                if ((socket_port = accept(server_fd, (struct sockaddr*) NULL, NULL)) < 0) {
                    if (errno == EINTR){
                        if (!keep_running){ 
                            sprintf(log_buffer_thread_2, "THREAD: %s:: Request to break main loop.", __func__);
                            thread2_logger(log_buffer_thread_2, 4);
                            break; 
                        }
                    }else if (errno == EBADF || ECONNABORTED){
                        sprintf(log_buffer_thread_2, "THREAD: %s:: Shutdown invoked.", __func__);
                        thread2_logger(log_buffer_thread_2, 4);
                        break;
                    }else{
                        perror("accept");
                        exit(EXIT_FAILURE);
                    }
                }
                pid_t pid = fork();
                if (pid>0){ //if (pid<0){ 
                    close(socket_port);
                    continue; 
                }
                if (pid==0){ // Child process
                    close(server_fd);
                    int len_new = 0;
                    len_new = read(socket_port, read_buffer, 253);
                    if (len_new > 0){
                        int ch;
                        read_buffer[254] = 0;
                        for (ch=0; ch<len_new; ch++){
                            //r = uart0_character_processor(read_buffer[ch]);
                            uart1_character_processor(read_buffer[ch]);
                        }
                    }
                    close(socket_port);
                    exit(0);
                }else{ // Parent process
                    close(socket_port);
                }

                if (!keep_running){
                    sprintf(log_buffer_thread_2, "%s:: THREAD: Request received to break task loop.", __func__);
                    thread2_logger(log_buffer_thread_2, 4);
                    break;
                }                 
                pthread_mutex_lock(&keep_socket_running_mutex);
                if (keep_socket_running!=1){ 
                    pthread_mutex_unlock(&keep_socket_running_mutex);
                    sprintf(log_buffer_thread_2, "THREAD: %s:: listen task ended.", __func__);
                    thread2_logger(log_buffer_thread_2, 4);
                    pthread_mutex_lock(&is_socket_running_mutex);
                    is_socket_running = 0;
                    pthread_mutex_unlock(&is_socket_running_mutex);
                    break;
                }
                pthread_mutex_unlock(&keep_socket_running_mutex);
            }else{ // Port is disabled
                pthread_mutex_lock(&keep_socket_running_mutex);
                if (keep_socket_running!=1){ 
                    pthread_mutex_unlock(&keep_socket_running_mutex);
                    sprintf(log_buffer_thread_2, "THREAD: %s:: listen task ended while port was disabled.", __func__);
                    thread2_logger(log_buffer_thread_2, 4);
                    pthread_mutex_lock(&is_socket_running_mutex);
                    is_socket_running = 0;
                    pthread_mutex_unlock(&is_socket_running_mutex);
                    break;
                }
                pthread_mutex_unlock(&keep_socket_running_mutex);
            }
        }
        close(socket_port);
        shutdown(server_fd, SHUT_RDWR);
        pthread_exit(NULL);
    }
}        
 
/* Reading from serial port and calling uart0_character_processor() for each character. */
//void* read_task(void* _){ // Changed name to serial_in_task
void* serial_in_task(void* _){
    int len_new = 0;
    char read_buffer[1024];
    int ch;
    pthread_mutex_lock(&is_serial_running_mutex);
    if (is_serial_running){
        pthread_mutex_unlock(&is_serial_running_mutex);
        sprintf(log_buffer_thread_1, "THREAD: %s:: serial_in_task already started.", __func__);
        thread1_logger(log_buffer_thread_1, 2);
        pthread_exit(NULL);
    }else{
        is_serial_running = 1;
        pthread_mutex_unlock(&is_serial_running_mutex);
        sprintf(log_buffer_thread_1, "THREAD: %s:: Entering main serial task loop.", __func__);
        thread1_logger(log_buffer_thread_1, 2);
        while (1){
            if (options.port_enable[0][0]==1){

                len_new = read(serial_port, &read_buffer, sizeof(read_buffer));
                read_buffer[len_new] = 0;
            
                if (len_new < 0) {
                    printf("Error reading: %s", strerror(errno));
                    pthread_exit(NULL);
                }
                if (len_new > 0){
                    for (ch=0; ch<len_new; ch++){
                        char new_c = read_buffer[ch];
                        //r = uart0_character_processor(new_c);
                        uart0_character_processor(new_c);
                    }
                }
    
                pthread_mutex_lock(&keep_serial_running_mutex);
                if (keep_serial_running!=1){ 
                    pthread_mutex_unlock(&keep_serial_running_mutex);
                    pthread_mutex_lock(&is_serial_running_mutex);
                    is_serial_running = 0;
                    sprintf(log_buffer_thread_1, "THREAD: %s:: Request to quit serial task loop.", __func__);
                    thread1_logger(log_buffer_thread_1, 2);
                    pthread_mutex_unlock(&is_serial_running_mutex);
                    break;
                }
                pthread_mutex_unlock(&keep_serial_running_mutex);
            }else{ // Port disabled
                if (keep_serial_running!=1){ 
                    pthread_mutex_unlock(&keep_serial_running_mutex);
                    pthread_mutex_lock(&is_serial_running_mutex);
                    is_serial_running = 0;
                    pthread_mutex_unlock(&is_serial_running_mutex);
                    break;
                }
                pthread_mutex_unlock(&keep_serial_running_mutex);
            }
        }
        sprintf(log_buffer_thread_1, "THREAD: %s:: exiting serial task.", __func__);
        thread1_logger(log_buffer_thread_1, 2);
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
    sprintf(log_buffer, "HAL: %s:: peer_list_setup list of peers: ", __func__);
    hal_logger(log_buffer, 4);
    for (i=0; i<peer_count; i++){
        sprintf(log_buffer, "     %s", peer_ip_list[i]);
        hal_logger(log_buffer, 4);
    }
    sprintf(log_buffer, "HAL: %s:: peer_list_setup self IP address: %s", __func__, SELF_IP_ADDRESS);
    hal_logger(log_buffer, 4);
    return ret_val;
}

int set_port(int port, bool dir, bool state){
    if (port<0 || port>1){
        return 1;
    }
    options.port_enable[port][dir] = state;
    return 0;
}

/* Flag for comm logging pdr:
 * bit 0 [1]: PORT, port value.
 * bit 1 [2]: DIR, {0: inbound, 1: outbound}.
 * bit 2 [4]: RFLAG, {For DIR = 0: 0: self or gen addressed, 1: other and forwarded}.
 *               {For DIR = 1: 0: message response,      1: message forward}.
 * bit 3 [8]: ORG, {0: auto,    1: external}.
 *
 * */


/*Called from U2U from functions: log_outbound_message, log_inbound_message */
void comm_logger(char* buffer, int buffer_length, int pdr){
    FILE* fptr;
    char file_name[255];
    int port = 0;
    int dir = 0;
    //char label[] = "label";
    char* in_or_out[2] = {"INBOUND", "OUTBOUND"};

    if (pdr & 0b00000001){ port = 1; }
    if (pdr & 0b00000010){ dir = 1; }
    //pid_t thread_id = gettid();
    pid_t thread_id = syscall(__NR_gettid); // Logging to unique file to avoid race condition.

    //sprintf(file_name, "/home/pi/c_taal/u2u/hal_lx_tester/logs/comms_0%d_0%d.log", thread_id, pdr);
    sprintf(file_name, "%s/comms_0%d_0%d.log", comm_log_file_location, thread_id, pdr);


    fptr = fopen(file_name, "a");
    if (fptr==NULL){
        printf("%s:: File write error '%s'.\n", __func__, file_name);
        return;
    }else{
        fprintf(fptr, "(PORT %d)(%s)(%d)(%d) #_COMM_#%s#_COMM_END_#\n", port, in_or_out[dir], thread_id, pdr, buffer);
        //printf("write file: %s: %s\n", time_stamp, str);
    }
    fclose(fptr);

    sprintf(log_buffer, "HAL: %s:: filename: %s, buffer: %s, len: %d, pdr: %d.", __func__, buffer, file_name, buffer_length, pdr);
    hal_logger(log_buffer, 2);

    return;
}

//void status_log(char* str, char* fid, char* label, int index){
//    FILE* fptr;
//    char file_name[255];
//
//    //pid_t thread_id = gettid();
//    pid_t thread_id = syscall(__NR_gettid); // Logging to unique file to avoid race condition.
//
//    sprintf(file_name, "/home/pi/c_taal/u2u/hal_lx_tester/logs/status_%s_id_%d%d.log", fid, 0, thread_id);
//
//
//    fptr = fopen(file_name, "a");
//    if (fptr==NULL){
//        printf("File write error\n");
//        return;
//    }else{
//        fprintf(fptr, "(%d)(%d)(%s)[%s]  %s\n", index, thread_id, fid, label, str);
//        //printf("write file: %s: %s\n", time_stamp, str);
//    }
//    fclose(fptr);
//    return;
//}

void inbound_message_logger(char* buffer, int buffer_length){
    sprintf(log_buffer, "%s.", buffer);  
    u2u_logger(log_buffer, 4);          
    sprintf(log_buffer, "HAL: %s:: buffer: %s, len: %d.", __func__, buffer, buffer_length);
    hal_logger(log_buffer, 2);

    return;
}

//int set_port(int port, bool dir, bool state)
int u2u_uart_setup(){
    int ret_val = 0; 
    ret_val = (ret_val * 1) + set_port(0, 0, 1); 
    ret_val = (ret_val * 2) + set_port(0, 1, 1);
    ret_val = (ret_val * 4) + set_port(1, 0, 1);
    ret_val = (ret_val * 8) + set_port(1, 1, 1);
        
    if (ret_val==0){
        sprintf(log_buffer, "HAL: %s::  ports enabled.", __func__);
        hal_logger(log_buffer, 4);
    }else{
        sprintf(log_buffer, "HAL: %s::  ports enabling failed: %d.", __func__, ret_val);
        hal_logger(log_buffer, 2);
    }


    sprintf(log_buffer, "HAL: %s::  loading peer_list.", __func__);
    hal_logger(log_buffer, 4);
    peer_list_setup();
    serial_port = open("/dev/ttyS0", O_RDWR);
    if (serial_port < 0) {
        //printf("Error %i from open: %s\n", errno, strerror(errno));
        sprintf(log_buffer, "HAL: %s::  Error %i from open: %s\n", __func__, errno, strerror(errno));
        hal_logger(log_buffer, 2);
    }
    struct termios tty;
    if(tcgetattr(serial_port, &tty) != 0) {
        //printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
        sprintf(log_buffer, "HAL: %s:: Error %i from tcgetattr: %s\n", __func__, errno, strerror(errno));
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
    tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes
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
        sprintf(log_buffer, "HAL: %s:: Error %i from tcsetattr: %s\n", __func__, errno, strerror(errno));
        hal_logger(log_buffer, 2);
        return 1;
    }

    keep_running = 1;

    sprintf(log_buffer, "HAL: %s:: Starting serial thread.", __func__);
    hal_logger(log_buffer, 4);
    keep_serial_running = 1;
    pthread_create(&thread_id_serial_read, NULL, serial_in_task, &serial_port);
    sprintf(log_buffer, "HAL: %s:: Serial thread started.", __func__);
    hal_logger(log_buffer, 4);

    sprintf(log_buffer, "HAL: %s:: Starting socket thread.", __func__);
    hal_logger(log_buffer, 4);
    /* Socket setup here: */
    keep_socket_running = 1;
    pthread_create(&thread_id_socket_read, NULL, socket_in_task, &socket_port);

    sprintf(log_buffer, "HAL: %s:: Socket thread started.", __func__);
    hal_logger(log_buffer, 4);
    //close(serial_port);
    sprintf(log_buffer, "HAL: %s:: Setup completed.", __func__);
    hal_logger(log_buffer, 4);
    return 0;
}



int u2u_uart_close(){
    sprintf(log_buffer, "HAL: %s:: closing u2u_uart.", __func__);
    hal_logger(log_buffer, 4);

    keep_running = 0;

    pthread_mutex_lock(&keep_serial_running_mutex);
    keep_serial_running = 0;
    pthread_mutex_unlock(&keep_serial_running_mutex);
    
    pthread_mutex_lock(&keep_socket_running_mutex);
    keep_socket_running = 0;
    pthread_mutex_unlock(&keep_socket_running_mutex);

    sprintf(log_buffer, "HAL: %s:: waiting for serial thread to collapse.", __func__);
    hal_logger(log_buffer, 4);
    pthread_join(thread_id_serial_read, NULL);
    shutdown(server_fd, SHUT_RDWR);
    close(server_fd);
    sprintf(log_buffer, "HAL: %s:: waiting for socket thread to collapse.", __func__);
    hal_logger(log_buffer, 4);
    pthread_join(thread_id_socket_read, NULL);
    sprintf(log_buffer, "HAL: %s:: threads collapsed.", __func__);
    hal_logger(log_buffer, 4);
    return 0;
}
