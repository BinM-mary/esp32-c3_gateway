#ifndef DNS_SERVER_H
#define DNS_SERVER_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C" {
#endif

// 函数声明
void dns_server_task(void *pvParameters);
void start_dns_server(void);

#ifdef __cplusplus
}
#endif

#endif // DNS_SERVER_H
