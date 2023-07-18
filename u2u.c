//u2u.c
#include "u2u.h"
//#include "u2uclientdef.h"


#define COLONS_TOTAL        0
#define COLONS_SUCCESSIVE   1
#define RESPOND_ON_BAD_CRC  1


struct Message_Queue {
    int buffer[MAX_MESSAGE_KEEP];
    int front;
    int rear;
    int count;
} ;

struct Message_Queue Queue;



static char log_buffer[1024];

/* String constants against which topics are compared */
const char* topic_list[]    = { "HAIL", "HELP", "SET LCD", "SET OLED", "GET SENSOR", "SET ENCODER", "GET ENCODER", "SET LED", "SET TIME", "GET TIME", "SET DATE", "GET DATE", "RESERVED 0", "RESERVED 1", "RESERVED 2", "RESERVED 3", "RESERVED 4"};
/* Topic number corresponds to indices of char pointers. These can be overwritten in u2u_topic_exchange().*/
const char* msg_responses[] = {msg_hail_response, msg_help_response, msg_set_lcd_response, msg_set_oled_response, msg_get_sensor_response, msg_set_encoder_response, msg_get_encoder_response, msg_set_led_response, msg_set_time_response, msg_get_time_response, msg_set_date_response, msg_get_date_response, msg_RESERVED_0, msg_RESERVED_1, msg_RESERVED_2, msg_RESERVED_3, msg_RESERVED_4} ;

struct Message mi0, mi1, mi2, mi3, mi4, mi5, mi6, mi7;

const unsigned char CRC7_POLY = 0x91;
//
//const uint8_t sg_WAIT_INDEX         = 0;
//const uint8_t sg_PREMESSAGE_INDEX   = 0;
//const uint8_t sg_SENDER_INDEX       = 1;
//const uint8_t sg_RECEIVER_INDEX     = 2;
//const uint8_t sg_RQS_INDEX          = 3;
//const uint8_t sg_TOPIC_INDEX        = 4;
//const uint8_t sg_CHAPTER_INDEX      = 5;
//const uint8_t sg_LENGTH_INDEX       = 6;
//const uint8_t sg_PAYLOAD_INDEX      = 7;
//const uint8_t sg_HOPCOUNT_INDEX     = 8;
//const uint8_t sg_CRC_INDEX          = 9;
//
const char message_segment_labels[12][32] = {"#Premessage#", "#Sender#", "#Receiver#",  "#RQS#",  "#Topic#",  "#Chapter#",  "#Payload_Length#",  "#Payload_Data#",  "#Hopcount#",  "#CRC#", "#ENDOFMESSAGE#"};

//const char RQS_r[] = "RS";
//const char RQS_q[] = "RQ";
//const char RQS_i[] = "RI";
//const char RQS_n[] = "NA";

int rx_id                               = 0;
int tx_id                               = 0;

uint8_t  message_counter_global         = 0;
uint8_t  segment_counter[2]             = {0, 0};
uint8_t  segment_length[2]              = {0, 0};
uint8_t  message_counter_port[2]        = {0, 0};
const uint8_t SEGMENT_MAX_LENGTH[]      = {1, 32, 32, 8, 16, 8, 4, MAX_MESSAGE_PAYLOAD_SIZE, 4, 4};
uint8_t  colon_counter[2][2]            = {{0, 0}, {0, 0}}; //[port][successive/total]
bool is_colon[2]                        = {0, 0};
bool was_colon[2]                       = {0, 0};

int message_index_queue[MAX_MESSAGE_KEEP] = { -1 };

uint8_t  (*parse_function_array[13])(uint8_t, int, char);
uint8_t  (*call_router_functions[8])(uint8_t, int);
uint8_t  (*uart_write_functions[2])(char*);

char payloads[MAX_MESSAGE_KEEP][MAX_MESSAGE_PAYLOAD_SIZE];
char payloads_out[MAX_MESSAGE_KEEP][MAX_MESSAGE_PAYLOAD_SIZE];

int ascii_to_int(char* str){
    int len = 0;
    int sum = 0;
    int decs[] = {1, 10, 100, 1000, 10000, 100000, 1000000};
    while (str[len] != 0){ len++; }
    for (int c=0; c<len; c++){
        if (str[c]>=48 && str[c]<58){
            sum = sum + (decs[len-c-1] * (str[c]-48));
        }
    }
    return sum;
}

void int_to_ascii(char* buf, int integer, int p) { // If p is 0 then p is determined.
        int tens[] = {10000000, 1000000, 100000, 10000, 1000, 100, 10, 1};
        int f = 0; //p denotes 'precision' or number of digits.
        int c = 0;
        int j = 0;
        const int M = 7;
        if (integer==0){ // Going to be zero often, so cutting this short.
            buf[0] = 48;
            buf[1] = '\0';
        }else{
            if (p==0){
                for (j=M; j>0; --j){
                    if (integer >= tens[j]){ p++; }else{ break; }
                }
            }
            f = M-p;
            for (c=0; c<p; c++) {
                buf[c] = 48 + (integer % tens[f+c] / (tens[f+c+1]));
            }
            buf[c] = '\0';
        }
        return;
}

