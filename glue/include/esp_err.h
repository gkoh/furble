#ifndef NIMBLE_STUB_ESP_ERR_H_
#define NIMBLE_STUB_ESP_ERR_H_
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef int32_t esp_err_t;
#define ESP_OK          0
#define ESP_FAIL        -1
#define ESP_ERR_NO_MEM          0x101
#define ESP_ERR_INVALID_ARG     0x102
#define ESP_ERR_INVALID_STATE   0x103
#define ESP_ERR_INVALID_SIZE    0x104
#define ESP_ERR_NOT_FOUND       0x105
#define ESP_ERR_NOT_SUPPORTED   0x106
#define ESP_ERR_TIMEOUT         0x107
#define ESP_ERR_INVALID_RESPONSE 0x108
#define ESP_ERR_INVALID_CRC     0x109
#define ESP_ERR_INVALID_VERSION 0x10A
#define ESP_ERR_INVALID_MAC     0x10B
#define ESP_ERR_NVS_BASE         0x1100
#define ESP_ERR_NVS_NOT_INITIALIZED   (ESP_ERR_NVS_BASE + 0x01)
#define ESP_ERR_NVS_NOT_FOUND         (ESP_ERR_NVS_BASE + 0x02)
#define ESP_ERR_NVS_TYPE_MISMATCH     (ESP_ERR_NVS_BASE + 0x03)
#define ESP_ERR_NVS_READ_ONLY         (ESP_ERR_NVS_BASE + 0x04)
#define ESP_ERR_NVS_NOT_ENOUGH_SPACE  (ESP_ERR_NVS_BASE + 0x05)
#define ESP_ERR_NVS_INVALID_NAME      (ESP_ERR_NVS_BASE + 0x06)
#define ESP_ERR_NVS_INVALID_HANDLE    (ESP_ERR_NVS_BASE + 0x07)
#define ESP_ERR_NVS_REMOVE_FAILED     (ESP_ERR_NVS_BASE + 0x08)
#define ESP_ERR_NVS_KEY_TOO_LONG      (ESP_ERR_NVS_BASE + 0x09)
#define ESP_ERR_NVS_PAGE_FULL         (ESP_ERR_NVS_BASE + 0x0a)
#define ESP_ERR_NVS_INVALID_STATE     (ESP_ERR_NVS_BASE + 0x0b)
#define ESP_ERR_NVS_INVALID_LENGTH    (ESP_ERR_NVS_BASE + 0x0c)
#ifdef __cplusplus
}
#endif
#endif
