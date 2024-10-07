#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

// LED and button pins
#define LED1_PIN 15
#define BUTTON1_PIN 14
#define LED2_PIN 13
#define BUTTON2_PIN 12
#define LED3_PIN 11
#define BUTTON3_PIN 10
#define LED4_PIN 9
#define BUTTON4_PIN 8

// Task handles and semaphore
TaskHandle_t buttonTaskHandles[4];
TaskHandle_t ledTaskHandles[4];
SemaphoreHandle_t ledSemaphore;
QueueHandle_t ledQueue[4];

// Adjusted debounce delay (in ms)
#define DEBOUNCE_DELAY 200

// Function declarations
void button_isr(uint gpio, uint32_t events);
void button_task(void *params);
void led_task(void *params);

typedef struct {
    uint32_t ledPin;
    uint32_t buttonPin;
    uint32_t taskIndex;
} ButtonLedConfig;

// Button and LED configurations
ButtonLedConfig buttonLedConfigs[4] = {
    {LED1_PIN, BUTTON1_PIN, 0},
    {LED2_PIN, BUTTON2_PIN, 1},
    {LED3_PIN, BUTTON3_PIN, 2},
    {LED4_PIN, BUTTON4_PIN, 3}
};

// Button ISR
void button_isr(uint gpio, uint32_t events) {
    static uint32_t last_interrupt_time = 0;
    uint32_t interrupt_time = to_ms_since_boot(get_absolute_time());

    // Debounce: ignore interrupts within DEBOUNCE_DELAY ms
    if (interrupt_time - last_interrupt_time > DEBOUNCE_DELAY) {
        for (int i = 0; i < 4; i++) {
            if (gpio == buttonLedConfigs[i].buttonPin) {
                BaseType_t xHigherPriorityTaskWoken = pdFALSE;
                vTaskNotifyGiveFromISR(buttonTaskHandles[i], &xHigherPriorityTaskWoken);
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
                break;
            }
        }
    }
    last_interrupt_time = interrupt_time;
}

// Button task
void button_task(void *params) {
    ButtonLedConfig *config = (ButtonLedConfig *)params;
    while (1) {
        // Wait for notification from ISR
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // Send command to LED task to toggle LED
        uint32_t ledCommand = 1; // Command to toggle the LED
        if (xQueueSend(ledQueue[config->taskIndex], &ledCommand, portMAX_DELAY) == pdPASS) {

              printf("Command sent to LED task %d\n", config->taskIndex + 1);
        }
    }
}

// LED task
void led_task(void *params) {
    ButtonLedConfig *config = (ButtonLedConfig *)params;
    uint32_t ledState = 0;
    while (1) {
        uint32_t ledCommand;
        // Wait for command in the queue
        if (xQueueReceive(ledQueue[config->taskIndex], &ledCommand, portMAX_DELAY)) {
            if (ledState == 0) {
                // Try to take semaphore to turn on the LED
                if (xSemaphoreTake(ledSemaphore, 0) == pdTRUE) {
                    ledState = 1;
                    gpio_put(config->ledPin, ledState);
                    printf("LED %d ON. Available slots: %d\n", config->taskIndex + 1, uxSemaphoreGetCount(ledSemaphore));
                } else {
                    printf("Cannot turn on LED %d, semaphore unavailable. Available slots: %d\n", config->taskIndex + 1, uxSemaphoreGetCount(ledSemaphore));
                }
            } else {
                // Turn off the LED and release the semaphore
                ledState = 0;
                gpio_put(config->ledPin, ledState);
                xSemaphoreGive(ledSemaphore);
                printf("LED %d OFF. Available slots: %d\n", config->taskIndex + 1, uxSemaphoreGetCount(ledSemaphore));
            }
        }
    }
}

int main() {
    // Initialize GPIO
    stdio_init_all();
    for (int i = 0; i < 4; i++) {
        gpio_init(buttonLedConfigs[i].ledPin);
        gpio_set_dir(buttonLedConfigs[i].ledPin, GPIO_OUT);
        gpio_put(buttonLedConfigs[i].ledPin, 0); // Start with LED off
        gpio_init(buttonLedConfigs[i].buttonPin);
        gpio_set_dir(buttonLedConfigs[i].buttonPin, GPIO_IN);
        gpio_pull_up(buttonLedConfigs[i].buttonPin);
    }

    // Create counting semaphore with max count of 3 and initial count of 3
    ledSemaphore = xSemaphoreCreateCounting(3, 3);

    // Create queue
    for (int i = 0; i < 4; i++) {
        ledQueue[i] = xQueueCreate(1, sizeof(uint32_t));
    }

    // Check if semaphore and queues were created successfully
    if (ledSemaphore != NULL) {
        // Configure button interrupts
        for (int i = 0; i < 4; i++) {
            gpio_set_irq_enabled_with_callback(buttonLedConfigs[i].buttonPin, GPIO_IRQ_EDGE_FALL, true, &button_isr);

            // Create tasks with higher priority for faster response
            xTaskCreate(button_task, "Button Task", 256, &buttonLedConfigs[i], 2, &buttonTaskHandles[i]);
            xTaskCreate(led_task, "LED Task", 256, &buttonLedConfigs[i], 2, &ledTaskHandles[i]);
        }

        // Start FreeRTOS scheduler
        vTaskStartScheduler();
    } else {
        printf("Failed to create semaphore or queues\n");
    }

    // The program should never reach here
    while (1) {
    }
    return 0;
}

