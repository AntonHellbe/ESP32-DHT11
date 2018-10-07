#ifndef DH11_SENSOR_H
#define DH11_SENSOR_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef enum {
    DHT_SUCCESS,
    DHT_INIT_ERR,
    DHT_TIMEOUT_ERR,
    DHT_CHECKSUM_ERR,
    DHT_TOTAL_ERROR
} dht_ret_code_t;

typedef struct {
    uint8_t temp_farenheit;
    uint16_t temp_celsius;
    uint8_t humidity;
} dht_sensor_t;

dht_ret_code_t dht__get_sensor_values(dht_sensor_t * p_dht_sensor);
dht_ret_code_t dht__init(uint8_t pin);


#endif