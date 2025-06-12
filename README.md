
# Tarefa: Roteiro de FreeRTOS #1 - EmbarcaTech 2025

Autor: **Carlos Martinez Perez**

Curso: Residência Tecnológica em Sistemas Embarcados

Instituição: EmbarcaTech - HBr

Campinas, junho de 2025

---
# RTOS - Tarefa 1 - Atividade Roteirizada com FreeRTOS na BitDogLab

1. Problema a ser resolvido
Desenvolver um sistema multitarefa embarcado usando a BitDogLab, programando com FreeRTOS em linguagem C no VSCode. Esse sistema deve controlar três periféricos da placa de forma concorrente:  
Um LED RGB que alterna ciclicamente entre vermelho, verde e azul.  
Um buzzer que emite bipes periodicamente.  
Dois botões:  
Botão A: suspende ou retoma a tarefa do LED.  
Botão B: suspende ou retoma a tarefa do buzzer.  

## Solução

Foi implementado o programa main.c, o sistema operacional FreeRTOS do repositório em https://github.com/FreeRTOS/FreeRTOS-Kernel foi baixado via arquivo zip e copiado na pasta FreeRTOS-kernel e foram aplicadas algumas alterações no CMakeLists.txt.  

## Código

main.c:
```c
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
```
CMakeLists.txt:
```c
# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

cmake_minimum_required(VERSION 3.12)

# Set any variables required for importing libraries
if (DEFINED ENV{FREERTOS_KERNEL_PATH})
  SET(FREERTOS_KERNEL_PATH $ENV{FREERTOS_KERNEL_PATH})
else()
  SET(FREERTOS_KERNEL_PATH ${CMAKE_CURRENT_LIST_DIR}/FreeRTOS-Kernel)
endif()

message("FreeRTOS Kernel located in ${FREERTOS_KERNEL_PATH}")

# Import those libraries
include(pico_sdk_import.cmake)
include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

# Define project
project(embarcatech-freertos-tarefa-1 C CXX ASM)

# Initialize the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable(embarcatech-freertos-tarefa-1
    src/main.c
    oled/ssd1306.c
)

target_include_directories(embarcatech-freertos-tarefa-1 PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/oled
)

#pico_enable_stdio_usb(embarcatech-freertos-tarefa-1 1)


target_link_libraries(embarcatech-freertos-tarefa-1 
    pico_stdlib 
    FreeRTOS-Kernel-Heap4 
    hardware_i2c
    )

pico_add_extra_outputs(embarcatech-freertos-tarefa-1)

# if you have anything in "lib" folder then uncomment below - remember to add a CMakeLists.txt
# file to the "lib" directory
#add_subdirectory(lib)
```

## Detalhes da implementação

Problema inicial: O CMakeLists.txt principal utilizava a variável FREERTOS_PATH para definir o caminho do diretório FreeRTOS-kernel/. No entanto, o arquivo de importação do FreeRTOS para o Pico (FreeRTOS_Kernel_import.cmake) esperava uma variável chamada FREERTOS_KERNEL_PATH. Essa inconsistência impedia que o FreeRTOS fosse localizado e configurado corretamente.  
Ajuste Realizado: Todas as referências à variável FREERTOS_PATH no CMakeLists.txt principal foram alteradas para FREERTOS_KERNEL_PATH. Isso garantiu que o caminho para o diretório FreeRTOS-kernel fosse reconhecido pelo sistema de build.  

Outra consideração importante refere-se ao fato de o FreeRTOS ter sido descompactado completo, com todas as configurações que traz para trabalhar com diversos microcontroladores. Com a observação de quais eram os arquivos realmente necessários ao projeto, foi possível a deleção dos arquivos dispensáveis, de modo a enxugar o volume de dados levados ao repositório de destino do projeto.  

Para o uso do display oled SSD1306, informando o estado das tarefas do led e do buzzer, foi usado o estado de cada tarefa, informação trazida pela função `eTaskGetState()`. Como cada tarefa passa mais tempo bloqueada do que rodando, esses dois estados foram mostrados no display como rodando. O outro estado das tarefas do projeto é suspenso, quando a tarefa é suspensa com o uso dos botões.


