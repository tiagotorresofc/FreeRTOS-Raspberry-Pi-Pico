#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

// LED and button pins
#define LED_PIN 15
#define BUTTON_PIN 14

// Task handles and semaphore
TaskHandle_t buttonTaskHandle = NULL;
TaskHandle_t ledTaskHandle = NULL;
SemaphoreHandle_t buttonSemaphore;
QueueHandle_t ledQueue;

// Debounce delay (in ms)
#define DEBOUNCE_DELAY 200

// Button ISR
void button_isr(uint gpio, uint32_t events) {
    static uint32_t last_interrupt_time = 0;
    uint32_t interrupt_time = to_ms_since_boot(get_absolute_time());

    // Debounce: ignore interrupts within DEBOUNCE_DELAY ms
    if (interrupt_time - last_interrupt_time > DEBOUNCE_DELAY) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(buttonSemaphore, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
    last_interrupt_time = interrupt_time;
}

// Button task
void button_task(void *params) {
    while (1) {
        // Wait for semaphore notification
        if (xSemaphoreTake(buttonSemaphore, portMAX_DELAY)) {
            printf("Button pressed\n");

            // Send command to LED task
            uint32_t ledCommand = 1; // Command to toggle the LED
            if (xQueueSend(ledQueue, &ledCommand, portMAX_DELAY) == pdPASS) {
                printf("Command sent to LED task\n");
            } else {
                printf("Failed to send command to LED task\n");
            }
        }
    }
}

// LED task
void led_task(void *params) {
    uint32_t ledState = 0;
    while (1) {
        uint32_t ledCommand;
        // Wait for command in the queue
        if (xQueueReceive(ledQueue, &ledCommand, portMAX_DELAY)) {
            ledState = !ledState; // Toggle LED state
            gpio_put(LED_PIN, ledState);
            printf("LED %s\n", ledState ? "on" : "off");
        }
    }
}

int main() {
    // Initialize GPIO
    stdio_init_all();
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    // Create binary semaphore and queue
    buttonSemaphore = xSemaphoreCreateBinary();
    ledQueue = xQueueCreate(10, sizeof(uint32_t));

    // Check if semaphore and queue were created successfully
    if (buttonSemaphore != NULL && ledQueue != NULL) {
        // Configure button interrupt
        gpio_set_irq_enabled_with_callback(BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true, &button_isr);

        // Create tasks
        xTaskCreate(button_task, "Button Task", 256, NULL, 1, &buttonTaskHandle);
        xTaskCreate(led_task, "LED Task", 256, NULL, 1, &ledTaskHandle);

        // Start FreeRTOS scheduler
        vTaskStartScheduler();
    } else {
        printf("Failed to create semaphore or queue\n");
    }

    // The program should never reach here
    while (1) {
    }
    return 0;
}

