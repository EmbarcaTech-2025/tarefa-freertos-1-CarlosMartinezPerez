#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include "pico/stdlib.h"

#include "hardware/i2c.h"
#include "oled/ssd1306.h"

// GPIOs
#define LED_RED_PIN     13
#define LED_GREEN_PIN   11
#define LED_BLUE_PIN    12
#define BUTTON_A_PIN    5
#define BUTTON_B_PIN    6
#define BUZZER1_PIN 10
#define BUZZER2_PIN 21

// Handles para suspender/retomar
TaskHandle_t ledTaskHandle = NULL;
TaskHandle_t buzzerTaskHandle = NULL;
TaskHandle_t oledTaskHandle = NULL;

// --- LED Task ---
void led_task(void *pvParameters) {
    const uint LED_PINS[] = {LED_RED_PIN, LED_GREEN_PIN, LED_BLUE_PIN};
    int color = 0;
    while (true) {
        // Apaga todos
        gpio_put(LED_RED_PIN, 0);
        gpio_put(LED_GREEN_PIN, 0);
        gpio_put(LED_BLUE_PIN, 0);
        // Acende cor atual
        gpio_put(LED_PINS[color], 1);
        color = (color + 1) % 3;
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// --- Buzzer Task ---
void buzzer_task(void *pvParameters) {
    while (true) {
        gpio_put(BUZZER1_PIN, 1);
        gpio_put(BUZZER2_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(50));
        gpio_put(BUZZER1_PIN, 0);
        gpio_put(BUZZER2_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(950));
    }
}

// --- Botão A (suspende/retoma LED) ---
void button_a_task(void *pvParameters) {
    bool led_suspended = false;
    while (true) {
        if (!gpio_get(BUTTON_A_PIN)) {  // Botão pressionado (ativo em 0)
            vTaskDelay(pdMS_TO_TICKS(100)); // debounce
            if (!gpio_get(BUTTON_A_PIN)) {
                if (led_suspended) {
                    vTaskResume(ledTaskHandle);
                } else {
                    vTaskSuspend(ledTaskHandle);
                }
                led_suspended = !led_suspended;
                while (!gpio_get(BUTTON_A_PIN)); // espera soltar
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// --- Botão B (suspende/retoma buzzer) ---
void button_b_task(void *pvParameters) {
    bool buzzer_suspended = false;
    while (true) {
        if (!gpio_get(BUTTON_B_PIN)) {
            vTaskDelay(pdMS_TO_TICKS(100));
            if (!gpio_get(BUTTON_B_PIN)) {
                if (buzzer_suspended) {
                    vTaskResume(buzzerTaskHandle);
                } else {
                    vTaskSuspend(buzzerTaskHandle);
                }
                buzzer_suspended = !buzzer_suspended;
                while (!gpio_get(BUTTON_B_PIN));
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// --- OLED Task ---
void oled_task(void *pvParameters) {
    char buffer[32];
    ssd1306_init();
    ssd1306_clear();
    ssd1306_show();

    while (true) {
        eTaskState led_state = eTaskGetState(ledTaskHandle);
        eTaskState buzzer_state = eTaskGetState(buzzerTaskHandle);

        // Limpa o display a cada atualização
        ssd1306_clear();

        // Converte o estado da tarefa LED para string e exibe
        switch (led_state) {
            case eRunning:      sprintf(buffer, "LED: RODANDO"); break;
            case eBlocked:      sprintf(buffer, "LED: RODANDO"); break;
            case eSuspended:    sprintf(buffer, "LED: SUSPENSO"); break;
            case eInvalid:
            default:            sprintf(buffer, "LED: ZEBRA"); break;
        }
        ssd1306_draw_string(0, 0, buffer);

        // Converte o estado da tarefa Buzzer para string e exibe
        switch (buzzer_state) {
            case eRunning:      sprintf(buffer, "BUZZER: RODANDO"); break;
            case eBlocked:      sprintf(buffer, "BUZZER: RODANDO"); break;
            case eSuspended:    sprintf(buffer, "BUZZER: SUSPENSO"); break;
            case eInvalid:
            default:            sprintf(buffer, "BUZZER: ZEBRA"); break;
        }
        ssd1306_draw_string(0, 32, buffer);

        ssd1306_show(); // Atualiza o display

        vTaskDelay(pdMS_TO_TICKS(200)); // Atualiza o display a cada 200ms
    }
}

// --- Setup de GPIOs ---
void gpio_init_all() {
    gpio_init(LED_RED_PIN);
    gpio_init(LED_GREEN_PIN);
    gpio_init(LED_BLUE_PIN);
    gpio_init(BUZZER1_PIN);
    gpio_init(BUZZER2_PIN);
    gpio_init(BUTTON_A_PIN);
    gpio_init(BUTTON_B_PIN);

    gpio_set_dir(LED_RED_PIN, GPIO_OUT);
    gpio_set_dir(LED_GREEN_PIN, GPIO_OUT);
    gpio_set_dir(LED_BLUE_PIN, GPIO_OUT);
    gpio_set_dir(BUZZER1_PIN, GPIO_OUT);
    gpio_set_dir(BUZZER2_PIN, GPIO_OUT);

    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_set_dir(BUTTON_B_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN);
    gpio_pull_up(BUTTON_B_PIN);
}

// --- Main ---
int main() {
    stdio_init_all();
    gpio_init_all();

    // Cria as tarefas
    xTaskCreate(led_task, "LED Task", 256, NULL, 1, &ledTaskHandle);
    xTaskCreate(buzzer_task, "Buzzer Task", 256, NULL, 1, &buzzerTaskHandle);
    xTaskCreate(button_a_task, "Button A Task", 256, NULL, 1, NULL);
    xTaskCreate(button_b_task, "Button B Task", 256, NULL, 1, NULL);
    xTaskCreate(oled_task, "OLED Task", 512, NULL, 0, &oledTaskHandle);

    // Inicia o escalonador
    vTaskStartScheduler();

    while (true) {
        // Nunca chega aqui
    }
}
