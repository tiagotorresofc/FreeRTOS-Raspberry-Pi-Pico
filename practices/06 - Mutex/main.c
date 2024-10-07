#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"

#define LED1_PIN 14
#define LED2_PIN 15
#define BUTTON1_PIN 17
#define BUTTON2_PIN 16
#define POTENTIOMETER_PIN 26
#define LED_TIMEOUT pdMS_TO_TICKS(5000) // 5 segundos

// Estrutura global para armazenar os dados do potenciômetro
typedef struct {
    uint16_t adc_value;   // Valor lido do potenciômetro
    TickType_t timestamp; // Timestamp da última leitura
} PotentiometerData_t;

// Declaração da variável global
PotentiometerData_t potentiometerData;
SemaphoreHandle_t xMutex; // Mutex para garantir exclusão mútua

void vLedTask(void *pvParameters) {
    uint32_t ledPin = (uint32_t) pvParameters;

    while (1) {
        // Tenta pegar o mutex para controlar o LED
        if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
            if (ledPin == LED2_PIN) {
                printf("LED %d ON - Mutex acquired\n", ledPin);
                gpio_put(ledPin, 1); // Liga o LED

                // Loop para ler o potenciômetro enquanto o LED2 estiver ligado
                for (TickType_t xStartTime = xTaskGetTickCount();
                     (xTaskGetTickCount() - xStartTime) < LED_TIMEOUT;
                     vTaskDelay(pdMS_TO_TICKS(100))) {

                    uint16_t adcValue = adc_read();

                    // Atualiza a estrutura global com o valor lido e o timestamp
                    potentiometerData.adc_value = adcValue;
                    potentiometerData.timestamp = xTaskGetTickCount();

                    printf("Potentiometer Value: %d at %lu ms\n", adcValue, potentiometerData.timestamp);
                }

                gpio_put(ledPin, 0); // Desliga o LED
                printf("LED %d OFF - Mutex released\n", ledPin);
            } else {
                gpio_put(ledPin, 1); // Liga o LED1
                vTaskDelay(LED_TIMEOUT); // Espera o timeout do LED
                gpio_put(ledPin, 0); // Desliga o LED1
            }

            xSemaphoreGive(xMutex); // Libera o mutex
        }

        vTaskSuspend(NULL); // Suspende a tarefa até que o botão a reative
    }
}

void vButtonTask(void *pvParameters) {
    uint32_t buttonPin = (uint32_t) pvParameters;
    uint32_t ledPin;

    if (buttonPin == BUTTON1_PIN) {
        ledPin = LED1_PIN;
    } else {
        ledPin = LED2_PIN;
    }

    while (1) {
        // Verifica se o botão foi pressionado
        if (gpio_get(buttonPin) == 0) {
            if (xSemaphoreTake(xMutex, 0) == pdFALSE) {
                printf("Attempt to turn on LED %d failed - Mutex is in use\n", ledPin);
            } else {
                xSemaphoreGive(xMutex); // Libera imediatamente porque apenas queremos verificar o status do mutex
                printf("Button %d pressed - Resuming LED %d task\n", buttonPin, ledPin);
                vTaskResume(xTaskGetHandle(ledPin == LED1_PIN ? "LedTask1" : "LedTask2"));
            }
            vTaskDelay(pdMS_TO_TICKS(500)); // Debounce
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

int main() {
    stdio_init_all();

    // Inicializa o ADC para o potenciômetro
    adc_init();
    adc_gpio_init(POTENTIOMETER_PIN);
    adc_select_input(0);

    // Configura os pinos dos LEDs e botões
    gpio_init(LED1_PIN);
    gpio_set_dir(LED1_PIN, GPIO_OUT);
    gpio_put(LED1_PIN, 0); 

    gpio_init(LED2_PIN);
    gpio_set_dir(LED2_PIN, GPIO_OUT);
    gpio_put(LED2_PIN, 0); 

    gpio_init(BUTTON1_PIN);
    gpio_set_dir(BUTTON1_PIN, GPIO_IN);
    gpio_pull_up(BUTTON1_PIN);

    gpio_init(BUTTON2_PIN);
    gpio_set_dir(BUTTON2_PIN, GPIO_IN);
    gpio_pull_up(BUTTON2_PIN);

    // Cria o mutex
    xMutex = xSemaphoreCreateMutex();

    if (xMutex != NULL) {
        // Cria as tarefas dos LEDs
        xTaskCreate(vLedTask, "LedTask1", 256, (void *) LED1_PIN, 2, NULL);
        xTaskCreate(vLedTask, "LedTask2", 256, (void *) LED2_PIN, 2, NULL);

        // Cria as tarefas dos botões
        xTaskCreate(vButtonTask, "ButtonTask1", 256, (void *) BUTTON1_PIN, 1, NULL);
        xTaskCreate(vButtonTask, "ButtonTask2", 256, (void *) BUTTON2_PIN, 1, NULL);

        // Inicia o agendador
        vTaskStartScheduler();
    }

    while (1);
}

