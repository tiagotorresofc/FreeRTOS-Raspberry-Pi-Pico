#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"

// Definições de pinos
#define ADC_PIN 26
#define LED_PIN 15
#define BUZZER_PIN 14

// Definições para a Queue
QueueHandle_t adcQueue;

// Tarefa para ler o valor do ADC
void adc_read_task(void *params) {
    uint16_t adc_value;

    while (1) {
        // Ler o valor do ADC
        adc_select_input(0);
        adc_value = adc_read();

        // Enviar o valor do ADC para a Queue
        xQueueSend(adcQueue, &adc_value, portMAX_DELAY);

        // Imprimir o valor do ADC no terminal
        printf("ADC Value: %d\n", adc_value);

        // Aguardar 300ms
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}

// Tarefa para acionar o LED com base no valor do ADC
void led_control_task(void *params) {
    uint16_t adc_value;

    while (1) {
        // Receber o valor do ADC da Queue
        if (xQueueReceive(adcQueue, &adc_value, portMAX_DELAY)) {
            // Acender ou apagar o LED com base no valor do ADC
            if (adc_value > 2000) {
                gpio_put(LED_PIN, 1);
            } else {
                gpio_put(LED_PIN, 0);
            }

            // Imprimir o estado do LED no terminal
            printf("LED State: %s\n", (adc_value > 2000) ? "ON" : "OFF");
        }
    }
}

// Tarefa para acionar o buzzer com base no valor do ADC
void buzzer_control_task(void *params) {
    uint16_t adc_value;

    while (1) {
        // Receber o valor do ADC da Queue
        if (xQueueReceive(adcQueue, &adc_value, portMAX_DELAY)) {
            // Acionar ou desligar o buzzer com base no valor do ADC
            if (adc_value > 2000) {
                for (int i = 0; i < 100; i++) { // Frequência arbitrária
                    gpio_put(BUZZER_PIN, 1);
                    busy_wait_us_32(500); // 1 kHz (500us high + 500us low)
                    gpio_put(BUZZER_PIN, 0);
                    busy_wait_us_32(500);
                }
            } else {
                gpio_put(BUZZER_PIN, 0);
            }
        }
    }
}

int main() {
    // Inicializar stdio
    stdio_init_all();

    // Inicializar o ADC
    adc_init();
    adc_gpio_init(ADC_PIN);

    // Inicializar o GPIO para o LED e o Buzzer
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);

    // Criar a Queue para comunicação entre as tarefas
    adcQueue = xQueueCreate(10, sizeof(uint16_t));

    if (adcQueue == NULL) {
        printf("Failed to create queue.\n");
        while (1);
    }

    // Criar as tarefas
    xTaskCreate(adc_read_task, "ADC Read Task", 256, NULL, 1, NULL);
    xTaskCreate(led_control_task, "LED Control Task", 256, NULL, 1, NULL);
    xTaskCreate(buzzer_control_task, "Buzzer Control Task", 256, NULL, 1, NULL);

    // Iniciar o scheduler do FreeRTOS
    vTaskStartScheduler();

    while (1) {
        // Não deve chegar aqui
    }

    return 0;
}

