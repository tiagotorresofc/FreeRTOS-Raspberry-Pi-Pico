#include "pico/stdlib.h"
#include "pico/time.h"
#include "FreeRTOS.h"
#include "task.h"

// Definições dos pinos dos LEDs
#define LED1_PIN 14
#define LED2_PIN 15

// Variáveis globais
volatile unsigned long ulIdleCycleCount = 0UL;
volatile unsigned long ulPreviousIdleCycleCount = 0UL;
volatile float cpu_usage = 0.0f; // Variável global para uso da CPU

// Função idle hook
void vApplicationIdleHook(void)
{
    ulIdleCycleCount++;
}

// Função para calcular e atualizar o uso da CPU
void update_cpu_usage(void)
{
    unsigned long ulCurrentIdleCycleCount = ulIdleCycleCount;
    cpu_usage = ((float)(ulCurrentIdleCycleCount - ulPreviousIdleCycleCount) / ulCurrentIdleCycleCount) * 100;
    ulPreviousIdleCycleCount = ulCurrentIdleCycleCount;
}

// Função para imprimir o nome da tarefa, o contador e o uso da CPU para LED1
void vPrintLED1Status(unsigned long ulNumber)
{
    update_cpu_usage();
    printf("LED1 Task is running. ulIdleCycleCount = %lu, CPU Usage: %.2f%%\n", ulNumber, cpu_usage);
}

// Função para imprimir o nome da tarefa, o contador e o uso da CPU para LED2
void vPrintLED2Status(unsigned long ulNumber)
{
    // O valor de cpu_usage já foi atualizado pela tarefa LED1
    printf("LED2 Task is running. ulIdleCycleCount = %lu, CPU Usage: %.2f%%\n", ulNumber, cpu_usage);
}

// Função da tarefa para piscar o LED1
void blink_led1_task(void *pvParameters) {
    const TickType_t delay = pdMS_TO_TICKS(500);

    // Configura o pino GPIO como saída
    gpio_init(LED1_PIN);
    gpio_set_dir(LED1_PIN, GPIO_OUT);

    for (;;) {
        gpio_put(LED1_PIN, 1);
        vTaskDelay(delay); // meio segundo de atraso
        gpio_put(LED1_PIN, 0);
        vTaskDelay(delay); // meio segundo de atraso
        vPrintLED1Status(ulIdleCycleCount);
    }
}

// Função da tarefa para piscar o LED2
void blink_led2_task(void *pvParameters) {
    const TickType_t delay = pdMS_TO_TICKS(500);

    // Configura o pino GPIO como saída
    gpio_init(LED2_PIN);
    gpio_set_dir(LED2_PIN, GPIO_OUT);

    for (;;) {
        gpio_put(LED2_PIN, 0);
        vTaskDelay(delay); // meio segundo de atraso
        gpio_put(LED2_PIN, 1);
        vTaskDelay(delay); // meio segundo de atraso
        vPrintLED2Status(ulIdleCycleCount);
    }
}

int main() {
    // Inicializa a UART
    stdio_init_all();

    // Cria as tarefas
    xTaskCreate(blink_led1_task, "Blink LED1 Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(blink_led2_task, "Blink LED2 Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);

    // Inicia o scheduler
    vTaskStartScheduler();

    // Nunca deve chegar aqui
    while (1);

    return 0;
}

