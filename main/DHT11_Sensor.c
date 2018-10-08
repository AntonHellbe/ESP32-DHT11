
#include "DHT11_Sensor.h"
#include "driver/gpio.h"
#include "esp_system.h"

#define VALID_PIN           20
#define DHT_DATA_LENGTH     40
#define DHT_DATA_PIN_HIGH   1
#define DHT_DATA_PIN_LOW    0
#define SEND_START_DELAY_1  22000
#define SEND_START_DELAY_2  43
#define DHT_TIMEOUT_LIMIT   200
#define IS_BIT_HIGH(X)      (X > 40 ? (true) : (false))


static void dht__check_length(uint8_t * p_bit_count, uint8_t * p_byte_count);
static dht_ret_code_t dht__data_transmission_seq(dht_sensor_t * p_dht_sensor);
static dht_ret_code_t dht__expect_pulse(uint8_t pin_level, uint8_t * p_delay_counter);
static uint8_t dht__data_level(void);
static void dht__send_start(void);

static uint8_t dht_data_pin;

dht_ret_code_t dht__init(uint8_t pin)
{
    if(GPIO_IS_VALID_GPIO(pin) == false) 
    {
        return DHT_INIT_ERR;
    }

    dht_data_pin = pin;
    return DHT_SUCCESS;
}

static void dht__send_start(void)
{
    gpio_set_direction(dht_data_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(dht_data_pin, DHT_DATA_PIN_LOW);
    ets_delay_us(SEND_START_DELAY_1);
    gpio_set_level(dht_data_pin, DHT_DATA_PIN_HIGH);
    ets_delay_us(SEND_START_DELAY_2);
    gpio_set_direction(dht_data_pin, GPIO_MODE_INPUT);
}

dht_ret_code_t dht__get_sensor_values(dht_sensor_t * p_dht_sensor)
{
    dht__send_start();
    uint8_t delay_counter = 0;

    //Wait for a response from the DHT11 Sensor, waiting time is usually between 20-40 us
    if(dht__expect_pulse(DHT_DATA_PIN_HIGH, &delay_counter) == DHT_TIMEOUT_ERR) 
    {
        printf("DHT: Timeout error, No response from DHT11 \r\n");
        return DHT_TIMEOUT_ERR;
    } 

    delay_counter = 0;
    // DHT has pulled data line low, it will be kept low for 80us and then high for another 80us
    // Check so DHT keeps the line low
    if(dht__expect_pulse(DHT_DATA_PIN_LOW, &delay_counter) == DHT_TIMEOUT_ERR) 
    {
        printf("DHT: Timeout error, DHT11 has kept line low for more than 80us \r\n");
        return DHT_TIMEOUT_ERR;
    } 

    delay_counter = 0;
    // Check that DHT11 keeps the line high
    if(dht__expect_pulse(DHT_DATA_PIN_HIGH, &delay_counter) == DHT_TIMEOUT_ERR) 
    {
        printf("DHT: Timeout error, DHT11 has kept line high for more than 80us \r\n");
        return DHT_TIMEOUT_ERR;
    }

    dht__data_transmission_seq(p_dht_sensor);

    return DHT_SUCCESS;

}

static dht_ret_code_t dht__data_transmission_seq(dht_sensor_t * p_dht_sensor)
{
    uint8_t received_bytes[5] = {0 ,0 ,0 ,0, 0};
    uint8_t byte_count = 0;
    uint8_t bit_count = 7;
    uint8_t delay_counter = 0;
    uint8_t i = 0;


    // Start of reading the actual sensor data
    // Data transmission size is 40 bits, i.e 5 bytes
    for(i = 0; i < DHT_DATA_LENGTH; i++)
    {
        // Data transmission starts with 50us low signal
        if(dht__expect_pulse(DHT_DATA_PIN_LOW, &delay_counter) == DHT_TIMEOUT_ERR) 
        {
            printf("DHT: Timeout error, data transmission start signal more than 50us \r\n");
        }

        delay_counter = 0;

        // Time to check if data is a 0 or 1
        if(dht__expect_pulse(DHT_DATA_PIN_HIGH, &delay_counter) == DHT_TIMEOUT_ERR)
        {
            printf("DHT: Timeout error, could not read bit properly from DHT11 \r\n");
            return DHT_TIMEOUT_ERR;
        }

        // Look if bit is 1 or 0, when count is greater than 40 it is a 1
        if(IS_BIT_HIGH(delay_counter))
        {
            // Shift the bit up to its position
            received_bytes[byte_count] |= (1 << bit_count);
        }

        // Check the length of bit count (Start new byte or decrement the bit count)
        dht__check_length(&bit_count, &byte_count);
        delay_counter = 0;
    }

    p_dht_sensor->humidity = received_bytes[0];
    p_dht_sensor->temp_celsius = received_bytes[2];
    p_dht_sensor->temp_farenheit = p_dht_sensor->temp_celsius * 1.8 + 32;
    
    uint8_t sum = (received_bytes[0] + received_bytes[1] + 
        received_bytes[2] + received_bytes[3]) & 0xFF;

    if(received_bytes[4] != sum)
    {
        printf("DHT: Checksum Error \r\n");
        return DHT_CHECKSUM_ERR;
    }


    return DHT_SUCCESS;

}

static dht_ret_code_t dht__expect_pulse(uint8_t pin_level, uint8_t * p_delay_counter)
{
    while(dht__data_level() == pin_level)
    {
        if(*p_delay_counter > DHT_TIMEOUT_LIMIT)
        {
            printf("DHT: Timeout error \r\n");
            return DHT_TIMEOUT_ERR;
        }

        *p_delay_counter = *p_delay_counter + 1;
        ets_delay_us(1);
    }

    return DHT_SUCCESS;

}

static void dht__check_length(uint8_t * p_bit_count, uint8_t * p_byte_count)
{
    if(*p_bit_count == 0)
    {
        *p_bit_count = 7;
        *p_byte_count = *p_byte_count + 1;
    }
    else 
    {
        *p_bit_count = *p_bit_count - 1;
    }
}

static uint8_t dht__data_level(void)
{
    return gpio_get_level(dht_data_pin);
}

