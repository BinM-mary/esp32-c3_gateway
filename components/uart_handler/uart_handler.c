#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "cJSON.h"
#include "uart_handler.h"


static const char *TAG = "uart_handler";

// --- ��1��: ѡ��Ӳ������ UART1 ��Ϊ����䰲ȫ��Ĭ������ ---
#define DATA_UART_NUM       UART_NUM_1      // ѡ��Ӳ������1 (����־��UART0�ֿ�)
#define DATA_UART_TXD_PIN   (GPIO_NUM_7)   // Ϊ UART1 ָ����ȫ�� TX ����
#define DATA_UART_RXD_PIN   (GPIO_NUM_6)    // Ϊ UART1 ָ����ȫ�� RX ����

#define BUF_SIZE            (2048)
#define RD_BUF_SIZE         (BUF_SIZE)

static QueueHandle_t data_uart_queue;
static uart_data_callback_t data_callback = NULL;

// JSON�������� (�����޸�)
static char* parse_data_to_json(const uint8_t* data, int len) {
    char buffer[len + 1];
    memcpy(buffer, data, len);
    buffer[len] = '\0';

    char *saveptr;
    char *key = strtok_r(buffer, ":", &saveptr);
    char *value_str = strtok_r(NULL, ":", &saveptr);

    if (key == NULL || value_str == NULL) {
        ESP_LOGE(TAG, "Failed to parse key:value format from UART data: %s", buffer);
        return NULL;
    }
    
    int value = atoi(value_str);

    cJSON *root = cJSON_CreateObject();
    if (root == NULL) return NULL;

    cJSON_AddStringToObject(root, "id", "123");
    cJSON_AddStringToObject(root, "version", "1.0");

    cJSON *params = cJSON_CreateObject();
    if (params == NULL) {
        cJSON_Delete(root);
        return NULL;
    }
    cJSON_AddItemToObject(root, "params", params);

    cJSON *property_obj = cJSON_CreateObject();
    if (property_obj == NULL) {
        cJSON_Delete(root);
        return NULL;
    }
    cJSON_AddItemToObject(params, key, property_obj);

    cJSON_AddNumberToObject(property_obj, "value", value);

    char *json_string = cJSON_Print(root);
    cJSON_Delete(root);

    ESP_LOGI(TAG, "Generated JSON: %s", json_string);

    return json_string;
}

// UART�¼���������
static void uart_event_task(void *pvParameters)
{
    uart_event_t event;
    uint8_t* dtmp = (uint8_t*) malloc(RD_BUF_SIZE);
    assert(dtmp);
    for (;;) {
        if (xQueueReceive(data_uart_queue, (void *)&event, (TickType_t)portMAX_DELAY)) {
            bzero(dtmp, RD_BUF_SIZE);
            switch (event.type) {
            case UART_DATA:
                uart_read_bytes(DATA_UART_NUM, dtmp, event.size, portMAX_DELAY);
                // ����־����ȷָ���� UART1
                ESP_LOGI(TAG, "UART [1] received data (size: %d):", event.size);
                ESP_LOG_BUFFER_CHAR(TAG, dtmp, event.size);

                char *json_payload = parse_data_to_json(dtmp, event.size);
                if (json_payload && data_callback) {
                    data_callback(json_payload);
                    free(json_payload);
                } else if (json_payload) {
                    free(json_payload);
                }
                break;
            case UART_FIFO_OVF:
                ESP_LOGI(TAG, "hw fifo overflow on UART [1]");
                uart_flush_input(DATA_UART_NUM);
                xQueueReset(data_uart_queue);
                break;
            case UART_BUFFER_FULL:
                ESP_LOGI(TAG, "ring buffer full on UART [1]");
                uart_flush_input(DATA_UART_NUM);
                xQueueReset(data_uart_queue);
                break;
            default:
                ESP_LOGI(TAG, "uart event type on UART [1]: %d", event.type);
                break;
            }
        }
    }
    free(dtmp);
    dtmp = NULL;
    vTaskDelete(NULL);
}

// ��ʼ������
esp_err_t uart_handler_init(uart_data_callback_t callback)
{
    data_callback = callback;

    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // --- ��2��: ������Ӧ�õ�ѡ����Ӳ���������� ---
    // ��װ������ UART_NUM_1
    ESP_ERROR_CHECK(uart_driver_install(DATA_UART_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 20, &data_uart_queue, 0));
    // ���� UART_NUM_1 �Ĳ���
    ESP_ERROR_CHECK(uart_param_config(DATA_UART_NUM, &uart_config));
    // �� GPIO 10 (TX) �� GPIO 9 (RX) ����� UART_NUM_1
    ESP_ERROR_CHECK(uart_set_pin(DATA_UART_NUM, DATA_UART_TXD_PIN, DATA_UART_RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    xTaskCreate(uart_event_task, "uart_event_task", 3072, NULL, 12, NULL);

    return ESP_OK;
}
