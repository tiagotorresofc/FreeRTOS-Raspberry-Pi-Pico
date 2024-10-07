#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "FreeRTOS.h"
#include "task.h"

#define LED1_PIN 5
#define LED2_PIN 6

int led1_count = 0;

void blink_led1_task(void *pvParameters) {
    const TickType_t delay = pdMS_TO_TICKS(250);

    for (;;) {
        if (led1_count < 3) {
            gpio_put(LED1_PIN, 1);
            vTaskDelay(delay); // meio segundo de atraso
            gpio_put(LED1_PIN, 0);
            vTaskDelay(delay); // meio segundo de atraso
            led1_count++;
        }else {
	
	    vTaskDelete(NULL);
	
	}
    }
}

void blink_led2_task(void *pvParameters) {
    const TickType_t delay = pdMS_TO_TICKS(250);

    for (;;) {
        gpio_put(LED2_PIN, 0);
        vTaskDelay(delay); // meio segundo de atraso
        gpio_put(LED2_PIN, 1);
        vTaskDelay(delay); // meio segundo de atraso
    }
}

int main() {
    stdio_init_all();

    gpio_init(LED1_PIN);
    gpio_set_dir(LED1_PIN, GPIO_OUT);

    gpio_init(LED2_PIN);
    gpio_set_dir(LED2_PIN, GPIO_OUT);

    xTaskCreate(blink_led1_task, "Blink LED1 Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(blink_led2_task, "Blink LED2 Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);

    vTaskStartScheduler();

    while(1);
    
    return 0;
}