int float_to_ascii(char* buf, float f, int p){
    float tens[] = {1., 10., 100., 1000., 10000., 100000.};
    int c = (int) f;
    float d = f - c;

    int i, j = 0;
    char tempbuf[5];
    int high_part = (int) f;
    int low_part = (int) (tens[p]*(f - high_part));
    bool isleading = 1;
    int_to_ascii(tempbuf, high_part, 3);
    for (i=0; i<3; i++){ // Loop to filter out leading zeros.
        if (tempbuf[i]=='0' && isleading){
        }else{
            isleading = 0;
            buf[j] = tempbuf[i];
            j++;
        }
    }
    buf[j] = '.';
    j++;
    int_to_ascii(tempbuf, low_part, p);
    for (i=0; i<3; i++){
        buf[j] = tempbuf[i];
        j++;
    }
    buf[j] = 0;
}

int len(const char* s){
    int i = 0;
    while (s[i] != 0){ i++; }
    return i;
}

int cmp(char* s1, const char* s2){
    /*Return True if strings compare equal, False if not or if empty.
     * Assuming null termination.*/
    int l1 = len(s1);
    int l2 = len(s2);
    int r = 0;
    int i = 0;
    if (l1==l2){
        if (l1==0){ return r;}
        else{
            r = 1;
            for (int c=0; c<l1; c++){
                r = r * ((int) s1[c] == (int) s2[c]);
            }
        }
    }
    return r;
}

int copy_str(char* buffer, const char* s, int index){
    /*Copy over string items into a buffer.
     * Provide index = 0 for simple 1-1 copy, for concatting together items
     * into one buffer, the returned index can be provided for next item.
     * Assuming null termination.*/
    int i = 0;
    while (s[i] !=0){
        buffer[index] = s[i];
        i++;
        index++;
    }
    return index;
}

char get_crc(char* buffer, char length){
    char i, j, crc = 0;
    for (i=0; i<length; i++){
        crc ^= buffer[i];
        for (j=0; j<8; j++){
            if (crc & 1){ crc ^= CRC7_POLY; }
            crc >>= 1;
        }
    }
    sprintf(log_buffer, "U2U: CRC calculated: %d, for lenght: %d.", crc, length);
    u2u_logger(log_buffer, 4);
    sprintf(log_buffer, "\tbuffer:\"%s\".", buffer);
    u2u_logger(log_buffer, 4);
    //return length;
    return crc;
}

void log_status(int port, char* func_name, char ch){
    if (ch==0){
        sprintf(log_buffer, "U2U: %.6s, prt, ch, MGC, MCP, SC, SL:  %.1d, (%d), %.3d, %.3d, %.3d, %.3d", func_name, port, ch,  message_counter_global, message_counter_port[port], segment_counter[port], segment_length[port]);
    }else{
        sprintf(log_buffer, "U2U: %.6s, prt, ch, MGC, MCP, SC, SL:  %.1d, '%c', %.3d, %.3d, %.3d, %.3d", func_name, port, ch,  message_counter_global, message_counter_port[port], segment_counter[port], segment_length[port]);
    }
    segment_logger(log_buffer, 4);
}


int in_queue(struct Message_Queue *queue, int data) {
    if (queue->count == MAX_MESSAGE_KEEP) {
        // Overflow condition, wrap around and overwrite oldest data
        queue->front = (queue->front + 1) % MAX_MESSAGE_KEEP;
    } else {
        queue->count++;
    }

    queue->buffer[queue->rear] = data;
    queue->rear = (queue->rear + 1) % MAX_MESSAGE_KEEP;
    //print();
    //printf("\n");
    return 0;
}

int out_queue(struct Message_Queue *queue, int *data) {
    if (queue->count == 0) {
        // Queue is empty
        return 1;
    }

    *data = queue->buffer[queue->front];
    queue->front = (queue->front + 1) % MAX_MESSAGE_KEEP;
    queue->count--;

    //print();
    //printf("\n");
    return 0;
}


uint8_t message_clear(uint8_t message_index, int port){
    uint8_t return_val;
    return_val              = 0;
    segment_counter[port]   = 0;
    segment_length[port]    = 0;
    colon_counter[port][0]  = 0;
    colon_counter[port][1]  = 0;
    is_colon[port]          = 0;
    was_colon[port]         = 0;
    return return_val;
}

uint8_t message_reset(uint8_t message_index, int port){
    uint8_t return_val;
    if (message_counter_global<=0){
        sprintf(log_buffer, "U2U: message_counter_global under-count during message_reset function.");
        u2u_logger(log_buffer, 2);
    }else{ message_counter_global--; }
    return_val = message_clear(message_index, port);
    return return_val;
}

void colon_parser(uint8_t message_index, int port){ //Appending NULL char onto last pos in segment.
    if (segment_counter[port]>0){
        messages[message_index]->Segments[segment_counter[port]][segment_length[port]] = 0; // Null terminating each segment.
    }
    segment_counter[port]++;
    segment_length[port] = 0;
    sprintf(log_buffer, "U2U: colon_parser MGC, MCP, SC, SL: %.3d, %.3d, %.3d, %.3d", message_counter_global, message_counter_port[port], segment_counter[port], segment_length[port]);
    segment_logger(log_buffer, 4);
}

