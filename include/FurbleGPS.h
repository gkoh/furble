#ifndef FURBLE_GPS_H
#define FURBLE_GPS_H

#include <lvgl.h>

#include <TinyGPS++.h>

namespace Furble {
class GPS {
 public:
  static GPS &getInstance();

  GPS(GPS const &) = delete;
  GPS(GPS &&) = delete;
  GPS &operator=(GPS const &) = delete;
  GPS &operator=(GPS &&) = delete;

  static void init(void);

  void setIcon(lv_obj_t *icon);
  bool isEnabled(void);
  void reloadSetting(void);
  void startService(void);
  TinyGPSPlus &get(void);

  void update(void);

 private:
  GPS(){};

  static constexpr const uint32_t BAUD = 9600;
#if FURBLE_GROVE_CORE
  static constexpr const int8_t RX = 22;
  static constexpr const int8_t TX = 21;
#else
  static constexpr const int8_t RX = 33;
  static constexpr const int8_t TX = 32;
#endif
  static constexpr const uint16_t SERVICE_MS = 250;
  static constexpr const uint32_t MAX_AGE_MS = 60 * 1000;

  void serviceSerial(void);

  lv_obj_t *m_Icon = NULL;
  lv_timer_t *m_Timer = NULL;

  bool m_Enabled = false;
  bool m_HasFix = false;
  TinyGPSPlus m_GPS;
};
}  // namespace Furble

#endif
