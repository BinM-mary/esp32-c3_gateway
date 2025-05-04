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
#include "dns_server.h"
#include "cJSON.h"

#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs.h"

#define EXAMPLE_ESP_WIFI_SSID CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_MAX_STA_CONN CONFIG_ESP_MAX_STA_CONN
#define EXAMPLE_ESP_MAXIMUM_RETRY  5

#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_BOTH
#define EXAMPLE_H2E_IDENTIFIER ""
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK

extern const char root_start[] asm("_binary_root_html_start");
extern const char root_end[] asm("_binary_root_html_end");

extern void wifi_scan_task(void *pvParameters); // 声明WiFi扫描函数
extern char* get_wifi_list_json(); // 声明WiFi初始化函数
static void wifi_init_sta(void); 
static void wifi_init_softapsta(void);
static httpd_handle_t start_webserver(void);
static const char *TAG = "example";

typedef struct {
    char ssid[32];
    char password[64];
} wifi_credentials_t;

// 初始化 WiFi 凭证结构体
wifi_credentials_t wifi_creds = {
    .ssid = "wifissid",
    .password = "wifipassword"
};

size_t len_ssid = sizeof(wifi_creds.ssid);
size_t len_password = sizeof(wifi_creds.password);

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static int s_retry_num = 0;

void nvs_read(void)
{
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("wifi_config", NVS_READWRITE, &my_handle);
    if (err!= ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return;
    }
    err = nvs_get_str(my_handle, "wifi_ssid", wifi_creds.ssid, &len_ssid);
        switch (err) {
            case ESP_OK:
                // printf("Done\n");
                // printf("SSID = %s\n", wifi_creds.ssid);
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                // printf("The value is not initialized yet!\n");
                break;
            default :
                printf("Error (%s) reading!\n", esp_err_to_name(err));
        }
        err = nvs_get_str(my_handle, "wifi_password", wifi_creds.password, &len_password);
        switch (err) {
            case ESP_OK:
                // printf("Done\n");
                // printf("Password = %s\n", wifi_creds.password);
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                // printf("The value is not initialized yet!\n");
                break;
            default :
                // printf("Error (%s) reading!\n", esp_err_to_name(err));
        }
    nvs_close(my_handle);
}

void nvs_write(void)
{
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("wifi_config", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return;
    }
    err = nvs_set_str(my_handle, "wifi_ssid", wifi_creds.ssid);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) writing SSID!", esp_err_to_name(err));
        return;
    }
    err = nvs_set_str(my_handle, "wifi_password", wifi_creds.password);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) writing PASSWORD!", esp_err_to_name(err));
        return;
    }
    err = nvs_commit(my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) committing NVS changes!", esp_err_to_name(err));
        return;
    }
    nvs_close(my_handle);
}


static void AP_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}

static void STA_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
}

static void wifi_init_softapsta(void)
{
    // 创建默认的 WiFi 接入点网络接口
    esp_netif_create_default_wifi_ap();

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &AP_event_handler, NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        }
    };
    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_AP_DEF"), &ip_info);

    char ip_addr[16];
    inet_ntoa_r(ip_info.ip.addr, ip_addr, 16);
    ESP_LOGI(TAG, "Set up softAP with IP: %s", ip_addr);

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:'%s' password:'%s'",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();
    esp_netif_create_default_wifi_sta();

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &STA_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &STA_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            // 使用结构体中的 WiFi 名和密码
            .ssid = {wifi_creds.ssid[0], 0},
            .password = {wifi_creds.password[0], 0},
            .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
            .sae_pwe_h2e = ESP_WIFI_SAE_MODE,
            .sae_h2e_identifier = EXAMPLE_H2E_IDENTIFIER,
        },
    };
    // 复制 SSID 到配置结构体，显式类型转换
    strncpy((char *)wifi_config.sta.ssid, wifi_creds.ssid, sizeof(wifi_config.sta.ssid) - 1);
    wifi_config.sta.ssid[sizeof(wifi_config.sta.ssid) - 1] = '\0';
    // 复制密码到配置结构体，显式类型转换
    strncpy((char *)wifi_config.sta.password, wifi_creds.password, sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.password[sizeof(wifi_config.sta.password) - 1] = '\0';

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                wifi_creds.ssid, wifi_creds.password);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                wifi_creds.ssid, wifi_creds.password);
                wifi_init_softapsta(); // 初始化SoftAP和Station模式
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}

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