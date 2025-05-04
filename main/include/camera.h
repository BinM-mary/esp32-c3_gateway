#ifndef CAMERA_H
#define CAMERA_H

#include "esp_http_server.h"

esp_err_t init_camera(void);
void register_camera_http_handlers(httpd_handle_t server);

#endif