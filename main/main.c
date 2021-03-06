#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include <string.h>

#include "esp_netif.h"
#include "esp_sntp.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "rgb.h"
#include <driver/gpio.h>
#include <lwip/netdb.h>

void communicate_task(void *args);

#define EXAMPLE_ESP_MAXIMUM_RETRY 5

struct
{
    char sta_ssid[32];
    char sta_password[32];
} config;


static const char *TAG = "light";

static int s_retry_num = 0;

static void tcp_server_task();
void startAp();

void startSta();
void wifi_scan(void);

void print_auth_mode(int authmode);
void print_cipher_type(int pairwise_cipher, int group_cipher);

void test_led(void *);
void show_ip(uint32_t ip);
uint32_t my_ip;

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        // 初始化WIFI成功
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY)
        {
            // WIFI连接失败，尝试重连
            wifi_event_sta_disconnected_t *disconnected = (wifi_event_sta_disconnected_t *)event_data;
            ESP_LOGE(TAG, "Disconnect reason **%s**: %d", disconnected->ssid, disconnected->reason);
            s_retry_num++;
            esp_wifi_connect();
            // esp_wifi_stop();
            // esp_wifi_start();
            ESP_LOGI(TAG, "retry to connect to the AP %d/%d", s_retry_num, EXAMPLE_ESP_MAXIMUM_RETRY);
        }
        else
        {
            // 重试三次失败，启动AP模式
            s_retry_num = 0;
            startAp();
        }
        ESP_LOGI(TAG, "connect to the AP fail");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        // 连接WIFI成功
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        my_ip = event->ip_info.ip.addr;
        // show_ip(event->ip_info.ip.addr);
        // xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        // Start TCP server.
        // xTaskCreate(tcp_server_task, "tcp_server", 4096, NULL, 5, NULL);
    }
    else if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        // AP模式有客户端连接
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " join, AID=%d", MAC2STR(event->mac), event->aid);
    }
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        // AP模式客户端断开连接
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d", MAC2STR(event->mac), event->aid);
    }
}


// wifi_ap_record_t ap_info[32];
void startSta()
{
    ESP_ERROR_CHECK(esp_netif_init());

    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, NULL));

    wifi_config_t cf1 = {
        // .sta = {
        //     .ssid = "TP-LINK_DE06FA",
        //     .password = "G313G313G313"
        // }
    };
    strcpy((char *)cf1.sta.ssid, config.sta_ssid);
    memcpy(cf1.sta.password, config.sta_password, 32);
    cf1.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    // cf1.sta.pmf_cfg.capable = true;
    // cf1.sta.pmf_cfg.required = false;


    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &cf1));
    ESP_ERROR_CHECK(esp_wifi_start());

    return;
    // uint16_t number = 32;
    // uint16_t ap_count = 0;
    // memset(ap_info, 0, sizeof(ap_info));

    // ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, true));
    // ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
    // ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    // ESP_LOGI(TAG, "Total APs scanned = %u", ap_count);
    // for (int i = 0; (i < 32) && (i < ap_count); i++) {
    //     ESP_LOGI(TAG, "SSID \t\t%s", ap_info[i].ssid);
    //     ESP_LOGI(TAG, "RSSI \t\t%d", ap_info[i].rssi);
    //     print_auth_mode(ap_info[i].authmode);
    //     if (ap_info[i].authmode != WIFI_AUTH_WEP) {
    //         print_cipher_type(ap_info[i].pairwise_cipher, ap_info[i].group_cipher);
    //     }
    //     ESP_LOGI(TAG, "Channel \t\t%d\n", ap_info[i].primary);
    // }
}

void hexToChar2(uint8_t hex, char *str)
{
    unsigned char val = hex >> 4;
    str[0] = val + (val < 10 ? '0' : ('A' - 10));
    val = hex & 0xF;
    str[1] = val + (val < 10 ? '0' : ('A' - 10));
}

void startAp()
{
    ESP_ERROR_CHECK(esp_netif_init());
    esp_netif_t *wifiAP = esp_netif_create_default_wifi_ap();

    esp_netif_ip_info_t ipInfo;
    IP4_ADDR(&ipInfo.ip, 192, 168, 222, 1);
    IP4_ADDR(&ipInfo.gw, 0, 0, 0, 0);
    IP4_ADDR(&ipInfo.netmask, 255, 255, 255, 0);
    esp_netif_dhcps_stop(wifiAP);
    esp_netif_set_ip_info(wifiAP, &ipInfo);
    esp_netif_dhcps_start(wifiAP);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL));

    uint8_t mac[10];
    char myMacStr[18] = {0};
    char buf[32] = {0};
    ESP_ERROR_CHECK(esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP));
    for (int i = 0; i < 8; i++)
    {
        hexToChar2(mac[i], myMacStr + 2 * i);
    }
    strcpy(buf, "RGBLIGHT-");
    strcat(buf, myMacStr);

    wifi_config_t cf1 = {
        .ap = {.channel = 4, .password = "12345678", .max_connection = 2, .authmode = WIFI_AUTH_WPA_WPA2_PSK},
    };
    strcpy((char *)cf1.ap.ssid, buf);
    cf1.ap.ssid_len = strlen(buf);

    ESP_LOGI(TAG, "Start AP SSID:%s, PASSWORD:%s", buf, "12345678");

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &cf1));
    ESP_ERROR_CHECK(esp_wifi_start());
}

