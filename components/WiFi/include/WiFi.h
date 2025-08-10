#ifndef WIFI_H
#define WIFI_H

#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "nvs.h"

#ifdef __cplusplus
extern "C" {
#endif

// WiFi凭证结构体
typedef struct {
    char ssid[32];
    char password[64];
} wifi_credentials_t;

// 事件组位定义
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

// 函数声明
void wifi_init(void);
void wifi_init_sta(void);
void wifi_init_softapsta(void);
void nvs_read(void);
void nvs_write(void);
void AP_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void STA_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

// 外部变量声明
extern wifi_credentials_t wifi_creds;
extern EventGroupHandle_t s_wifi_event_group;
extern int s_retry_num;

#ifdef __cplusplus
}
#endif

#endif // WIFI_H
