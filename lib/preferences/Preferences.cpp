// Copyright 2015-2021 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <cstring>

#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "Preferences.h"

namespace Furble {

static const char *LOG_TAG = "Preferences";

const char *nvs_errors[] = {"OTHER",         "NOT_INITIALIZED",  "NOT_FOUND",    "TYPE_MISMATCH",
                            "READ_ONLY",     "NOT_ENOUGH_SPACE", "INVALID_NAME", "INVALID_HANDLE",
                            "REMOVE_FAILED", "KEY_TOO_LONG",     "PAGE_FULL",    "INVALID_STATE",
                            "INVALID_LENGTH"};
#define nvs_error(e) \
  (((e) > ESP_ERR_NVS_BASE) ? nvs_errors[(e) & ~(ESP_ERR_NVS_BASE)] : nvs_errors[0])

Preferences::Preferences() : _handle(0), _started(false), _readOnly(false) {}

Preferences::~Preferences() {
  end();
}

bool Preferences::begin(const char *name, bool readOnly, const char *partition_label) {
  if (_started) {
    return false;
  }
  _readOnly = readOnly;
  esp_err_t err = ESP_OK;
  if (partition_label != NULL) {
    err = nvs_flash_init_partition(partition_label);
    if (err) {
      ESP_LOGE(LOG_TAG, "nvs_flash_init_partition failed: %s", nvs_error(err));
      return false;
    }
    err = nvs_open_from_partition(partition_label, name, readOnly ? NVS_READONLY : NVS_READWRITE,
                                  &_handle);
  } else {
    err = nvs_open(name, readOnly ? NVS_READONLY : NVS_READWRITE, &_handle);
  }
  if (err) {
    ESP_LOGE(LOG_TAG, "nvs_open failed: %s", nvs_error(err));
    return false;
  }
  _started = true;
  return true;
}

void Preferences::end() {
  if (!_started) {
    return;
  }
  nvs_close(_handle);
  _started = false;
}

/*
 * Clear all keys in opened preferences
 * */

bool Preferences::clear() {
  if (!_started || _readOnly) {
    return false;
  }
  esp_err_t err = nvs_erase_all(_handle);
  if (err) {
    ESP_LOGE(LOG_TAG, "nvs_erase_all fail: %s", nvs_error(err));
    return false;
  }
  err = nvs_commit(_handle);
  if (err) {
    ESP_LOGE(LOG_TAG, "nvs_commit fail: %s", nvs_error(err));
    return false;
  }
  return true;
}

/*
 * Remove a key
 * */

bool Preferences::remove(const char *key) {
  if (!_started || !key || _readOnly) {
    return false;
  }
  esp_err_t err = nvs_erase_key(_handle, key);
  if (err) {
    ESP_LOGE(LOG_TAG, "nvs_erase_key fail: %s %s", key, nvs_error(err));
    return false;
  }
  err = nvs_commit(_handle);
  if (err) {
    ESP_LOGE(LOG_TAG, "nvs_commit fail: %s %s", key, nvs_error(err));
    return false;
  }
  return true;
}

template <>
size_t Preferences::put(const char *key, uint8_t value) {
  if (!_started || !key || _readOnly) {
    return 0;
  }
  esp_err_t err = nvs_set_u8(_handle, key, value);
  if (err) {
    ESP_LOGE(LOG_TAG, "nvs_set_u8 fail: %s %s", key, nvs_error(err));
    return 0;
  }
  err = nvs_commit(_handle);
  if (err) {
    ESP_LOGE(LOG_TAG, "nvs_commit fail: %s %s", key, nvs_error(err));
    return 0;
  }
  return 1;
}

template <>
size_t Preferences::put(const char *key, uint32_t value) {
  if (!_started || !key || _readOnly) {
    return 0;
  }
  esp_err_t err = nvs_set_u32(_handle, key, value);
  if (err) {
    ESP_LOGE(LOG_TAG, "nvs_set_u32 fail: %s %s", key, nvs_error(err));
    return 0;
  }
  err = nvs_commit(_handle);
  if (err) {
    ESP_LOGE(LOG_TAG, "nvs_commit fail: %s %s", key, nvs_error(err));
    return 0;
  }
  return 4;
}

template <>
size_t Preferences::put(const char *key, const bool value) {
  return put<uint8_t>(key, (uint8_t)(value ? 1 : 0));
}

size_t Preferences::put(const char *key, const char *value) {
  if (!_started || !key || !value || _readOnly) {
    return 0;
  }
  esp_err_t err = nvs_set_str(_handle, key, value);
  if (err) {
    ESP_LOGE(LOG_TAG, "nvs_set_str fail: %s %s", key, nvs_error(err));
    return 0;
  }
  err = nvs_commit(_handle);
  if (err) {
    ESP_LOGE(LOG_TAG, "nvs_commit fail: %s %s", key, nvs_error(err));
    return 0;
  }
  return strlen(value);
}

template <>
size_t Preferences::put(const char *key, const std::string value) {
  return put(key, value.c_str());
}

size_t Preferences::put(const char *key, const void *value, size_t len) {
  if (!_started || !key || !value || !len || _readOnly) {
    return 0;
  }
  esp_err_t err = nvs_set_blob(_handle, key, value, len);
  if (err) {
    ESP_LOGE(LOG_TAG, "nvs_set_blob fail: %s %s", key, nvs_error(err));
    return 0;
  }
  err = nvs_commit(_handle);
  if (err) {
    ESP_LOGE(LOG_TAG, "nvs_commit fail: %s %s", key, nvs_error(err));
    return 0;
  }
  return len;
}

Preferences::PreferenceType Preferences::getType(const char *key) {
  if (!_started || !key || strlen(key) > 15) {
    return PT_INVALID;
  }
  int8_t mt1;
  uint8_t mt2;
  int16_t mt3;
  uint16_t mt4;
  int32_t mt5;
  uint32_t mt6;
  int64_t mt7;
  uint64_t mt8;
  size_t len = 0;
  if (nvs_get_i8(_handle, key, &mt1) == ESP_OK) {
    return PT_I8;
  }
  if (nvs_get_u8(_handle, key, &mt2) == ESP_OK) {
    return PT_U8;
  }
  if (nvs_get_i16(_handle, key, &mt3) == ESP_OK) {
    return PT_I16;
  }
  if (nvs_get_u16(_handle, key, &mt4) == ESP_OK) {
    return PT_U16;
  }
  if (nvs_get_i32(_handle, key, &mt5) == ESP_OK) {
    return PT_I32;
  }
  if (nvs_get_u32(_handle, key, &mt6) == ESP_OK) {
    return PT_U32;
  }
  if (nvs_get_i64(_handle, key, &mt7) == ESP_OK) {
    return PT_I64;
  }
  if (nvs_get_u64(_handle, key, &mt8) == ESP_OK) {
    return PT_U64;
  }
  if (nvs_get_str(_handle, key, NULL, &len) == ESP_OK) {
    return PT_STR;
  }
  if (nvs_get_blob(_handle, key, NULL, &len) == ESP_OK) {
    return PT_BLOB;
  }
  return PT_INVALID;
}

bool Preferences::isKey(const char *key) {
  return getType(key) != PT_INVALID;
}

/*
 * Get a key value
 * */

template <>
uint8_t Preferences::get(const char *key, const uint8_t defaultValue) {
  uint8_t value = defaultValue;
  if (!_started || !key) {
    return value;
  }

  esp_err_t err = nvs_get_u8(_handle, key, &value);
  if (err) {
    ESP_LOGV(LOG_TAG, "nvs_get_u8 fail: %s %s", key, nvs_error(err));
  }

  return value;
}

template <>
bool Preferences::get(const char *key, const bool defaultValue) {
  return get<uint8_t>(key, defaultValue ? 1 : 0) == 1;
}

template <>
uint32_t Preferences::get(const char *key, const uint32_t defaultValue) {
  uint32_t value = defaultValue;
  if (!_started || !key) {
    return value;
  }

  esp_err_t err = nvs_get_u32(_handle, key, &value);
  if (err) {
    ESP_LOGV(LOG_TAG, "nvs_get_u32 fail: %s %s", key, nvs_error(err));
  }

  return value;
}

template <>
std::string Preferences::get(const char *key, const std::string defaultValue) {
  return std::string();
}

std::string Preferences::get(const char *key, const std::string defaultValue) {
  char *value = NULL;
  size_t len = 0;
  if (!_started || !key) {
    return std::string(defaultValue);
  }
  esp_err_t err = nvs_get_str(_handle, key, value, &len);
  if (err) {
    ESP_LOGE(LOG_TAG, "nvs_get_str len fail: %s %s", key, nvs_error(err));
    return std::string(defaultValue);
  }
  char buf[len];
  value = buf;
  err = nvs_get_str(_handle, key, value, &len);
  if (err) {
    ESP_LOGE(LOG_TAG, "nvs_get_str fail: %s %s", key, nvs_error(err));
    return std::string(defaultValue);
  }
  return std::string(buf);
}

size_t Preferences::getBytesLength(const char *key) {
  size_t len = 0;
  if (!_started || !key) {
    return 0;
  }
  esp_err_t err = nvs_get_blob(_handle, key, NULL, &len);
  if (err) {
    ESP_LOGE(LOG_TAG, "nvs_get_blob len fail: %s %s", key, nvs_error(err));
    return 0;
  }
  return len;
}

size_t Preferences::get(const char *key, void *buf, size_t maxLen) {
  size_t len = getBytesLength(key);
  if (!len || !buf || !maxLen) {
    return len;
  }
  if (len > maxLen) {
    ESP_LOGE(LOG_TAG, "not enough space in buffer: %u < %u", maxLen, len);
    return 0;
  }
  esp_err_t err = nvs_get_blob(_handle, key, buf, &len);
  if (err) {
    ESP_LOGE(LOG_TAG, "nvs_get_blob fail: %s %s", key, nvs_error(err));
    return 0;
  }
  return len;
}

size_t Preferences::freeEntries() {
  nvs_stats_t nvs_stats;
  esp_err_t err = nvs_get_stats(NULL, &nvs_stats);
  if (err) {
    ESP_LOGE(LOG_TAG, "Failed to get nvs statistics");
    return 0;
  }
  return nvs_stats.free_entries;
}

}  // namespace Furble
