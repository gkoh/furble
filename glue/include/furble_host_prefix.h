#ifndef FURBLE_HOST_PREFIX_H_
#define FURBLE_HOST_PREFIX_H_

#if __has_include(<memory>)
#include <memory>
#endif

/* Newer libc++ stopped pulling these in transitively; furble lib relies on
 * them (std::abs, std::strlen, ...). */
#include <cmath>
#include <cstdlib>
#include <cstring>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#endif
