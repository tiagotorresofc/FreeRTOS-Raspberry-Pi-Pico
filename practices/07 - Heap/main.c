#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include <string.h>

#define LED_PIN 15
#define HEAP_CONSUMPTION_SIZE 128  // Definindo o tamanho de cada consumo de heap em bytes

void vHeapMonitorTask(void *pvParameters) {
    const size_t totalHeapSize = configTOTAL_HEAP_SIZE;  // Tamanho total do heap configurado em FreeRTOSConfig.h
    size_t freeHeapSize;
    const size_t heapThreshold = totalHeapSize / 2;  // 50% do tamanho total do heap

    while (1) {
        freeHeapSize = xPortGetFreeHeapSize();  // Obtendo o tamanho livre do heap
        
        printf("Heap livre: %u bytes\n", freeHeapSize);  // Enviando tamanho livre do heap pela porta serial

        if (freeHeapSize < heapThreshold) {
            gpio_put(LED_PIN, 1);  // Acende o LED se o tamanho livre for menor que 50%
        } else {
            gpio_put(LED_PIN, 0);  // Apaga o LED caso contrário
        }

        vTaskDelay(pdMS_TO_TICKS(1000));  // Espera de 1 segundo
    }
}

void vHeapConsumptionTask(void *pvParameters) {
    while (1) {
        // Aloca memória dinamicamente para consumir o heap
        void *pMem = pvPortMalloc(HEAP_CONSUMPTION_SIZE);
        
        if (pMem != NULL) {
            // Simula alguma operação que utiliza a memória alocada
            memset(pMem, 0, HEAP_CONSUMPTION_SIZE);
        }

        vTaskDelay(pdMS_TO_TICKS(50));  // Espera de 0.5 segundo antes de consumir mais heap
    }
}

int main() {
    stdio_init_all();  // Inicializa a UART

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    xTaskCreate(vHeapMonitorTask, "Heap Monitor", 256, NULL, 1, NULL);  // Tarefa para monitorar o heap
    xTaskCreate(vHeapConsumptionTask, "Heap Consumer", 256, NULL, 1, NULL);  // Tarefa para consumir o heap

    vTaskStartScheduler();  // Inicia o agendador do FreeRTOS

    while (1) {
        // O código não deve chegar aqui, pois o agendador está rodando
    }

    return 0;
}