bool colon_checker(uint8_t message_index, int port, char ch){ //message_index is ignored
    uint8_t return_val;
    //log_status(port, "_check", ch); //%%TEST%%
    //log_status(port, "_checker", ch); //%%TEST%%
    if (ch == ':'){
        colon_counter[port][COLONS_TOTAL]++;
        is_colon[port] = 1;
        if (was_colon[port]==1){
            colon_counter[port][COLONS_SUCCESSIVE]++;
            if (segment_counter[port]>2){
                was_colon[port] = 0;
                is_colon[port] = 0;
                //log_status(port, "_chec!", ch); //%%TEST%%
                //return_val = message_clear(message_index, port); //%%TEST%%
                return_val = message_reset(message_index, port);
            }
        }
    }else{
        //colon_counter[port][COLONS_TOTAL] = 0;
        is_colon[port] = 0;
    }
    was_colon[port] = is_colon[port];
    return is_colon[port];
}

uint8_t premessage_setup(uint8_t message_index, int port, char ch){ // message_index is ignored and message_counter_global is written into ~counter_port[~].
    struct Message* new_message;
    if (colon_checker(message_index, port, ch)){ // Here, message_index doesn't matter.
        /* Here, message_index and message_counter_port are updated and will be referenced */

    }else{ // Here only proceed if two or more colons are received before a character is received.
        if (colon_counter[port][COLONS_SUCCESSIVE]>=1){
            message_index = message_counter_global;
            message_counter_port[port] = message_index;
            if (message_counter_global>=MAX_MESSAGE_KEEP){
                printf("Message registers overflow\n");
                sprintf(log_buffer, "U2U: premessage_setup Message registers overflow."); //%%TEST%%
                u2u_logger(log_buffer, 2); //%%TEST%%
            }
            //sprintf(log_buffer, "U2U: premessage_setup Message 1 MCG: %d.", message_counter_global); //%%TEST%%
            //u2u_logger(log_buffer, 4); //%%TEST%%
            message_counter_global = (message_counter_global + 1) % MAX_MESSAGE_KEEP;
            //sprintf(log_buffer, "U2U: premessage_setup Message 2 MCG: %d.", message_counter_global); //%%TEST%%
            //u2u_logger(log_buffer, 2); //%%TEST%%
            new_message = messages[message_index];
            new_message->Index = message_index;
            new_message->Port = port;
            new_message->Payload = payloads[message_index];
            colon_parser(message_index, port);
            new_message->CRC_buffer[0] = ':';
            new_message->CRC_buffer[1] = ':';
            new_message->CRC_buffer[2] = ch;
            new_message->CRC_index = 3;
            new_message->CRC_check = 0;

            // Writing first character (and the only one in this function).
            new_message->Segments[segment_counter[port]][segment_length[port]] = ch;
            segment_length[port] = 1;
            //segment_length[port] = 0;
            //segment_length[port] = (segment_length[port] + 1) % SEGMENT_MAX_LENGTH[segment_counter[port]];
            //segment_length[port] = (segment_length[port] + 1) % MAX_MESSAGE_SIZE;
            //segment_length[port] = (segment_length[port] + 1) % 255;
        }
    }
    return 0;
}

uint8_t write_into_segment(uint8_t message_index, int port, char ch){
    uint8_t return_val = 0;
    struct Message* new_message = messages[message_index];
    new_message->ID = rx_id;
    rx_id++;
    new_message->CRC_buffer[new_message->CRC_index] = ch;
    new_message->CRC_index++;
    if (colon_checker(message_index, port, ch)){
        colon_parser(message_index, port);
    }else{
        new_message->Segments[segment_counter[port]][segment_length[port]] = ch;
        segment_length[port] = (segment_length[port] + 1) % SEGMENT_MAX_LENGTH[segment_counter[port]];
    }
    return return_val;
}

uint8_t write_into_payload_length(uint8_t message_index, int port, char ch){
    uint8_t return_val = 0;
    struct Message* new_message = messages[message_index];
    new_message->CRC_buffer[new_message->CRC_index] = ch;
    new_message->CRC_index++;
    if (colon_checker(message_index, port, ch)){
        char* pl_length;
        int int_pl_len;
        new_message->Segments[segment_counter[port]][segment_length[port]] = 0;
        pl_length = new_message->Segments[segment_counter[port]];
        int_pl_len = ascii_to_int(pl_length);
        new_message->intLength = int_pl_len;
        colon_parser(message_index, port);
    }else{
        new_message->Segments[segment_counter[port]][segment_length[port]] = ch;
        segment_length[port] = (segment_length[port] + 1) % SEGMENT_MAX_LENGTH[segment_counter[port]];
        return_val = 0;
    }
    return return_val;
}

