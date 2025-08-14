#ifndef SETTINGS_H
#define SETTINGS_H

#include <unordered_map>

#include "Preferences.h"

#include "interval.h"

namespace Furble {
class Settings {
 public:
  Settings() = delete;
  ~Settings() = delete;

  typedef enum {
    BRIGHTNESS,
    INACTIVITY,
    THEME,
    TX_POWER,
    GPS,
    GPS_BAUD,
    INTERVAL,
    MULTICONNECT,
    RECONNECT,
    FAUXNY,
  } type_t;

  typedef struct {
    type_t type;
    const char *name;
    const char *key;
    const char *nvs_namespace;
  } setting_t;

  static const uint32_t BAUD_9600 = 9600;
  static const uint32_t BAUD_115200 = 115200;

  static void init(void);

  static const setting_t &get(type_t);

  template <typename T>
  static T load(type_t type);

  template <typename T>
  static void save(const type_t type, const T &value);

 private:
  static const std::unordered_map<type_t, setting_t> m_Setting;
  static Preferences m_Prefs;
};
}  // namespace Furble

#endif
