#include <sys/param.h>
#include <string.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"

#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "lwip/inet.h"

#include "esp_http_server.h"
#include "cJSON.h"

#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs.h"

// 包含组件头文件
#include "WiFi.h"
#include "scan.h"
#include "dns_server.h"

extern const char root_start[] asm("_binary_root_html_start");
extern const char root_end[] asm("_binary_root_html_end");

static httpd_handle_t start_webserver(void);
static const char *TAG = "example";

// HTTP GET Handler for root page
static esp_err_t root_get_handler(httpd_req_t *req)
{
    const uint32_t root_len = root_end - root_start;

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, root_start, root_len);

    return ESP_OK;
}

static esp_err_t reback_button_handler(httpd_req_t *req) {
    char json_data[50]; // 确保有足够的空间来存储JSON字符串
    snprintf(json_data, sizeof(json_data), "{\"show\": false}");

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_data, strlen(json_data));

    return ESP_OK;
}

// HTTP GET Handler for WiFi list
static esp_err_t wifi_list_get_handler(httpd_req_t *req) {
    char *json_str = get_wifi_list_json();
    if (json_str == NULL) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, strlen(json_str));

    free(json_str);
    return ESP_OK;
}

// HTTP POST Handler for WiFi configuration
static esp_err_t wifi_config_post_handler(httpd_req_t *req)
{
    char content[1024];
    httpd_req_recv(req, content, req->content_len);

    cJSON *root = cJSON_Parse(content);
    if (!root) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    cJSON *ssid_item = cJSON_GetObjectItem(root, "ssid");
    cJSON *password_item = cJSON_GetObjectItem(root, "password");

    if (ssid_item && password_item) {
        ESP_LOGI(TAG, "Received SSID: %s", ssid_item->valuestring);
        ESP_LOGI(TAG, "Received Password: %s", password_item->valuestring);

        // Use strncpy to copy the SSID
        strncpy(wifi_creds.ssid, ssid_item->valuestring, sizeof(wifi_creds.ssid) - 1);
        wifi_creds.ssid[sizeof(wifi_creds.ssid) - 1] = '\0';  // Ensure null-termination

        // Use strncpy to copy the password
        strncpy(wifi_creds.password, password_item->valuestring, sizeof(wifi_creds.password) - 1);
        wifi_creds.password[sizeof(wifi_creds.password) - 1] = '\0';  // Ensure null-termination
        nvs_write(); // 写入 NVS 存储的 Wi-Fi 信息

        cJSON *response = cJSON_CreateObject();
        cJSON_AddBoolToObject(response, "success", true);
        cJSON_AddStringToObject(response, "message", "Wi-Fi连接请求已接收");

        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, cJSON_Print(response), HTTPD_RESP_USE_STRLEN);

        cJSON_Delete(response);
        esp_restart(); // 重启系统
    } else {
        httpd_resp_send_err(req, 400, "Missing SSID or password");
    }

    cJSON_Delete(root);
    return ESP_OK;
}

// HTTP Error (404) Handler - Redirects all requests to the root page
esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    httpd_resp_set_status(req, "302 Temporary Redirect");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, "Redirect to the captive portal", HTTPD_RESP_USE_STRLEN);

    return ESP_OK;
}

static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_open_sockets = 13;
    config.lru_purge_enable = true;

    if (httpd_start(&server, &config) == ESP_OK) {
        wifi_mode_t wifi_mode;
        esp_wifi_get_mode(&wifi_mode);
        if (wifi_mode == WIFI_MODE_APSTA) {
            ESP_LOGE(TAG, "Starting captive portal in AP+STA mode");
            httpd_uri_t root_uri = {"/", HTTP_GET, root_get_handler};
            httpd_uri_t wifi_list_uri = {"/api/wifi_list", HTTP_GET, wifi_list_get_handler};
            httpd_uri_t wifi_config_uri = {"/api/wifi_config", HTTP_POST, wifi_config_post_handler};

            httpd_register_uri_handler(server, &root_uri);
            httpd_register_uri_handler(server, &wifi_list_uri);
            httpd_register_uri_handler(server, &wifi_config_uri);
            httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, http_404_error_handler);
        }else if (wifi_mode == WIFI_MODE_STA) {
            
        }
    }

    return server;
}

void app_main(void)
{
    wifi_init(); // 初始化WiFi
    nvs_read(); // 读取 NVS 存储的 Wi-Fi 信息
    wifi_init_sta(); // 初始化Station模式

    // 启动WiFi扫描任务
    xTaskCreate(wifi_scan_task, "wifi_scan_task", 4096, NULL, 5, NULL);

    start_webserver(); // 启动HTTP服务器

    start_dns_server(); // 启动DNS服务器
}