uint8_t write_into_payload_data(uint8_t message_index, int port, char ch){
    struct Message* new_message = messages[message_index];
    new_message->CRC_buffer[new_message->CRC_index] = ch;
    new_message->CRC_index++;
    uint8_t return_val = 0;
    if (segment_length[port] > new_message->intLength-1){
        new_message->Payload[segment_length[port]] = 0;
        colon_parser(message_index, port);
    }else{
        new_message->Payload[segment_length[port]] = ch;
        segment_length[port] = (segment_length[port] + 1) % SEGMENT_MAX_LENGTH[segment_counter[port]];
    }
    return return_val;
}

uint8_t write_into_last_segment(uint8_t message_index, int port, char ch){
    struct Message* new_message = messages[message_index];
    uint8_t return_val = 0;
    int r;
    int crc_val;
    int crc_rx;
    //if (ch == ':'){ // Call get_crc here
    if (colon_checker(message_index, port, ch)){
        new_message->CRC_buffer[new_message->CRC_index] = 0;
        colon_parser(message_index, port);
        crc_val = get_crc(new_message->CRC_buffer, new_message->CRC_index);
        crc_rx = ascii_to_int(new_message->Segments[sg_CRC_INDEX]);

        new_message->CRC_check = (crc_rx==crc_val);
        new_message->Sender     =  new_message->Segments[sg_SENDER_INDEX];
        new_message->Receiver   =  new_message->Segments[sg_RECEIVER_INDEX];
        new_message->RQ         =  new_message->Segments[sg_RQS_INDEX];
        new_message->Topic      =  new_message->Segments[sg_TOPIC_INDEX];
        new_message->Chapter    =  new_message->Segments[sg_CHAPTER_INDEX];
        int i;
        sprintf(log_buffer, "Message ID: %d received on port %d: crc checked: %d", new_message->ID, port, new_message->CRC_check); //%%TEST%%
        u2u_logger(log_buffer, 4); //%%TEST%%
        if (new_message->CRC_check==0){
            sprintf(log_buffer, "   CRC failure on ID: %d CRC_i: %d seg_CRC: '%s', \"%s\".", new_message->ID, new_message->CRC_index,  new_message->Segments[sg_CRC_INDEX], new_message->CRC_buffer); //%%TEST%%
            u2u_logger(log_buffer, 2);                                                              //%%TEST%%
            sprintf(log_buffer, "   CRC failure values calculated (val): %d, received (rx): %d.", crc_val,  crc_rx); //%%TEST%%
            u2u_logger(log_buffer, 2);                                                              //%%TEST%%
            sprintf(log_buffer, "   CRC segment contents: \"%s\".", new_message->Segments[sg_CRC_INDEX]); //%%TEST%%
            u2u_logger(log_buffer, 2);                                                              //%%TEST%%
            sprintf(log_buffer, "   CRC segment contents (ati): %d.", ascii_to_int(new_message->Segments[sg_CRC_INDEX])); //%%TEST%%
            u2u_logger(log_buffer, 2);                                                              //%%TEST%%
            sprintf(log_buffer, "   CRC get_crc : %d.", get_crc(new_message->CRC_buffer, new_message->CRC_index)); //%%TEST%%
            u2u_logger(log_buffer, 2);                                                              //%%TEST%%
        }else{
            r = in_queue(&Queue, message_index);
        }
        return_val = message_clear(message_index, port);
        if (new_message->CRC_check || RESPOND_ON_BAD_CRC){
            return_val = message_processor(message_index);
        }
    }else{
        new_message->Segments[segment_counter[port]][segment_length[port]] = ch;
        segment_length[port] = (segment_length[port] + 1) % SEGMENT_MAX_LENGTH[segment_counter[port]];
    }
    return return_val;
}

//
//RQS_r[] = "RS";
//RQS_q[] = "RQ";
//RQS_i[] = "RI";
//RQS_n[] = "NA";
//

uint8_t determine_addressee(uint8_t message_index, int port){ //0: Other, 1: Self, 2: General.
    struct Message* new_message = messages[message_index];
    uint8_t  router_value = 0;
    router_value = router_value + (1 * cmp(new_message->Segments[sg_RECEIVER_INDEX], self_name));
    router_value = router_value + (2 * cmp(new_message->Segments[sg_RECEIVER_INDEX], "GEN"));
    router_value = router_value + (4 * cmp(new_message->Segments[sg_RQS_INDEX], RQS_i));
    router_value = router_value + (4 * cmp(new_message->Segments[sg_RQS_INDEX], RQS_r));
    router_value = router_value + (4 * cmp(new_message->Segments[sg_RQS_INDEX], RQS_n));

    sprintf(log_buffer, "U2U: det add router value: %d.", router_value);
    u2u_logger(log_buffer, 4);
    return router_value;
}

int message_topic_checker(uint8_t message_index, int port, char* buffer){
    uint8_t  t, i;
    int topic_function_selector= -1;
    for (t=0; t<TOPIC_AMOUNT; t++){
        if (cmp(messages[message_index]->Segments[sg_TOPIC_INDEX], topic_list[t])){
            topic_function_selector = t;
            i = copy_str(buffer, msg_responses[t], 0);
            buffer[i] = 0;
            break;
        }
    }
    return  topic_function_selector;
}