Como o main.c Coordenou (e Continua Coordenando) o RTOS:
O main.c atua como o ponto de entrada e inicialização para o RTOS e para as funcionalidades do hardware. Ele executa os seguintes passos:  

- Inicialização de Hardware:  
`stdio_init_all();`: Inicializa a comunicação serial (para printf, usado durante o desenvolvimento para depuração).  
`gpio_init_all();`: Configura todos os pinos GPIO para LEDs, Buzzer e Botões (direção, pull-ups). Isso é feito antes do RTOS assumir o controle, garantindo que o hardware básico esteja pronto.  

- Criação das Tarefas FreeRTOS:
`xTaskCreate(led_task, "LED Task", 256, NULL, 1, &ledTaskHandle);`  
`xTaskCreate(buzzer_task, "Buzzer Task", 256, NULL, 1, &buzzerTaskHandle);`  
`xTaskCreate(button_a_task, "Button A Task", 256, NULL, 1, NULL);`  
`xTaskCreate(button_b_task, "Button B Task", 256, NULL, 1, NULL);`  
`xTaskCreate(oled_task, "OLED Task", 512, NULL, 0, &oledTaskHandle);`  
Esta é a etapa em que o programa se divide em unidades de trabalho independentes. Para cada funcionalidade (LED, Buzzer, Botão A, Botão B e display oled), uma tarefa separada é criada.  
Nesses comandos, são fornecidos:  
O nome da função que será dado a tarefa (led_task, buzzer_task, etc.).  
Um nome descritivo ("LED Task", "Buzzer Task").  
O tamanho da pilha (256 palavras) que a tarefa usará.  
Um parâmetro (NULL neste caso, mas poderia ser um dado para a tarefa).  
A prioridade (1 para todas neste exemplo).  
Um handle (por exemplo, &ledTaskHandle, &buzzerTaskHandle) que permite que outras partes do código (como as tarefas dos botões) referenciem e controlem essas tarefas específicas.  

- Início do Escalonador FreeRTOS:  
`vTaskStartScheduler();`  
Esta é uma chamada mágica. Após esta linha, o controle do programa é transferido para o escalonador do FreeRTOS. O main() nunca mais retorna de `vTaskStartScheduler()` (a menos que haja um erro crítico no RTOS).  
O escalonador assume o controle e começa a executar as tarefas que foram criadas, alternando entre elas de acordo com suas prioridades e os atrasos solicitados.  

- Loop Infinito Pós-Escalonador:  
`while (true) { /* Nunca chega aqui */ }`  
Este loop é uma salvaguarda. Como `vTaskStartScheduler()` não retorna, o código dentro deste loop nunca é executado em um sistema FreeRTOS funcional. É uma prática comum para garantir que o programa não termine caso o escalonador pare por algum motivo inesperado (o que raramente acontece em sistemas embarcados estáveis).  

De maneira geral, o main.c configura o hardware, prepara o cenário para o FreeRTOS criando todas as tarefas que irão executar as diferentes funcionalidades do seu sistema, e então passa o controle para o FreeRTOS para que ele gerencie a execução dessas tarefas de forma concorrente. As tarefas, por sua vez, contêm os loops while(true) de suas respectivas funcionalidades, que são onde o trabalho real é feito, coordenado pelo escalonador.  

# Resultado obtido

[Clique aqui e baixe o vídeo de demonstração do projeto](video/tarefartos.mp4)


# Reflexões finais

1. O que acontece se todas as tarefas tiverem a mesma prioridade?

No FreeRTOS, quando múltiplas tarefas estão prontas para serem executadas e possuem a mesma prioridade, o escalonador geralmente emprega uma política de round-robin (ou "rodízio").  
Isso significa que cada tarefa de mesma prioridade recebe um fatia de tempo (time slice) da CPU para executar.  
Ao final dessa fatia de tempo (ou se a tarefa chamar `vTaskDelay()` ou bloquear em algum recurso), o escalonador alterna para a próxima tarefa de mesma prioridade que está pronta.  
As tarefas se revezam na execução, dando a impressão de que estão rodando "em paralelo".  
Todas as tarefas (led_task, buzzer_task, button_a_task, button_b_task) foram criadas com a prioridade 1. Portanto, elas compartilharão o tempo da CPU em um esquema round-robin.  

