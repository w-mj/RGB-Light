#include "rgb.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <led_strip.h>
#include <stdio.h>
#include <time.h>

#define LED_TYPE LED_STRIP_WS2812
#define LED_GPIO 13
#define CONFIG_LED_STRIP_LEN 60
int led_strip_len = CONFIG_LED_STRIP_LEN;

// static const rgb_t colors[] = {
//     { .r = 0x0f, .g = 0x0f, .b = 0x0f },
//     { .r = 0x00, .g = 0x00, .b = 0x2f },
//     { .r = 0x00, .g = 0x2f, .b = 0x00 },
//     { .r = 0x2f, .g = 0x00, .b = 0x00 },
//     { .r = 0x00, .g = 0x00, .b = 0x00 },
// };

rgb_t colors[240][60];

int ms_per_frame = 100;
int total_frames = 100;
int led_mode = 0;

void led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b)
{
    h %= 360; // h -> [0,360]
    uint32_t rgb_max = v * 2.55f;
    uint32_t rgb_min = rgb_max * (100 - s) / 100.0f;

    uint32_t i = h / 60;
    uint32_t diff = h % 60;

    // RGB adjustment amount by hue
    uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

    switch (i)
    {
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

extern uint32_t my_ip;
#define COLORS_TOTAL (sizeof(colors) / sizeof(rgb_t))

static int custom_effect(rgb_t color[60])
{
    static int frame = 0;
    for (int i = 0; i < led_strip_len; i++)
    {
        color[i] = colors[frame][i];
    }
    frame = (frame + 1) % total_frames;
    return ms_per_frame;
}

static int time_effect(rgb_t color[60])
{
    static bool rev = false;
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    for (int i = 0; i < 60; i++)
    {
        int sec_index = timeinfo.tm_sec;
        int min_index = timeinfo.tm_min;
        int hou_index = (timeinfo.tm_hour % 12) * 5 + (min_index / 12);
        rgb_t c = {};
        if (i == hou_index)
        {
            if (hou_index == min_index)
            {
                c.red = rev ? 0 : 255;
                c.blue = rev ? 255 : 0;
            }
            else
            {
                c.red = 255;
            }
        }
        else if (i == min_index)
        {
            c.blue = 255;
        }
        else if (i == sec_index)
        {
            c.green = rev ? 255 : 0;
        }
        color[i] = c;
    }
    rev = !rev;
    return 500;
}

static int ip_effect(rgb_t color[60])
{
    uint32_t ip = my_ip;
    // 显示IP模式
    unsigned ii = 0;
    for (int i = 0; i < 60; i++)
    {
        rgb_t c = {};
        if (i == 0 || i == 9 || i == 18 || i == 27 || i == 36)
        {
            c.g = 255;
            c.r = 255;
        }
        else if (i > 36)
        {
            // do nothing
        }
        else
        {
            if (ip & (1 << ii))
            {
                if (i >= 1 && i < 9)
                {
                    c.r = 255;
                }
                else if (i >= 10 && i < 18)
                {
                    c.g = 255;
                }
                else if (i >= 19 && i < 27)
                {
                    c.b = 255;
                }
                else
                {
                    c.r = 255;
                }
            }
            ii++;
        }
        color[i] = c;
    }
    return 1000;
}

static int flow_effect(rgb_t color[60])
{
    static int start = 0;
    for (int i = 0; i < 60; i++)
    {
        rgb_t c = {};
        c.r = i % 3 == 0 ? 255 : 1;
        c.g = i % 3 == 1 ? 255 : 1;
        c.b = i % 3 == 2 ? 255 : 1;
        color[(i + start) % 60] = c;
    }
    start = (start + 1) % 60;
    return 200;
}


typedef int (*led_effect_t)(rgb_t[60]);

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
    // 将时区设置为中国标准时间
    setenv("TZ", "CST-8", 1);
    tzset();

    led_effect_t effects[] = {custom_effect, time_effect, ip_effect, flow_effect};
    const int effect_count = sizeof(effects) / sizeof(effects[0]);
    rgb_t color[60];
    while (true)
    {
        if (led_mode >= 0 && led_mode < effect_count)
        {
            int delay = effects[led_mode](color);
            led_strip_set_pixels(&strip, 0, 60, color);
            led_strip_flush(&strip);
            led_strip_wait(&strip, pdMS_TO_TICKS(delay));
            vTaskDelay(pdMS_TO_TICKS(delay));
        }
    }
}
