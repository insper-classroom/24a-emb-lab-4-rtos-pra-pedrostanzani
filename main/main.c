/*
 * LED blink with FreeRTOS
 */
#include <FreeRTOS.h>
#include <queue.h>
#include <semphr.h>
#include <stdio.h>
#include <task.h>

#include "gfx.h"
#include "pico/stdlib.h"
#include "ssd1306.h"

const uint BTN_1_OLED = 28;
const uint BTN_2_OLED = 26;
const uint BTN_3_OLED = 27;

const uint LED_1_OLED = 20;
const uint LED_2_OLED = 21;
const uint LED_3_OLED = 22;

void oled1_btn_led_init(void) {
    gpio_init(LED_1_OLED);
    gpio_set_dir(LED_1_OLED, GPIO_OUT);

    gpio_init(LED_2_OLED);
    gpio_set_dir(LED_2_OLED, GPIO_OUT);

    gpio_init(LED_3_OLED);
    gpio_set_dir(LED_3_OLED, GPIO_OUT);

    gpio_init(BTN_1_OLED);
    gpio_set_dir(BTN_1_OLED, GPIO_IN);
    gpio_pull_up(BTN_1_OLED);

    gpio_init(BTN_2_OLED);
    gpio_set_dir(BTN_2_OLED, GPIO_IN);
    gpio_pull_up(BTN_2_OLED);

    gpio_init(BTN_3_OLED);
    gpio_set_dir(BTN_3_OLED, GPIO_IN);
    gpio_pull_up(BTN_3_OLED);
}

void oled1_demo_1(void *p) {
    printf("Inicializando Driver\n");
    ssd1306_init();

    printf("Inicializando GLX\n");
    ssd1306_t disp;
    gfx_init(&disp, 128, 32);

    printf("Inicializando btn and LEDs\n");
    oled1_btn_led_init();

    char cnt = 15;
    while (1) {
        if (gpio_get(BTN_1_OLED) == 0) {
            cnt = 15;
            gpio_put(LED_1_OLED, 0);
            gfx_clear_buffer(&disp);
            gfx_draw_string(&disp, 0, 0, 1, "LED 1 - ON");
            gfx_show(&disp);
        } else if (gpio_get(BTN_2_OLED) == 0) {
            cnt = 15;
            gpio_put(LED_2_OLED, 0);
            gfx_clear_buffer(&disp);
            gfx_draw_string(&disp, 0, 0, 1, "LED 2 - ON");
            gfx_show(&disp);
        } else if (gpio_get(BTN_3_OLED) == 0) {
            cnt = 15;
            gpio_put(LED_3_OLED, 0);
            gfx_clear_buffer(&disp);
            gfx_draw_string(&disp, 0, 0, 1, "LED 3 - ON");
            gfx_show(&disp);
        } else {
            gpio_put(LED_1_OLED, 1);
            gpio_put(LED_2_OLED, 1);
            gpio_put(LED_3_OLED, 1);
            gfx_clear_buffer(&disp);
            gfx_draw_string(&disp, 0, 0, 1, "PRESSIONE ALGUM");
            gfx_draw_string(&disp, 0, 10, 1, "BOTAO");
            gfx_draw_line(&disp, 15, 27, cnt, 27);
            vTaskDelay(pdMS_TO_TICKS(50));
            if (++cnt == 112) cnt = 15;

            gfx_show(&disp);
        }
    }
}

void oled1_demo_2(void *p) {
    printf("Inicializando Driver\n");
    ssd1306_init();

    printf("Inicializando GLX\n");
    ssd1306_t disp;
    gfx_init(&disp, 128, 32);

    printf("Inicializando btn and LEDs\n");
    oled1_btn_led_init();

    while (1) {
        gfx_clear_buffer(&disp);
        gfx_draw_string(&disp, 0, 0, 1, "Mandioca");
        gfx_show(&disp);
        vTaskDelay(pdMS_TO_TICKS(150));

        gfx_clear_buffer(&disp);
        gfx_draw_string(&disp, 0, 0, 2, "Batata");
        gfx_show(&disp);
        vTaskDelay(pdMS_TO_TICKS(150));

        gfx_clear_buffer(&disp);
        gfx_draw_string(&disp, 0, 0, 4, "Inhame");
        gfx_show(&disp);
        vTaskDelay(pdMS_TO_TICKS(150));
    }
}

// Lab code starts here

QueueHandle_t xQueueTime, xQueueDistance;
SemaphoreHandle_t xSemaphoreTrigger;

const uint ECHO_PIN = 5;
const uint TRIG_PIN = 6;
const uint TIMEOUT_IN_MS = 24;     // [us]
const uint TIMEOUT_IN_US = 23500;  // [us]

