#ifndef UART_HANDLER_H
#define UART_HANDLER_H

#include "esp_err.h"

/**
 * @brief Callback function type to handle received and parsed UART data.
 *
 * @param data The null-terminated string containing the data to be handled.
 */
typedef void (*uart_data_callback_t)(const char *data);

/**
 * @brief Initializes the UART driver and starts the UART event task.
 *
 * @param callback The function to call when UART data is received and parsed.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t uart_handler_init(uart_data_callback_t callback);

#endif // UART_HANDLER_H
