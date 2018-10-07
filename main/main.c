/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "DHT11_Sensor.h"


static QueueHandle_t xQueue1;

static void sender_task(void *pvParameters);
static void receiver_task(void *pvParameters);

void app_main()
{
    dht__init(GPIO_NUM_4);

    xQueue1 = xQueueCreate(10, sizeof(dht_sensor_t));
    if(xQueue1 != NULL) 
    {
        xTaskCreate(&sender_task, "sender_task", 2048, NULL, 6, NULL);
        xTaskCreate(&receiver_task, "receiver_task", 2048, NULL, 6, NULL);
    }
}


static void sender_task(void *pvParameters)
{
    dht_sensor_t dht_sensor;

    while(1) 
    {   
        int ret_val = dht__get_sensor_values(&dht_sensor);

        printf("Get sensor values returned status code: %d\r\n", ret_val);
        if(ret_val == DHT_SUCCESS) 
        {   
            printf("Sending value!!!\r\n");
            xQueueSend(xQueue1, &dht_sensor, (TickType_t) 0);
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}


static void receiver_task(void *pvParameters)
{
    dht_sensor_t recv_dht_sensor;
    while(1) 
    {
        if(xQueue1 != NULL) 
        {
            if (xQueueReceive(xQueue1, &(recv_dht_sensor), (TickType_t) 10)) 
            {
                printf("Receiver task, got value from sender task \r\n");
                printf("Humidity: %d\r\n", recv_dht_sensor.humidity);
                printf("Temp Celsius: %d\r\n", recv_dht_sensor.temp_celsius);
                printf("Temp Farenheit: %d\r\n", recv_dht_sensor.temp_farenheit);
            }
        }

        vTaskDelay(1050 / portTICK_PERIOD_MS);
    }
}