void pin_callback(uint gpio, uint32_t events) {
    uint32_t time;

    if (events == 0x4) {  // fall edge
        if (gpio == ECHO_PIN) {
            time = to_us_since_boot(get_absolute_time());
        };
    } else if (events == 0x8) {  // rise edge
        if (gpio == ECHO_PIN) {
            time = to_us_since_boot(get_absolute_time());
        }
    }

    xQueueSendFromISR(xQueueTime, &time, 0);
}

void echo_task(void *p) {
    gpio_init(ECHO_PIN);
    gpio_set_dir(ECHO_PIN, GPIO_IN);

    gpio_set_irq_enabled_with_callback(
        ECHO_PIN, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &pin_callback);

    uint32_t trigger_timestamps[2];
    int times_read = 0;

    for (;;) {
        if (xQueueReceive(xQueueTime, &trigger_timestamps[times_read],
                          pdMS_TO_TICKS(100))) {
            times_read++;

            if (times_read == 2) {
                printf("Echo de saída...\n");
                double distance = 0.0;
                uint32_t startTime = trigger_timestamps[0];
                uint32_t endTime = trigger_timestamps[1];
                uint32_t time_delta = endTime - startTime;
                if (time_delta > TIMEOUT_IN_US) {
                    distance = -1.0;
                } else {
                    distance = (endTime - startTime) * 0.03403 / 2;
                }
                xQueueSend(xQueueDistance, &distance, 0);
                times_read = 0;
            } else {
                printf("Echo de entrada...\n");
            }
        }
    }
}

void trigger_task(void *p) {
    gpio_init(TRIG_PIN);
    gpio_set_dir(TRIG_PIN, GPIO_OUT);

    while (true) {
        gpio_put(TRIG_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(5));
        gpio_put(TRIG_PIN, 0);
        xSemaphoreGive(xSemaphoreTrigger);
        vTaskDelay(pdMS_TO_TICKS(495));
    }
}

void oled_task(void *p) {
    printf("Inicializando Driver\n");
    ssd1306_init();

    printf("Inicializando GLX\n");
    ssd1306_t disp;
    gfx_init(&disp, 128, 32);

    printf("Inicializando btn and LEDs\n");
    oled1_btn_led_init();

    double distance;
    char distanceStr[12];
    const int maxWidth = 128; // Maximum width of the bar, corresponding to 400cm

    while (1) {
        if (xSemaphoreTake(xSemaphoreTrigger, pdMS_TO_TICKS(100)) == pdTRUE) {
            vTaskDelay(pdMS_TO_TICKS(TIMEOUT_IN_MS));

            if (xQueueReceive(xQueueDistance, &distance, pdMS_TO_TICKS(10))) {
                if (distance >= 0) {
                    gfx_clear_buffer(&disp);
                    snprintf(distanceStr, sizeof(distanceStr), "Dist: %d", (int) distance);
                    gfx_draw_string(&disp, 0, 0, 2, distanceStr);
                    int barLength = (int)((distance / 300.0) * maxWidth);
                    if (barLength > maxWidth) {
                        barLength = maxWidth; // Ensure the bar does not exceed the maximum width
                    }
                    gfx_draw_line(&disp, 0, 31, barLength, 31); // Draw the bar at the bottom of the OLED
                    gfx_show(&disp);
                    vTaskDelay(pdMS_TO_TICKS(150));
                } else {
                    printf("Timeout\n");
                    gfx_clear_buffer(&disp);
                    gfx_draw_string(&disp, 0, 0, 2, "Failed");
                    int barLength = 0;
                    gfx_draw_line(&disp, 0, 31, barLength, 31); // Draw the bar at the bottom of the OLED
                    gfx_show(&disp);
                    vTaskDelay(pdMS_TO_TICKS(150));
                }
            }
        }
    }
}

// Lab code ends here

int main() {
    stdio_init_all();

    // Semaphores
    xSemaphoreTrigger = xSemaphoreCreateBinary();

    // Queues
    xQueueTime = xQueueCreate(32, sizeof(uint32_t));
    if (xQueueTime == NULL) {
        printf("Falha em criar a fila xQueueTime... \n");
    }

    xQueueDistance = xQueueCreate(32, sizeof(double));
    if (xQueueDistance == NULL) {
        printf("Falha em criar a fila xQueueDistance... \n");
    }

    // Tasks
    // xTaskCreate(oled1_demo_2, "Demo 2", 4095, NULL, 1, NULL);
    xTaskCreate(echo_task, "EchoTask", 4095, NULL, 1, NULL);
    xTaskCreate(oled_task, "OLEDTask", 4095, NULL, 1, NULL);
    xTaskCreate(trigger_task, "TriggerTask", 4095, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true)
        ;
}