uint8_t  append_segment(char* buffer, const char* str, int index){
    int new_index;
    new_index = copy_str(buffer, str, index);
    buffer[new_index] = ':';
    new_index++;
    return new_index;
}

uint8_t  add_crc(char* buffer, int index){
    int crc_val;
    char crc_str[3];
    buffer[index] = 0;
    crc_val = get_crc(buffer, index);
    int_to_ascii(crc_str, crc_val, 3);
    index = copy_str(buffer, crc_str, index);
    return index;
}

//const char self_name[] = "BUTTERLING";
//const char self_name_TEST[] = "SELFNAME";

uint8_t compose_transmit_message(struct Message* message_tx, char* buffer){
    uint8_t return_value = 0;
    int index = 0;
    int i = 0;
    char response_buffer[MAX_MESSAGE_SIZE];
    char len_buf[8];
    char chapter_buf[8];
    const char* nill = {"0"};
    message_tx->ID = tx_id;
    tx_id++;
    int_to_ascii(len_buf, len(message_tx->Payload), 3);
    int_to_ascii(chapter_buf, message_tx->intChapter, 3);
    i = copy_str(message_tx->Segments[sg_CHAPTER_INDEX], chapter_buf, 0);
    message_tx->Segments[sg_CHAPTER_INDEX][i] = 0;
    message_tx->Chapter = message_tx->Segments[sg_CHAPTER_INDEX];
    buffer[index] = ':'; //SENDER
    index++;
    buffer[index] = ':'; //SENDER
    index++;
    /* %%TEST%%
    index = append_segment(buffer, self_name, index);
     %%TEST%% */
    index = append_segment(buffer, self_name_TEST, index); //%%TEST%%
    index = append_segment(buffer, message_tx->Receiver, index);
    index = append_segment(buffer, message_tx->RQ, index);
    index = append_segment(buffer, message_tx->Topic, index);
    index = append_segment(buffer, message_tx->Chapter, index);
    index = append_segment(buffer, len_buf, index);
    index = append_segment(buffer, message_tx->Payload, index);
    index = append_segment(buffer, nill, index); //Hopcount is nill
    index = add_crc(buffer, index);
    buffer[index] = ':';
    index++;
    buffer[index] = '\0';
    sprintf(log_buffer, "U2U: ID: %d, ctm %s.", message_tx->ID, buffer);
    u2u_logger(log_buffer, 4);
    return return_value;
}

uint8_t u2u_send_message_uart0(struct Message* message_tx){
    uint8_t return_value = 0;
    int index = 0;
    char buffer[MAX_MESSAGE_SIZE];
    return_value = compose_transmit_message(message_tx, buffer);
    write_from_uart0(buffer); // Calling on u2u_HAL function.
    return return_value;
}

uint8_t u2u_send_message_uart1(struct Message* message_tx){
    uint8_t return_value = 0;
    int index = 0;
    char buffer[MAX_MESSAGE_SIZE];
    int i;
    sprintf(log_buffer, "send_message: SENDER: '%s'.", i, message_tx->Sender);
    u2u_logger(log_buffer, 4);
    sprintf(log_buffer, "send_message: RECEIVER: '%s'.", i, message_tx->Receiver);
    u2u_logger(log_buffer, 4);
    sprintf(log_buffer, "send_message: TOPIC: '%s'.", i, message_tx->Topic);
    u2u_logger(log_buffer, 4);
    sprintf(log_buffer, "send_message: intCHAPTER: %d.", i, message_tx->intChapter);
    u2u_logger(log_buffer, 4);

    return_value = compose_transmit_message(message_tx, buffer);

    sprintf(log_buffer, "send_message: SENDER: '%s'.", i, message_tx->Sender);
    u2u_logger(log_buffer, 4);
    sprintf(log_buffer, "send_message: RECEIVER: '%s'.", i, message_tx->Receiver);
    u2u_logger(log_buffer, 4);
    sprintf(log_buffer, "send_message: TOPIC: '%s'.", i, message_tx->Topic);
    u2u_logger(log_buffer, 4);
    sprintf(log_buffer, "send_message: intCHAPTER: %d.", i, message_tx->intChapter);
    u2u_logger(log_buffer, 4);
    sprintf(log_buffer, "send_message: CHAPTER: %d.", i, message_tx->Chapter);
    u2u_logger(log_buffer, 4);
    write_from_uart1(buffer); // Calling on u2u_HAL function.
    return return_value;
}

uint8_t u2u_send_message(struct Message* message_tx){
    uint8_t return_value = 0;
    int index = 0;
    char buffer[MAX_MESSAGE_SIZE];
    return_value = compose_transmit_message(message_tx, buffer);
    write_from_uart0(buffer); // Calling on u2u_HAL function.
    write_from_uart1(buffer); // Calling on u2u_HAL function.
    return return_value;
}

