#ifndef FURBLE_X3_KEYS_H
#define FURBLE_X3_KEYS_H
#include <cstdint>

namespace Furble {
// Bit positions match X3App::Key. Each resistor ladder reports one key only.
inline uint8_t x3DecodeKeys(int bank1, int bank2, bool power) {
  uint8_t state = power ? 64 : 0;
  if (bank1 >= 0 && bank1 <= 3900) {
    state |= bank1 > 3100 ? 32 : bank1 > 2090 ? 16 : bank1 > 750 ? 4 : 8;
  }
  if (bank2 >= 0 && bank2 <= 3900)
    state |= bank2 > 1120 ? 1 : 2;
  return state;
}
class X3KeyDebouncer {
 public:
  uint8_t update(uint8_t sample, uint32_t now) {
    // The button which wakes the board cannot trigger a power-off action.
    if (!m_Armed) {
      if (sample & 64)
        m_PowerReleased = false;
      else if (!m_PowerReleased) {
        m_PowerReleased = true;
        m_ReleaseStart = now;
      } else if (now - m_ReleaseStart >= 25)
        m_Armed = true;
      sample &= ~64;
    }
    for (unsigned i = 0; i < 7; ++i) {
      const uint8_t bit = 1u << i;
      if ((sample ^ m_Candidate) & bit) {
        m_Candidate ^= bit;
        m_Since[i] = now;
      } else if (now - m_Since[i] >= 25) {
        m_Stable = (m_Stable & ~bit) | (m_Candidate & bit);
      }
    }
    return m_Stable;
  }

 private:
  uint32_t m_Since[7] = {};
  uint32_t m_ReleaseStart = 0;
  uint8_t m_Candidate = 0, m_Stable = 0;
  bool m_Armed = false;
  bool m_PowerReleased = false;
};
// Three consecutive failed samples force release through the same debouncer.
// A transient read failure alone cannot cancel a held shutter.
class X3KeyReadGuard {
 public:
  bool sample(bool valid, uint8_t raw, uint8_t &out) {
    if (valid) {
      m_Failures = 0;
      out = raw;
      return true;
    }
    if (m_Failures < 3)
      ++m_Failures;
    // Power is a separate digital GPIO and remains reliable during ADC faults.
    out = raw & 64;
    return failed();
  }
  bool failed() const { return m_Failures >= 3; }

 private:
  uint8_t m_Failures = 0;
};
}  // namespace Furble

#endif
