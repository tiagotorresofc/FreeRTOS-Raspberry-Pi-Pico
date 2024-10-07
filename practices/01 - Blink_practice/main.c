#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "FreeRTOS.h"
#include "task.h"

#define LED1_PIN 2
#define LED2_PIN 3
#define LED3_PIN 4

void led_task(void *pvParameters) {
    const TickType_t xDelay = pdMS_TO_TICKS(250); // Intervalo de 500ms
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1) {
        gpio_put(LED1_PIN, 1); // Liga o LED1
        vTaskDelayUntil(&xLastWakeTime, xDelay);

        gpio_put(LED1_PIN, 0); // Desliga o LED1
        gpio_put(LED2_PIN, 1); // Liga o LED2
        vTaskDelayUntil(&xLastWakeTime, xDelay);

        gpio_put(LED2_PIN, 0); // Desliga o LED2
        gpio_put(LED3_PIN, 1); // Liga o LED3
        vTaskDelayUntil(&xLastWakeTime, xDelay);

        gpio_put(LED3_PIN, 0); // Desliga o LED3
    }
}

int main() {
    stdio_init_all();

    gpio_init(LED1_PIN);
    gpio_set_dir(LED1_PIN, GPIO_OUT);
    gpio_init(LED2_PIN);
    gpio_set_dir(LED2_PIN, GPIO_OUT);
    gpio_init(LED3_PIN);
    gpio_set_dir(LED3_PIN, GPIO_OUT);

    xTaskCreate(led_task, "LED Task", 1024, NULL, configMAX_PRIORITIES, NULL);

    vTaskStartScheduler();

    return 0;
}