uint8_t compose_response_message(uint8_t message_index, int port, char* buffer){
    struct Message* new_message = messages[message_index];
    uint8_t return_value = 0;
    int index = 0;
    char response_buffer[MAX_MESSAGE_SIZE];
    int  topic_function_selector;
    char len_buf[4];
    char hops_buf[3];
    int hops;
    buffer[index] = ':'; //RESET
    index++;
    buffer[index] = ':'; //SENDER
    index++;
    index = append_segment(buffer, self_name, index);
    index = append_segment(buffer, new_message->Segments[sg_SENDER_INDEX], index);
    topic_function_selector = message_topic_checker(message_index, port, response_buffer);
    if (new_message->CRC_check==1){
        if (topic_function_selector == -1){
            index = append_segment(buffer, RQS_n, index);
        }else{
            index = append_segment(buffer, RQS_r, index);
        }
    }else{
        index = append_segment(buffer, RQS_n, index);
    }
    new_message->Topic_number = topic_function_selector;
    index = append_segment(buffer, new_message->Segments[sg_TOPIC_INDEX], index);
    index = append_segment(buffer, new_message->Segments[sg_CHAPTER_INDEX], index);
    int_to_ascii(len_buf, len(response_buffer), 3);
    index = append_segment(buffer, len_buf, index);
    index = append_segment(buffer, response_buffer, index);
    hops = ascii_to_int(new_message->Segments[sg_HOPCOUNT_INDEX]);
    hops++;
    int_to_ascii(hops_buf, hops, 3);
    index = append_segment(buffer, hops_buf, index);
    index = add_crc(buffer, index);
    sprintf(log_buffer, "U2U: crm crc for ID: %d: %s.", new_message->ID, buffer);
    u2u_logger(log_buffer, 4);
    buffer[index] = ':';
    index++;
    buffer[index] = '\0';
    sprintf(log_buffer, "U2U: crm: %s.", buffer);
    u2u_logger(log_buffer, 4);
    return return_value;
}

uint8_t compose_forward_message(uint8_t message_index, int port, char* buffer){
    struct Message* new_message = messages[message_index];
    uint8_t return_value = 0;
    int index = 0;
    char response_buffer[MAX_MESSAGE_SIZE];
    uint8_t  i = 0;
    char hops_buf[3];
    uint8_t  hops;
    buffer[index] = ':';
    index++;
    buffer[index] = ':';
    index++;
    for (i=sg_SENDER_INDEX; i<sg_PAYLOAD_INDEX; i++){
        index = append_segment(buffer, new_message->Segments[i], index);
        //sprintf(log_buffer, "U2U: cfm: %s, segs: %s.", buffer, new_message->Segments[i]);
        //u2u_logger(log_buffer, 4);
    }
    index = append_segment(buffer, new_message->Payload, index);
    hops_buf[3];
    hops = ascii_to_int(new_message->Segments[sg_HOPCOUNT_INDEX]);
    hops++;
    int_to_ascii(hops_buf, hops, 3);
    index = append_segment(buffer, hops_buf, index);
    index = add_crc(buffer, index);
    buffer[index] = ':';
    index++;
    buffer[index] = '\0';
    sprintf(log_buffer, "U2U: cfm: %s.", buffer);
    u2u_logger(log_buffer, 4);
    return return_value;
}

//uint8_t write_from_uart0(char* buffer){
//    uart_puts(uart0, buffer);
//    return 0;
//}
//
//uint8_t write_from_uart1(char* buffer){
//    uart_puts(uart1, buffer);
//    return 0;
//}

uint8_t self_call(uint8_t message_index, int port){
    uint8_t return_value = 0;
    char buffer[MAX_MESSAGE_SIZE];
    sprintf(log_buffer, "U2U: self_call  message_index: %d, port: %d.", message_index, port);
    u2u_logger(log_buffer, 4);
    return_value = compose_response_message(message_index, port, buffer);
    return_value = (*uart_write_functions[port])(buffer);
    messages[message_index]->For_self = 1;
    messages[message_index]->For_gen = 0;
    return return_value;
}

uint8_t other_call(uint8_t message_index, int port){
    uint8_t return_value = 0;
    char buffer[MAX_MESSAGE_SIZE];
    int other_port = 1 - port;
    sprintf(log_buffer, "U2U: other_call  message_index: %d, port: %d.", message_index, port);
    u2u_logger(log_buffer, 4);
    //port = 1 - port;
    return_value = compose_forward_message(message_index, other_port, buffer);
    return_value = (*uart_write_functions[other_port])(buffer);
    messages[message_index]->For_self = 0;
    messages[message_index]->For_gen = 0;
    return return_value;
}

uint8_t no_response_call(uint8_t message_index, int port){
    uint8_t return_value = 0;
    sprintf(log_buffer, "U2U: no_response_call  message_index: %d, port: %d.", message_index, port);
    u2u_logger(log_buffer, 4);
    messages[message_index]->For_self = 0;
    messages[message_index]->For_gen = 0;
    return return_value;
}

uint8_t general_call(uint8_t message_index, int port){
    uint8_t return_value = 0;
    char buffer[MAX_MESSAGE_SIZE];
    int other_port = 1 - port;
    sprintf(log_buffer, "U2U: general_call  message_index: %d, port: %d.", message_index, port);
    u2u_logger(log_buffer, 4);
    return_value = compose_response_message(message_index, port, buffer);
    return_value = (*uart_write_functions[port])(buffer);
    return_value = compose_forward_message(message_index, port, buffer);
    //port = 1 - port;
    return_value = (*uart_write_functions[other_port])(buffer);
    messages[message_index]->For_self = 1;
    messages[message_index]->For_gen = 1;
    return return_value;
}

