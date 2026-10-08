#ifndef FURBLE_INTERVALOMETER_H
#define FURBLE_INTERVALOMETER_H

#include <cstdint>

#include "interval.h"

namespace Furble {

/**
 * Interval timing independent of the display and camera transport.
 *
 * Call update from a regularly scheduled task and forward every returned action
 * to the camera controller. Each call emits at most one action. Late updates
 * start the next phase at the actual update time, avoiding catch-up captures.
 * Times are monotonic milliseconds modulo UINT32_MAX + 1; updates must not be
 * separated by a full clock period. Zero-duration phases need another update.
 */
class Intervalometer {
 public:
  enum class Action { NONE, SHUTTER_PRESS, SHUTTER_RELEASE };
  enum class State { IDLE, WAIT, SHUTTER_OPEN, DELAY, FINISHED };

  /**
   * Start with existing persisted interval settings.
   * Reject invalid units, zero finite count, or an already running interval.
   * A rejected start leaves the current state unchanged.
   */
  bool start(const interval_t &interval, uint32_t now);

  /** Advance timing, or cancel immediately when connections are lost. */
  Action update(uint32_t now, bool connected = true);

  /**
   * Stop, returning SHUTTER_RELEASE when an exposure is held open.
   * The caller must deliver the action or handle a failed/unavailable transport.
   */
  Action cancel(void);

  State getState(void) const;
  bool isRunning(void) const;

  /** Number of completed press/release pairs, saturating at UINT32_MAX. */
  uint32_t getCompletedCount(void) const;

  /** Time until the current phase ends, zero when inactive or already due. */
  uint32_t remaining(uint32_t now) const;

 private:
  static bool duration(const SpinValue::nvs_t &value, uint32_t &milliseconds);
  void enter(State state, uint32_t now, uint32_t duration);

  State m_State = State::IDLE;
  uint32_t m_PhaseStart = 0;
  uint32_t m_PhaseDuration = 0;
  uint32_t m_Shutter = 0;
  uint32_t m_Delay = 0;
  uint32_t m_Count = 0;
  uint32_t m_Completed = 0;
  bool m_Infinite = false;
};

}  // namespace Furble

#endif
