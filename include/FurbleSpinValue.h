#ifndef FURBLE_SPINVALUE_H
#define FURBLE_SPINVALUE_H

#include <array>
#include <cstdint>

namespace Furble {
class SpinValue {
 public:
  /**
   * Enumeration of unit types.
   *
   * Value is serialised to non-volatile storage, do not change ordering.
   */
  typedef enum {
    UNIT_NIL = 0,  // no units
    UNIT_INF = 1,  // ignore value, assume infinity
    UNIT_MS = 2,   // milliseconds
    UNIT_SEC = 3,  // seconds
    UNIT_MIN = 4,  // minutes
  } unit_t;

  typedef struct {
    uint32_t hours;
    uint32_t minutes;
    uint32_t seconds;
  } hms_t;

  typedef struct __attribute__((packed)) {
    uint16_t value;
    unit_t unit;
  } nvs_t;

  SpinValue(nvs_t &nvs);

  /** Convert to packed format suitable for non-volatile storage. */
  nvs_t toNVS(void);

  /** Convert SpinValue to milliseconds. */
  uint32_t toMilliseconds(void);

  /** Convert unit enumeration to string. */
  const char *getUnitString(void);

  /** Convert milliseconds to hours:minutes:seconds. */
  static hms_t toHMS(uint32_t ms);

  uint16_t m_Value;
  unit_t m_Unit;

 private:
  // Relies on the unit_t enumeration order
  static constexpr std::array<const char *, 5> m_UnitMap = {"NIL", "INF", "msec", "secs", "mins"};
};
}  // namespace Furble

#endif
