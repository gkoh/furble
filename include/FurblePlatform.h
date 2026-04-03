#ifndef FURBLE_PLATFORM_H
#define FURBLE_PLATFORM_H

#include <M5PM1.h>

namespace Furble {
class Platform {
 public:
  static Platform &getInstance();

  Platform(Platform const &) = delete;
  Platform(Platform &&) = delete;
  Platform &operator=(Platform const &) = delete;
  Platform &operator=(Platform &&) = delete;

  static void init(void);

  /**
   * Get time since boot in milliseconds.
   */
  uint32_t tick(void);

  /**
   * Get debounced power button click count.
   */
  uint8_t getPWRClickCount(void);

  /**
   * Update platform specific state.
   */
  void update(void);

  /**
   * Power off the device.
   */
  void powerOff(void);

  /**
   * Enable or disable automatic and light sleep.
   */
  void setSleep(bool enable);

 private:
  Platform() {};

  const int CPU_MAX_FREQ_MHZ = 160;
  const int CPU_MIN_FREQ_MHZ = 40;

  // Power button click streak threshold
  const uint8_t PWR_CLICK_THRESHOLD_MS = 20;

  M5PM1 m_M5PM1;

  bool m_Init = false;
  bool m_PMICHack = false;
  uint8_t m_PMICClickCount = 0;
  uint32_t m_PMICClickTime = 0;
};
}  // namespace Furble

#endif
