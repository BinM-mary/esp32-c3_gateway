#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "cJSON.h"

#include "scan.h"

#define DEFAULT_SCAN_LIST_SIZE CONFIG_EXAMPLE_SCAN_LIST_SIZE
#define MAX_WIFI_LIST_SIZE 32 // 定义最大WiFi列表大小

static const char *TAG = "scan";
wifi_ap_record_t g_ap_info[MAX_WIFI_LIST_SIZE]; // 全局变量存储扫描到的AP信息
uint16_t g_ap_count = 0; // 全局变量存储AP数量

static void print_auth_mode(int authmode)
{
    switch (authmode) {
    case WIFI_AUTH_OPEN:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_OPEN");
        break;
    case WIFI_AUTH_OWE:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_OWE");
        break;
    case WIFI_AUTH_WEP:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WEP");
        break;
    case WIFI_AUTH_WPA_PSK:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA_PSK");
        break;
    case WIFI_AUTH_WPA2_PSK:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA2_PSK");
        break;
    case WIFI_AUTH_WPA_WPA2_PSK:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA_WPA2_PSK");
        break;
    case WIFI_AUTH_ENTERPRISE:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_ENTERPRISE");
        break;
    case WIFI_AUTH_WPA3_PSK:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA3_PSK");
        break;
    case WIFI_AUTH_WPA2_WPA3_PSK:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA2_WPA3_PSK");
        break;
    case WIFI_AUTH_WPA3_ENT_192:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA3_ENT_192");
        break;
    default:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_UNKNOWN");
        break;
    }
}

static void print_cipher_type(int pairwise_cipher, int group_cipher)
{
    switch (pairwise_cipher) {
    case WIFI_CIPHER_TYPE_NONE:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_NONE");
        break;
    case WIFI_CIPHER_TYPE_WEP40:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_WEP40");
        break;
    case WIFI_CIPHER_TYPE_WEP104:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_WEP104");
        break;
    case WIFI_CIPHER_TYPE_TKIP:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_TKIP");
        break;
    case WIFI_CIPHER_TYPE_CCMP:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_CCMP");
        break;
    case WIFI_CIPHER_TYPE_TKIP_CCMP:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_TKIP_CCMP");
        break;
    case WIFI_CIPHER_TYPE_AES_CMAC128:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_AES_CMAC128");
        break;
    case WIFI_CIPHER_TYPE_SMS4:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_SMS4");
        break;
    case WIFI_CIPHER_TYPE_GCMP:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_GCMP");
        break;
    case WIFI_CIPHER_TYPE_GCMP256:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_GCMP256");
        break;
    default:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_UNKNOWN");
        break;
    }

    switch (group_cipher) {
    case WIFI_CIPHER_TYPE_NONE:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_NONE");
        break;
    case WIFI_CIPHER_TYPE_WEP40:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_WEP40");
        break;
    case WIFI_CIPHER_TYPE_WEP104:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_WEP104");
        break;
    case WIFI_CIPHER_TYPE_TKIP:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_TKIP");
        break;
    case WIFI_CIPHER_TYPE_CCMP:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_CCMP");
        break;
    case WIFI_CIPHER_TYPE_TKIP_CCMP:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_TKIP_CCMP");
        break;
    case WIFI_CIPHER_TYPE_SMS4:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_SMS4");
        break;
    case WIFI_CIPHER_TYPE_GCMP:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_GCMP");
        break;
    case WIFI_CIPHER_TYPE_GCMP256:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_GCMP256");
        break;
    default:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_UNKNOWN");
        break;
    }
}

void wifi_scan_task(void *pvParameters) {
    while (1) {
        uint16_t number = MAX_WIFI_LIST_SIZE;
        memset(g_ap_info, 0, sizeof(g_ap_info));
        g_ap_count = 0;

        esp_wifi_scan_start(NULL, true);
        // ESP_LOGI(TAG, "Max AP number can hold = %u", number);
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&g_ap_count));
        if (g_ap_count > MAX_WIFI_LIST_SIZE) {
            g_ap_count = MAX_WIFI_LIST_SIZE;
        }
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&g_ap_count, g_ap_info));
        // ESP_LOGI(TAG, "Total APs scanned = %u, actual AP number stored = %u", g_ap_count, number);
        // for (int i = 0; i < g_ap_count; i++) {
        //     ESP_LOGI(TAG, "SSID \t\t%s", g_ap_info[i].ssid);
        //     ESP_LOGI(TAG, "RSSI \t\t%d", g_ap_info[i].rssi);
        //     print_auth_mode(g_ap_info[i].authmode);
        //     if (g_ap_info[i].authmode != WIFI_AUTH_WEP) {
        //         print_cipher_type(g_ap_info[i].pairwise_cipher, g_ap_info[i].group_cipher);
        //     }
        //     ESP_LOGI(TAG, "Channel \t\t%d\n", g_ap_info[i].primary);
        // }

        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}

char* get_wifi_list_json() {
    cJSON *root = cJSON_CreateObject();
    cJSON *wifi_list = cJSON_CreateArray();

    for (int i = 0; i < g_ap_count; i++) {
        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "ssid", (char *)g_ap_info[i].ssid);
        cJSON_AddNumberToObject(item, "rssi", g_ap_info[i].rssi);
        cJSON_AddNumberToObject(item, "channel", g_ap_info[i].primary);

        char auth_mode_str[20];
        switch (g_ap_info[i].authmode) {
            case WIFI_AUTH_OPEN: strcpy(auth_mode_str, "开放"); break;
            case WIFI_AUTH_WEP: strcpy(auth_mode_str, "WEP"); break;
            case WIFI_AUTH_WPA_PSK: strcpy(auth_mode_str, "WPA-PSK"); break;
            case WIFI_AUTH_WPA2_PSK: strcpy(auth_mode_str, "WPA2-PSK"); break;
            case WIFI_AUTH_WPA_WPA2_PSK: strcpy(auth_mode_str, "WPA/WPA2-PSK"); break;
            default: strcpy(auth_mode_str, "未知"); break;
        }
        cJSON_AddStringToObject(item, "auth_mode", auth_mode_str);

        cJSON_AddItemToArray(wifi_list, item);
    }

    cJSON_AddItemToObject(root, "wifi_list", wifi_list);
    char *json_str = cJSON_Print(root);
    cJSON_Delete(root);

    return json_str;
}
