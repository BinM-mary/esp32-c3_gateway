#ifndef MQTT_GATEWAY_H
#define MQTT_GATEWAY_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Starts the MQTT client and connects to the broker.
 */
void mqtt_app_start(void);

/**
 * @brief Publishes data to a predefined MQTT topic.
 *
 * @param data The null-terminated string to publish.
 */
void mqtt_publish_data(const char *data);

#ifdef __cplusplus
}
#endif

#endif // MQTT_GATEWAY_H