uint8_t format_return_message(uint8_t message_index){
    uint8_t r = 0;
    uint8_t i = 0;
    //sprintf(log_buffer, "U2U: format_return_message START");
    //u2u_logger(log_buffer, 4);
/*
    i = copy_str(messages[message_index]->Sender, messages[message_index]->Segments[sg_SENDER_INDEX], 0);
    messages[message_index]->Sender[i] = 0;
    i = copy_str(messages[message_index]->Receiver, messages[message_index]->Segments[sg_RECEIVER_INDEX], 0);
    messages[message_index]->Receiver[i] = 0;
    i = copy_str(messages[message_index]->Topic, messages[message_index]->Segments[sg_TOPIC_INDEX], 0);
    messages[message_index]->Topic[i] = 0;
    i = copy_str(messages_out[message_index]->Chapter, messages[message_index]->Segments[sg_CHAPTER_INDEX], 0);
    messages_out[message_index]->Chapter[i] = 0;
    messages_out[message_index]->Payload = payloads_out[message_index];
    i = copy_str(messages_out[message_index]->Payload, messages[message_index]->Payload, 0);
    messages_out[message_index]->Payload[i] = 0;
    messages_out[message_index]->Topic_number = messages[message_index]->Topic_number;
    messages_out[message_index]->intLength = messages[message_index]->intLength;

    i = copy_str(messages[message_index]->Sender, messages[message_index]->Segments[sg_SENDER_INDEX], 0);
    messages[message_index]->Sender[i] = 0;
    i = copy_str(messages[message_index]->Receiver, messages[message_index]->Segments[sg_RECEIVER_INDEX], 0);
    messages[message_index]->Receiver[i] = 0;
    i = copy_str(messages[message_index]->Topic, messages[message_index]->Segments[sg_TOPIC_INDEX], 0);
    messages[message_index]->Topic[i] = 0;
    i = copy_str(messages[message_index]->Chapter, messages[message_index]->Segments[sg_CHAPTER_INDEX], 0);
    messages[message_index]->Chapter[i] = 0;
    messages[message_index]->Payload = payloads_out[message_index];
    i = copy_str(messages[message_index]->Payload, messages[message_index]->Payload, 0);
    messages[message_index]->Payload[i] = 0;
    messages[message_index]->Topic_number = messages[message_index]->Topic_number;
    messages[message_index]->intLength = messages[message_index]->intLength;
*/
    //sprintf(log_buffer, "U2U: format_return_message  message_index: %d.", message_index);
    //u2u_logger(log_buffer, 4);
    return r;
}

uint8_t message_processor(uint8_t message_index){
    //sprintf(log_buffer, "U2U: message_processor START");
    //u2u_logger(log_buffer, 4);
    uint8_t r = 0;
    int res = 0;
    int port = messages[message_index]->Port;
    int router_value;

    if (messages[message_index]->CRC_check==0){
        router_value = 1;
    }else{
        router_value = determine_addressee(message_index, port);
    }
    res = (*call_router_functions[router_value])(message_index, port);
    //r = format_return_message(message_index);
    //return_message = messages_out[message_index];
    return r;
}


//struct Message* message_processor(){
//    //printf("mp\n");
//    struct Message* return_message;
//    return_message = NULL;
//    if (i_message_ready){
//        i_message_ready = 0;
//        uint8_t m;
//        uint8_t r = 1;
//
//        for (m=0; m<message_counter_global; m++){
//
//            int res;
//            int port = messages[m]->Port;
//            int router_value = determine_addressee(m, port);
//            res = (*call_router_functions[router_value])(m, port);
//            r = format_return_message(m);
//            return_message = messages_out[m];
//        }
//        message_counter_global = 0;
//    }
//    return return_message;
//}

//struct Message* get_message(){
//    struct Message* return_message;
//    return_message = 0;
//    //static int first_index = 0;
//    uint8_t r = 1;
//    if (i_message_ready){
//        int res;
//        int port = messages[first_index]->Port;
//        int router_value = determine_addressee(first_index, port);
//        res = (*call_router_functions[router_value])(first_index, port);
//
//        r = format_return_message(router_value);
//        return_message = messages_out[router_value];
//        i_message_ready = 0;
//
//    }
//    return return_message;
//}

struct Message* get_message(){
    //struct Message* m;
    int i, r, m;
    r = out_queue(&Queue, &m);
    if (r==0){
        return messages[m];
    }else{
        return NULL;
    }
}

uint8_t u2u_topic_exchange(char* custom_payload, uint8_t topic_number){
    uint8_t return_val = 0;
    msg_responses[topic_number] = custom_payload;
    return return_val;
}

/*This is called by u2u_HAL on UART rx interupt.*/
uint8_t uart_character_processor(char ch){
    uint8_t res;
    res = uart0_character_processor(ch);
    return res;
}

