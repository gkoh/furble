#ifndef NIMBLE_STUB_ESP_LOG_H_
#define NIMBLE_STUB_ESP_LOG_H_
#include <stdio.h>
#ifndef ESP_LOGI
#define ESP_LOGI(tag, fmt, ...) fprintf(stderr, "I %s: " fmt "\n", tag, ##__VA_ARGS__)
#endif
#ifndef ESP_LOGE
#define ESP_LOGE(tag, fmt, ...) fprintf(stderr, "E %s: " fmt "\n", tag, ##__VA_ARGS__)
#endif
#ifndef ESP_LOGW
#define ESP_LOGW(tag, fmt, ...) fprintf(stderr, "W %s: " fmt "\n", tag, ##__VA_ARGS__)
#endif
#ifndef ESP_LOGD
#define ESP_LOGD(tag, fmt, ...) fprintf(stderr, "D %s: " fmt "\n", tag, ##__VA_ARGS__)
#endif
#ifndef ESP_LOGV
#define ESP_LOGV(tag, fmt, ...) ((void)0)
#endif
#ifndef LOG_COLOR
#define LOG_COLOR(x) ""
#endif
#ifndef LOG_COLOR_BLACK
#define LOG_COLOR_BLACK ""
#endif
#ifndef LOG_COLOR_RED
#define LOG_COLOR_RED ""
#endif
#ifndef LOG_COLOR_GREEN
#define LOG_COLOR_GREEN ""
#endif
#ifndef LOG_COLOR_BROWN
#define LOG_COLOR_BROWN ""
#endif
#ifndef LOG_COLOR_BLUE
#define LOG_COLOR_BLUE ""
#endif
#ifndef LOG_COLOR_PURPLE
#define LOG_COLOR_PURPLE ""
#endif
#ifndef LOG_COLOR_CYAN
#define LOG_COLOR_CYAN ""
#endif
#ifndef CONFIG_LOG_COLORS
#define CONFIG_LOG_COLORS 0
#endif
#endif
