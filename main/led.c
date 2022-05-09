#include "rgb.h"
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <led_strip.h>

#define LED_TYPE LED_STRIP_WS2812
#define LED_GPIO 13
#define CONFIG_LED_STRIP_LEN 60

// static const rgb_t colors[] = {
//     { .r = 0x0f, .g = 0x0f, .b = 0x0f },
//     { .r = 0x00, .g = 0x00, .b = 0x2f },
//     { .r = 0x00, .g = 0x2f, .b = 0x00 },
//     { .r = 0x2f, .g = 0x00, .b = 0x00 },
//     { .r = 0x00, .g = 0x00, .b = 0x00 },
// };

void led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b)
{
    h %= 360; // h -> [0,360]
    uint32_t rgb_max = v * 2.55f;
    uint32_t rgb_min = rgb_max * (100 - s) / 100.0f;

    uint32_t i = h / 60;
    uint32_t diff = h % 60;

    // RGB adjustment amount by hue
    uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

    switch (i) {
    case 0:
        *r = rgb_max;
        *g = rgb_min + rgb_adj;
        *b = rgb_min;
        break;
    case 1:
        *r = rgb_max - rgb_adj;
        *g = rgb_max;
        *b = rgb_min;
        break;
    case 2:
        *r = rgb_min;
        *g = rgb_max;
        *b = rgb_min + rgb_adj;
        break;
    case 3:
        *r = rgb_min;
        *g = rgb_max - rgb_adj;
        *b = rgb_max;
        break;
    case 4:
        *r = rgb_min + rgb_adj;
        *g = rgb_min;
        *b = rgb_max;
        break;
    default:
        *r = rgb_max;
        *g = rgb_min;
        *b = rgb_max - rgb_adj;
        break;
    }
}

#define COLORS_TOTAL (sizeof(colors) / sizeof(rgb_t))

void test_led(void *pvParameters)
{
    led_strip_install();
    led_strip_t strip = {
        .type = LED_TYPE,
        .length = CONFIG_LED_STRIP_LEN,
        .gpio = LED_GPIO,
        .buf = NULL,
#ifdef LED_STRIP_BRIGHTNESS
        .brightness = 255,
#endif
    };

    ESP_ERROR_CHECK(led_strip_init(&strip));

    // size_t c = 0;
    // while (1)
    // {
    //     ESP_ERROR_CHECK(led_strip_fill(&strip, 0, strip.length, colors[c]));
    //     ESP_ERROR_CHECK(led_strip_flush(&strip));

    //     vTaskDelay(pdMS_TO_TICKS(1000));

    //     if (++c >= COLORS_TOTAL)
    //         c = 0;
    // }
    uint32_t red = 0;
    uint32_t green = 0;
    uint32_t blue = 0;
    uint16_t hue = 0;
    uint16_t start_rgb = 0;
    while (true) {
        for (int i = 0; i < 3; i++) {
            for (int j = i; j < strip.length; j += 3) {
                // Build RGB values
                hue = j * 360 / strip.length + start_rgb;
                led_strip_hsv2rgb(hue, 100, 100, &red, &green, &blue);
                // Write RGB values to strip driver
                rgb_t rgb = {
                    .r = red,
                    .g = green,
                    .b = blue
                };
                led_strip_set_pixel(&strip, j, rgb);
                // ESP_ERROR_CHECK(strip->set_pixel(strip, j, red, green, blue));
            }
            // Flush RGB values to LEDs
            led_strip_flush(&strip);
            // ESP_ERROR_CHECK(strip->refresh(strip, 100));
            vTaskDelay(pdMS_TO_TICKS(10));
            // strip->clear(strip, 50);
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        start_rgb += 60;
    }
}

// void app_main()
// {
//     led_strip_install();
//     xTaskCreate(test, "test", configMINIMAL_STACK_SIZE * 5, NULL, 5, NULL);
// }

