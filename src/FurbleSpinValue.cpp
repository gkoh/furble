#include "FurbleSpinValue.h"

namespace Furble {
constexpr std::array<const char *, 5> SpinValue::m_UnitMap;

SpinValue::SpinValue(nvs_t &nvs) : m_Value(nvs.value), m_Unit(nvs.unit){};

SpinValue::nvs_t SpinValue::toNVS(void) {
  return (nvs_t){m_Value, m_Unit};
}

uint32_t SpinValue::toMilliseconds(void) {
  switch (m_Unit) {
    case UNIT_MIN:
      return (m_Value * 60 * 1000);
    case UNIT_SEC:
      return (m_Value * 1000);
    case UNIT_MS:
      return (m_Value);
    default:
      return 0;
  }
  return 0;
}

const char *SpinValue::getUnitString(void) {
  return m_UnitMap[m_Unit];
}

SpinValue::hms_t SpinValue::toHMS(uint32_t ms) {
  return (hms_t){(ms / 1000) / (60 * 60), ((ms / 1000) / 60) % 60, (ms / 1000) % 60};
}
}  // namespace Furble
