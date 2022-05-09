#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include <lwip/netdb.h>

#include "esp_netif.h"

#define TAG_HELLO 1
#define TAG_SET 2
#define TAG_GET 3
#define TAG_ADD_EFFECT 5
#define TAG_DEL_EFFECT 6
#define TAG_SET_EFFECT 7
#define TAG_RESTART 8
#define TAG_NVS_COMMIT 9
#define TAG_GET_INFO 10

#define LOGI(...) ESP_LOGI("COMM", __VA_ARGS__)
#define LOGE(...) ESP_LOGE("COMM", __VA_ARGS__)

/***
 * TAG_SET: 用于设置变量。
 * 
 * */

 char light_data[128 * 1024] = {};


 struct {
     size_t free_heap_size;
 } deviceInfo;


#define recv_or_end(sock, ptr, len) do {int __n_; __n_ = recv(sock, ptr, len, MSG_WAITALL); if (__n_ < len) { goto end; }} while (0)


void communicate_task(void *args) {
    int sock = (int)(args);
    uint8_t tag;
    uint8_t len;        
    char key_buf[32];
    char val_buf[32];
    nvs_handle_t nvs_handle = -1;
    esp_err_t err;
    while (1) {
        recv_or_end(sock, &tag, 1);
        switch(tag) {
            case TAG_HELLO: {
                send(sock, "Hello", 6, 0);
                break;
            }
            case TAG_SET: {
                recv_or_end(sock, &len, 1);
                recv_or_end(sock, key_buf, len);
                key_buf[len] = 0;
                recv_or_end(sock, &len, 1);
                recv_or_end(sock, val_buf, len);
                val_buf[len] = 0;
                LOGI("set var %s=%s", key_buf, val_buf);
                if (nvs_handle == -1) {
                    err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
                    if (err != ESP_OK) {
                        LOGE("Cannot open nvs storage");
                        break;
                    }
                }
                err = nvs_set_str(nvs_handle, key_buf, val_buf);
                if (err != ESP_OK) {
                    LOGE("Write nvs error %d", err);
                    break;
                }
                // err = nvs_commit(nvs_handle);
                // if (err != ESP_OK) {
                //     LOGE("nvs commit error");
                // } 
                break;
            }
            case TAG_NVS_COMMIT: {
                err = nvs_commit(nvs_handle);
                if (err != ESP_OK) {
                    LOGE("nvs commit error %d", err);
                } else {
                    LOGI("nvs commit");
                }
                break;
            }
            case TAG_GET_INFO: {
                deviceInfo.free_heap_size = esp_get_free_heap_size();
                LOGI("free heap size %u", deviceInfo.free_heap_size);
                light_data[deviceInfo.free_heap_size % 1024] = deviceInfo.free_heap_size;
                send(sock, &deviceInfo, sizeof(deviceInfo), MSG_WAITALL);
                break;
            }
            default: {
                LOGE("Unknown tag %d", tag);
                break;
            }
        }
    }

    // size_t maxv = 32;
end:
    LOGI("Connection close");
    if (nvs_handle != -1) {
        // val_buf[0] = 0;
        // nvs_get_str(nvs_handle, "sta_ssid", val_buf, &maxv);
        // LOGE("get nvs sta_ssid: (%d)%s", maxv, val_buf);
        // maxv = 32;
        // nvs_get_str(nvs_handle, "sta_password", val_buf, &maxv);
        // LOGE("get nvs sta_password: (%d)%s", maxv, val_buf);
        nvs_close(nvs_handle);
    }
    close(sock);
    vTaskDelete(NULL);
}