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
    TOUCH_CALIBRATION,
    AUTOCONNECT,
  } type_t;

  typedef struct {
    union {
      struct {
        uint16_t x;
        uint16_t y;
      };
      uint16_t coords[2];
    };
  } calibration_point_t;

  typedef struct {
    union {
      struct {
        calibration_point_t top_left;
        calibration_point_t bottom_left;
        calibration_point_t top_right;
        calibration_point_t bottom_right;
      };
      calibration_point_t pairs[4];
      uint16_t points[8];
    };
    bool calibrated;
  } calibration_t;

  typedef struct {
    type_t type;
    const char *name;
    const char *key;
    const char *nvs_namespace;
  } setting_t;

  static constexpr uint32_t BAUD_9600 = 9600;
  static constexpr uint32_t BAUD_115200 = 115200;

  static void init(void);

  static const setting_t &get(type_t);

  /** Bind each setting to its storage type for type-safe load/save. */
  template <type_t S>
  struct storage_type;

  /** Load a setting — type deduced from the setting. */
  template <type_t S>
  static typename storage_type<S>::type load() {
    return load<typename storage_type<S>::type>(S);
  }

  /** Save a setting — type deduced from the setting. */
  template <type_t S>
  static void save(const typename storage_type<S>::type &value) {
    save<typename storage_type<S>::type>(S, value);
  }

  template <typename T>
  static T load(type_t type);

  template <typename T>
  static void save(const type_t type, const T &value);

 private:
  template <typename T>
  static T loadValue(type_t type);
  template <typename T>
  static void saveValue(type_t type, const T &value);

  static const std::unordered_map<type_t, setting_t> m_Setting;
  static Preferences m_Prefs;
};

template <>
struct Settings::storage_type<Settings::BRIGHTNESS> {
  using type = uint8_t;
};
template <>
struct Settings::storage_type<Settings::INACTIVITY> {
  using type = uint8_t;
};
template <>
struct Settings::storage_type<Settings::THEME> {
  using type = std::string;
};
template <>
struct Settings::storage_type<Settings::TX_POWER> {
  using type = uint8_t;
};
template <>
struct Settings::storage_type<Settings::GPS> {
  using type = bool;
};
template <>
struct Settings::storage_type<Settings::GPS_BAUD> {
  using type = uint32_t;
};
template <>
struct Settings::storage_type<Settings::INTERVAL> {
  using type = interval_t;
};
template <>
struct Settings::storage_type<Settings::MULTICONNECT> {
  using type = bool;
};
template <>
struct Settings::storage_type<Settings::RECONNECT> {
  using type = bool;
};
template <>
struct Settings::storage_type<Settings::FAUXNY> {
  using type = bool;
};
template <>
struct Settings::storage_type<Settings::TOUCH_CALIBRATION> {
  using type = Settings::calibration_t;
};
template <>
struct Settings::storage_type<Settings::AUTOCONNECT> {
  using type = bool;
};

}  // namespace Furble

#endif
