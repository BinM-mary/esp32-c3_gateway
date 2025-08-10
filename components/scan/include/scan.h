#ifndef SCAN_H
#define SCAN_H

#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C" {
#endif

// 函数声明
void wifi_scan_task(void *pvParameters);
char* get_wifi_list_json(void);

#ifdef __cplusplus
}
#endif

#endif // SCAN_H