/*This is called by u2u_HAL on UART rx interupt.*/
uint8_t uart0_character_processor(char ch){
    //sprintf(log_buffer, "U2U: uart0_character_processor START");
    //u2u_logger(log_buffer, 4);
    int port = 0; // Original Port definition.
    //printf("U0CP: %d, %c, %d, %d, %s\n", message_counter_global, ch, segment_counter[port], segment_length[port], message_segment_labels[segment_counter[port]]);
    uint8_t res;
    /* Before premessage_setup() message_counter_port's value will be ignored. Which is good because they are defaulted to null. */
    uint8_t message_index = message_counter_port[port];
    res = (*parse_function_array[segment_counter[port]])(message_index, port, ch);
    if (ch==0){
        //sprintf(log_buffer, "U2U: uart0_character_processor ch: (%d), message_index: %d, port: %d, segment_counter[port]: %d", ch, message_index, port, segment_counter[port]);
    }else{
        //sprintf(log_buffer, "U2U: uart0_character_processor ch: '%c', message_index: %d, port: %d, segment_counter[port]: %d", ch, message_index, port, segment_counter[port]);
    }
    //u2u_logger(log_buffer, 4);
    return 0;
}

/*This is called by u2u_HAL on UART rx interupt.*/
uint8_t uart1_character_processor(char ch){
    int port = 1; // Original Port definition.
    //printf("U1CP: %d, %c, %d, %d, %s\n", message_counter_global, ch, segment_counter[port], segment_length[port], message_segment_labels[segment_counter[port]]);
    uint8_t res;
    uint8_t  message_index = message_counter_port[port];
    res = (*parse_function_array[segment_counter[port]])(message_index, port, ch);
    if (ch==0){
        //sprintf(log_buffer, "U2U: uart1_character_processor ch: (%d), message_index: %d, port: %d, segment_counter[port]: %d", ch, message_index, port, segment_counter[port]);
    }else{
        //sprintf(log_buffer, "U2U: uart1_character_processor ch: '%c', message_index: %d, port: %d, segment_counter[port]: %d", ch, message_index, port, segment_counter[port]);
    }
    //u2u_logger(log_buffer, 4);
    return 0;
}

//
//void uart0_irq_routine(void){
//    int r;
//    //gpio_put(LED_2, 1);
//    while (uart_is_readable(uart0)){
//        uint8_t ch = uart_getc(uart0);
//        r = uart0_character_processor(ch);
//    }
//}
//
//void uart1_irq_routine(void){
//    int r;
//    //gpio_put(LED_3, 1);
//    while (uart_is_readable(uart1)){
//        uint8_t ch = uart_getc(uart1);
//        r = uart1_character_processor(ch);
//    }
//}
//
//

void u2u_message_setup(){
    int i, res;

    //struct Queue queue = { .front = 0, .rear = 0, .count = 0 };

    Queue.front = 0;
    Queue.rear = 0;
    Queue.count = 0;

    sprintf(log_buffer, "U2U: u2u_message_setup %d", MAX_MESSAGE_KEEP);
    u2u_logger(log_buffer, 4);

    //printf("Message stack being set up\n");
    messages[0] = &mi0;
    messages[1] = &mi1;
    messages[2] = &mi2;
    messages[3] = &mi3;
    messages[4] = &mi4;
    messages[5] = &mi5;
    messages[6] = &mi6;
    messages[7] = &mi7;
    segment_counter[0]  = 0;
    segment_length[0]   = 0;
    segment_counter[1]  = 0;
    segment_length[1]   = 0;
    //parse_function_array[sg_WAIT_INDEX]         = &wait_for_segment;
    parse_function_array[sg_PREMESSAGE_INDEX]   = &premessage_setup;
    parse_function_array[sg_SENDER_INDEX]       = &write_into_segment;        // Sender
    parse_function_array[sg_RECEIVER_INDEX]     = &write_into_segment;        // Receiver
    parse_function_array[sg_RQS_INDEX]          = &write_into_segment;        // RQS
    parse_function_array[sg_TOPIC_INDEX]        = &write_into_segment;        // Topic
    parse_function_array[sg_CHAPTER_INDEX]      = &write_into_segment;        // Chapter
    parse_function_array[sg_LENGTH_INDEX]       = &write_into_payload_length; // Payload Length
    parse_function_array[sg_PAYLOAD_INDEX]      = &write_into_payload_data;   // Payload Data
    parse_function_array[sg_HOPCOUNT_INDEX]     = &write_into_segment;        // Hopcount
    parse_function_array[sg_CRC_INDEX]          = &write_into_last_segment;   // CRC etc.

    call_router_functions[0] = &other_call;
    call_router_functions[1] = &self_call;
    call_router_functions[2] = &general_call;
    call_router_functions[3] = &general_call;
    call_router_functions[4] = &other_call;
    call_router_functions[5] = &no_response_call;
    call_router_functions[6] = &other_call;
    call_router_functions[7] = &other_call;
    call_router_functions[8] = &other_call;

    uart_write_functions[0] = &write_from_uart0;
    uart_write_functions[1] = &write_from_uart1;
    //sprintf(log_buffer, "U2U: u2u_message_setup");
    //u2u_logger(log_buffer, 4);

}