static void tcp_server_task()
{
    char addr_str[128];
    int addr_family = AF_INET;
    struct sockaddr_storage dest_addr;

    struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
    dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr_ip4->sin_family = AF_INET;
    dest_addr_ip4->sin_port = htons(4617);

    int listen_sock = socket(addr_family, SOCK_STREAM, IPPROTO_IP);
    if (listen_sock < 0)
    {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }
    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    ESP_LOGI(TAG, "Socket created");

    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0)
    {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        ESP_LOGE(TAG, "IPPROTO: %d", addr_family);
        goto CLEAN_UP;
    }
    ESP_LOGI(TAG, "Socket bound, port %d", 4617);

    err = listen(listen_sock, 1);
    if (err != 0)
    {
        ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
        goto CLEAN_UP;
    }

    while (1)
    {
        ESP_LOGI(TAG, "Socket listening");
        struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
        socklen_t addr_len = sizeof(source_addr);
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        if (sock < 0)
        {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
            break;
        }

        // Convert ip address to string
        inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
        ESP_LOGI(TAG, "Socket accepted ip address: %s", addr_str);

        // 接受客户端连接
        xTaskCreate(communicate_task, "conn_task", 4096, (void *)sock, 5, NULL);


        // do_retransmit(sock);

        // shutdown(sock, 0);
        // close(sock);
    }

CLEAN_UP:
    close(listen_sock);
    vTaskDelete(NULL);
}

int led_state = 0;
extern int led_strip_len;
extern int total_frames;
extern int ms_per_frame;
extern rgb_t colors[240][60];

void default_led()
{
    total_frames = 240;
    ms_per_frame = 100;
    memset(colors, 0, sizeof(colors));
    for (int i = 0; i < total_frames; i++)
    {
        rgb_t d = {.r = 255, .g = 0, .b = 0};
        colors[i][i % 60] = d;
    }
}

void show_ip(uint32_t ip)
{
    total_frames = 1;
    ms_per_frame = 10;
    memset(colors, 0, sizeof(colors));
    for (int i = 0; i < 32; i++)
    {
        rgb_t c = {};
        if (ip & (1 << i))
        {
            if (i >= 0 && i < 8)
            {
                c.r = 255;
            }
            else if (i >= 8 && i < 16)
            {
                c.g = 255;
            }
            else if (i >= 16 && i < 24)
            {
                c.b = 255;
            }
            else
            {
                c.r = 255;
                c.g = 255;
            }
        }
        colors[0][i] = c;
    }
    // for (int i = 32; i < 60; i++)
    // {
    //     rgb_t c = {.r = 255, .g = 255, .b = 255};
    //     colors[0][i] = c;
    // }
}

void init_led()
{
    nvs_handle_t nvs_handler = -1;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &nvs_handler);
    if (err != ESP_OK)
    {
        ESP_LOGE("init_led", "cannot open nvs storage in init_led %s", esp_err_to_name(err));
        return;
    }
    err = nvs_get_i32(nvs_handler, "total_frames", &total_frames);
    if (err != ESP_OK)
    {
        ESP_LOGW("init_led", "cannot load total_frames, init default led.");
        return default_led();
    }
    ESP_LOGI("init_led", "load total_frames: %d", total_frames);
    char key[32] = {};
    for (int i = 0; i < total_frames; i++)
    {
        sprintf(key, "frame-%d", i);
        // err = nvs_get_blob(nvs_handler, key, colors[i], 180);
        if (err != ESP_OK)
        {
            ESP_LOGE("init_led", "cannot load frame %d", i);
            break;
        }
        ESP_LOGI("init_led", "load frame %d/%d", i + 1, total_frames);
    }
}

void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    nvs_handle_t nvs_handle;
    ret = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK)
    {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(ret));
    }

    nvs_stats_t stats;
    ret = nvs_get_stats(NULL, &stats);
    if (ret != ESP_OK)
    {
        printf("Error (%s) getting nvs stats!\n", esp_err_to_name(ret));
    }
    else
    {
        printf("used_entries: %d\nfree_entries: %d\ntotle_entries: %d\nnamespace_count: %d\n",
               stats.used_entries,
               stats.free_entries,
               stats.total_entries,
               stats.namespace_count);
    }

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    config.sta_ssid[0] = 0;
    size_t length = 32;
    ret = nvs_get_str(nvs_handle, "sta_ssid", config.sta_ssid, &length);
    if (ret == ESP_OK)
    {
        length = 32;
        ret = nvs_get_str(nvs_handle, "sta_password", config.sta_password, &length);
        config.sta_password[length] = 0;
        if (ret != ESP_OK)
        {
            // config.sta_ssid[0] = 0;
        }
    }
    nvs_close(nvs_handle);


    if (config.sta_ssid[0] == 0)
    {
        ESP_LOGI(TAG, "Cannot get wifi info, run as ap mode");
        startAp();
    }
    else
    {
        ESP_LOGI(TAG, "Get wifi info ssid:%s, password:%s", config.sta_ssid, config.sta_password);
        startSta();
    }
    // init_led();
    default_led();

    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "ntp.aliyun.com");
    sntp_setservername(1, "ntp.tencent.com");
    sntp_setservername(2, "cn.pool.ntp.org");
    sntp_init();

    ESP_LOGI(TAG, "START Socket");
    xTaskCreate(tcp_server_task, "tcp_server", 4096, NULL, 5, NULL);
    xTaskCreate(test_led, "test_led", 4096, NULL, 5, NULL);
}
