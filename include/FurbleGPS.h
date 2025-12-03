#ifndef FURBLE_GPS_H
#define FURBLE_GPS_H

#include <driver/uart.h>

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
  bool isEnabled(void) const;
  void reloadSetting(void);
  void startService(void);

  TinyGPSPlus &get(void);

  void reset(void);
  void task(void);

 private:
  GPS() {};

  static constexpr const size_t BUFFER_SIZE = 256;
  static constexpr const int QUEUE_SIZE = 32;

#if FURBLE_GROVE_CORE
  static constexpr const int8_t RX = 22;
  static constexpr const int8_t TX = 21;
#else
  static constexpr const int8_t RX = 33;
  static constexpr const int8_t TX = 32;
  //  RX=33, TX=32 for Module GPS v2.1, currently unsupported
#endif
  static constexpr const uint16_t SERVICE_MS = 1000;
  static constexpr const uint32_t MAX_AGE_MS = 30 * 1000;

  void enable(void);
  void disable(void);
  void serviceSerial(void);
  void update(void);

  uart_port_t m_UART = UART_NUM_2;

  lv_obj_t *m_Icon = NULL;
  lv_timer_t *m_Timer = NULL;

  TaskHandle_t m_Task = NULL;
  QueueHandle_t m_Queue = NULL;

  bool m_Enabled = false;
  bool m_HasFix = false;
  TinyGPSPlus m_GPS;
};
}  // namespace Furble

extern "C" {
void gps_task(void *param);
}

#endif