2. Qual tarefa consome mais tempo da CPU?  

Analisando o main.c e as chamadas `vTaskDelay()`:  
led_task: Atraso de 500ms.  
buzzer_task: Atraso de 950ms (após um beep de 50ms, totalizando 1 segundo).  
button_a_task e button_b_task: Atraso de 20ms.  
oled_task: Atraso de 200ms.  
As tarefas que chamam `vTaskDelay()` com os menores valores de tempo são as que se tornam prontas para execução com mais frequência. Consequentemente, elas serão escalonadas (executadas) mais vezes pelo RTOS e, portanto, consumirão mais tempo da CPU em termos de frequência de execução.  
Neste caso, as tarefas button_a_task e button_b_task são as que têm o menor `vTaskDelay()` (20ms). Isso significa que elas acordam a cada 20ms para verificar o estado dos botões. Embora o trabalho que elas fazem em cada ativação seja mínimo (ler um GPIO e talvez chamar `vTaskSuspend`ou `vTaskResume`), a frequência com que são executadas as torna as maiores consumidoras de tempo de CPU no cenário geral, pois o escalonador estará constantemente alternando para elas.  

3. Quais seriam os riscos de usar polling sem prioridades?  

Usar polling sem prioridades (ou com prioridades iguais) pode levar a alguns riscos importantes, especialmente em sistemas de tempo real:  

- Consumo Ineficiente de CPU:  0
Se uma tarefa usa polling em um loop muito apertado (por exemplo, `while(true)` sem `vTaskDelay()` ou bloqueio em alguma função RTOS), ela consumirá 100% da fatia de tempo da CPU que lhe for atribuída, mesmo que não haja nada significativo para fazer.  
Sem prioridades, todas as tarefas competem igualmente. Uma tarefa de polling "ocupada" pode impedir que outras tarefas de mesma prioridade obtenham tempo de CPU em tempo hábil.  

- Latência e Não-Responsividade:  
Se uma tarefa de polling está executando por um período prolongado (mesmo que com `vTaskDelay)`, ela pode atrasar a execução de outras tarefas importantes (mesmo as de prioridade igual) que precisam reagir a eventos em tempo real.  
Em um sistema sem prioridades (ou com prioridades iguais), não há garantia de que uma tarefa crítica (ex: ler um sensor rápido) será executada no momento certo se uma tarefa de polling menos importante estiver usando sua fatia de tempo.  

- Falta de Determinismo:  
Sistemas de tempo real precisam ser determinísticos, ou seja, suas ações devem ocorrer dentro de limites de tempo previsíveis. O polling excessivo, especialmente sem prioridades, pode tornar o comportamento do sistema imprevisível, pois não há garantia de qual tarefa será executada quando.  

- Consumo de Energia (para sistemas alimentados por bateria):  
Fazer polling muito frequentemente (como a cada 20ms) mantém a CPU ativa mais vezes, consumindo mais energia do que se a tarefa pudesse "dormir" por períodos mais longos e ser despertada por uma interrupção (abordagem baseada em interrupções, que é mais eficiente para botões).  
Embora nesse projeto as tarefas dos botões estejam usando polling (lendo o estado do GPIO), elas estão usando `vTaskDelay(pdMS_TO_TICKS(20))` no final do loop. Este `vTaskDelay` faz com que a tarefa ceda o controle da CPU ao escalonador e "durma" por 20ms. Isso evita que ela consuma 100% da CPU e permite que outras tarefas sejam executadas. Sem esse `vTaskDelay`, ou se o valor fosse ainda menor, a tarefa dos botões seria um "devorador" de CPU.  
O ideal para botões é usar interrupções (ISR) ao invés de polling, pois o microcontrolador só é interrompido e a tarefa do botão só é acionada quando o estado do pino realmente muda, economizando CPU.  

---

## 📜 Licença
GNU GPL-3.0.
