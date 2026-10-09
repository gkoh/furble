#include "FurbleIntervalTimer.h"

namespace Furble {

bool IntervalTimer::duration(const SpinValue::nvs_t &value, uint32_t &milliseconds) {
  uint32_t multiplier;
  switch (value.unit) {
    case SpinValue::UNIT_MS:
      multiplier = 1;
      break;
    case SpinValue::UNIT_SEC:
      multiplier = 1000;
      break;
    case SpinValue::UNIT_MIN:
      multiplier = 60000;
      break;
    default:
      return false;
  }
  milliseconds = static_cast<uint32_t>(value.value) * multiplier;
  return true;
}

bool IntervalTimer::start(const interval_t &interval, uint32_t now) {
  uint32_t wait, shutter, delay;
  if (isRunning() || !duration(interval.wait, wait) || !duration(interval.shutter, shutter)
      || !duration(interval.delay, delay)) {
    return false;
  }
  const bool infinite = interval.count.unit == SpinValue::UNIT_INF;
  if (!infinite && (interval.count.unit != SpinValue::UNIT_NIL || interval.count.value == 0)) {
    return false;
  }

  m_Shutter = shutter;
  m_Delay = delay;
  m_Count = interval.count.value;
  m_Infinite = infinite;
  m_Completed = 0;
  enter(State::WAIT, now, wait);
  return true;
}

void IntervalTimer::enter(State state, uint32_t now, uint32_t duration) {
  m_State = state;
  m_PhaseStart = now;
  m_PhaseDuration = duration;
}

IntervalTimer::Action IntervalTimer::update(uint32_t now, bool connected) {
  if (!connected) {
    return cancel();
  }
  if (!isRunning() || remaining(now) != 0) {
    return Action::NONE;
  }

  switch (m_State) {
    case State::WAIT:
      enter(State::SHUTTER_OPEN, now, m_Shutter);
      return Action::SHUTTER_PRESS;
    case State::SHUTTER_OPEN:
      if (m_Completed != UINT32_MAX) {
        ++m_Completed;
      }
      enter(State::DELAY, now, m_Delay);
      return Action::SHUTTER_RELEASE;
    case State::DELAY:
      if (!m_Infinite && m_Completed >= m_Count) {
        enter(State::FINISHED, now, 0);
        return Action::NONE;
      }
      enter(State::SHUTTER_OPEN, now, m_Shutter);
      return Action::SHUTTER_PRESS;
    case State::IDLE:
    case State::FINISHED:
      return Action::NONE;
  }
  return Action::NONE;
}

IntervalTimer::Action IntervalTimer::cancel(void) {
  const bool held = m_State == State::SHUTTER_OPEN;
  m_State = State::IDLE;
  m_PhaseDuration = 0;
  return held ? Action::SHUTTER_RELEASE : Action::NONE;
}

IntervalTimer::State IntervalTimer::getState(void) const {
  return m_State;
}

bool IntervalTimer::isRunning(void) const {
  return m_State == State::WAIT || m_State == State::SHUTTER_OPEN || m_State == State::DELAY;
}

uint32_t IntervalTimer::getCompletedCount(void) const {
  return m_Completed;
}

uint32_t IntervalTimer::remaining(uint32_t now) const {
  if (!isRunning()) {
    return 0;
  }
  const uint32_t elapsed = now - m_PhaseStart;
  return elapsed >= m_PhaseDuration ? 0 : m_PhaseDuration - elapsed;
}

}  // namespace Furble
