#include "u2u_HAL.h"

#define TXD1_PIN 21   // UART1 TX
#define RXD1_PIN 19   // UART1 RX
#define TXD2_PIN 17   // UART2 TX
#define RXD2_PIN 16   // UART2 RX

#define UART1_NUM UART_NUM_1
#define UART2_NUM UART_NUM_2

#define BUF_SIZE (1024)
#define TEST_STR "Sample string test.\n"

#define BLINK_GPIO 2

//#include "u2uclientdef.h"


#define BAUD_RATE 115200
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY    UART_PARITY_NONE

#define UART_IN UART_NUM_1
#define UART_OUT UART_NUM_2

#define TAG "u2u_hal"

///*From u2u.h*/
//uint8_t uart0_character_processor(char ch);
//uint8_t uart1_character_processor(char ch);
//
//void uart0_irq_routine(void);
//void uart1_irq_routine(void);
uint8_t write_from_uart(char* buffer, int len);
uint8_t write_from_uart0(char* buffer, int len);
uint8_t write_from_uart1(char* buffer, int len);
////void comm_logger(char* buffer, int len, int pdr);
////void inbound_message_logger(char* buffer, int len);
//int u2u_uart_setup();
//
////int message_ready;
//int u2u_uart_close(void);


static uint8_t rxbuf[BUF_SIZE];

static QueueHandle_t uart1_queue; // UART2 event queue
static QueueHandle_t uart2_queue; // UART2 event queue


int len(const char* string){
    const int MAX = 1000;
    int i = 0;
    while (string[i] != '\n'){
        i++;
        if (i>=MAX){
            i = 0;
            break;
        }
    }
    return i;
}

static void configure_led(void)
{
    ESP_LOGI(TAG, "Example configured to blink GPIO LED!");
    gpio_reset_pin(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
}

//sender uses UART1
//void uart_sender_task(void *arg)
//{
//    const char *test_str = TEST_STR;
//    int len_str = len(test_str);
//    while (1) {
//        uart_write_bytes(UART1_NUM, test_str, len_str);
//        //ESP_LOGI(TAG, "Sent: %s", test_str);
//        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay 1s
//    }
//}

uint8_t write_from_uart(char* buffer, int len){
    write_from_uart0(buffer, len);
    write_from_uart1(buffer, len);
    return 0;
}

uint8_t write_from_uart0(char* buffer, int len){
    uart_write_bytes(UART1_NUM, buffer, len);
    return 0;
}

uint8_t write_from_uart1(char* buffer, int len){
    uart_write_bytes(UART2_NUM, buffer, len);
    return 0;
}

void uart_event_task(void *pvParameters) {
    uart_event_t event;
    uint8_t* data = (uint8_t*) malloc(BUF_SIZE);

    for (;;) {
        // Wait for UART event from ISR
        gpio_set_level(BLINK_GPIO, 0);
        if (xQueueReceive(uart2_queue, (void *)&event, portMAX_DELAY)) {
            bzero(data, BUF_SIZE);

            switch (event.type) {
                case UART_DATA: {
                    int len = uart_read_bytes(UART2_NUM, data, event.size, portMAX_DELAY);
                    if (len > 0) {
                        gpio_set_level(BLINK_GPIO, 1);
                        data[len] = '\0';
                        ESP_LOGI(TAG, "EVENT: DATA, Received: %d, %s", len, data);
                    }
                    break;
                }
                case UART_BREAK: {
                    ESP_LOGW(TAG, "EVENT: BREAK");
                     break;
                }

                case UART_BUFFER_FULL: {
                    ESP_LOGW(TAG, "EVENT: BUFFER_FULL");
                     break;
                }
                case UART_FIFO_OVF: {
                    ESP_LOGW(TAG, "EVENT: FIFO_OVF");
                     break;
                }
                case UART_FRAME_ERR: {
                    ESP_LOGW(TAG, "EVENT: FRAME_ERR");
                     break;
                }
                case UART_PARITY_ERR: {
                    ESP_LOGW(TAG, "EVENT: PARITY_ERR");
                     break;
                }
                case UART_DATA_BREAK: {
                    ESP_LOGW(TAG, "EVENT: DATA_BREAK");
                     break;
                }
                case UART_PATTERN_DET: {
                    ESP_LOGW(TAG, "EVENT: PATTERN_DET");
                     break;
                }
                case UART_EVENT_MAX: {
                    ESP_LOGW(TAG, "EVENT: EVENT_MAX");
                     break;
                }
                                
                default:
                    ESP_LOGW(TAG, "UART event type: %d", event.type);
                    break;
            }
        }
    }
    free(data);
    vTaskDelete(NULL);
}

void comm_logger(char* buffer, int len, int pdr){
    return; 
}

int u2u_uart_setup(void){
    uart_config_t uart1_config = {
        .baud_rate = BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART1_NUM, &uart1_config);
    uart_set_pin(UART1_NUM, TXD1_PIN, RXD1_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART1_NUM, BUF_SIZE * 2, 0, 20, &uart2_queue, 0);

    // UART2 (receiver)
    uart_config_t uart2_config = {
        .baud_rate = BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_param_config(UART2_NUM, &uart2_config);
    uart_set_pin(UART2_NUM, TXD2_PIN, RXD2_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    // Install UART2 driver with RX buffer and event queue
    //uart2_queue = xQueueCreate(BUF_SIZE, sizeof(char));
    uart_driver_install(UART2_NUM, BUF_SIZE * 2, 0, 20, &uart2_queue, 0);
    //uart_driver_install(UART2_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);
	// enable RX interrupt
	ESP_ERROR_CHECK(uart_enable_rx_intr(UART2_NUM));
    return 0;
}


int u2u_uart_close(){
    uart_driver_delete(UART1_NUM);
    uart_driver_delete(UART2_NUM);
    return 0;
}



//void app_main(void)
//{
//    configure_led();
//    u2u_uart_setup();
//    // UART1 (sender)
//    // Start tasks
//    xTaskCreate(uart_sender_task, "uart_sender_task", 4096, NULL, 10, NULL);
//    xTaskCreate(uart_event_task, "uart_event_task", 4096, NULL, 10, NULL);
//}
