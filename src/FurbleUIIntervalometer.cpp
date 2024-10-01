#include <lvgl.h>

#include "FurbleUI.h"

namespace Furble {

UI::Intervalometer::Intervalometer(const interval_t &interval)
    : m_State{STATE_IDLE},
      m_Count(this, interval.count, true),
      m_Delay(this, interval.delay),
      m_Shutter(this, interval.shutter) {}

void UI::Intervalometer::save(void) {
  interval_t interval = {m_Count.m_SpinValue.toNVS(), m_Delay.m_SpinValue.toNVS(),
                         m_Shutter.m_SpinValue.toNVS()};
  Settings::save<interval_t>(Settings::INTERVAL, interval);
}

void UI::Intervalometer::Spinner::update(void) {
  if (lv_obj_has_state(m_SwitchInfinite, LV_STATE_CHECKED)) {
    m_SpinValue.m_Unit = SpinValue::UNIT_INF;
    lv_obj_add_flag(m_RowSpinners, LV_OBJ_FLAG_HIDDEN);
  } else {
    m_SpinValue.m_Unit = SpinValue::UNIT_NIL;
    lv_obj_clear_flag(m_RowSpinners, LV_OBJ_FLAG_HIDDEN);

    uint32_t h = lv_roller_get_selected(m_Roller[0]);
    uint32_t t = lv_roller_get_selected(m_Roller[1]);
    uint32_t u = lv_roller_get_selected(m_Roller[2]);

    m_SpinValue.m_Value = (h * 100) + (t * 10) + u;
  }

  if (m_RollerUnit != nullptr) {
    uint32_t u = lv_roller_get_selected(m_RollerUnit);
    switch (u) {
      case 0:
        m_SpinValue.m_Unit = SpinValue::UNIT_MS;
        break;
      case 1:
        m_SpinValue.m_Unit = SpinValue::UNIT_SEC;
        break;
      case 2:
        m_SpinValue.m_Unit = SpinValue::UNIT_MIN;
        break;
    }
  }
  updateLabels();

  m_Intervalometer->save();
}

void UI::Intervalometer::Spinner::updateLabels(void) {
  switch (m_SpinValue.m_Unit) {
    case SpinValue::UNIT_INF:
      lv_label_set_text_fmt(m_Value, "Infinite");
      break;

    case SpinValue::UNIT_NIL:
      lv_label_set_text_fmt(m_Value, "%u", m_SpinValue.m_Value);
      break;

    default:
      lv_label_set_text_fmt(m_Value, "%u %s", m_SpinValue.m_Value, m_SpinValue.getUnitString());
      break;
  }

  if (m_SpinValue.m_Unit != SpinValue::UNIT_INF) {
    // Update rollers
    uint32_t h = m_SpinValue.m_Value / 100;
    uint32_t t = (m_SpinValue.m_Value % 100) / 10;
    uint32_t u = (m_SpinValue.m_Value % 10);

    lv_roller_set_selected(m_Roller[0], h, LV_ANIM_ON);
    lv_roller_set_selected(m_Roller[1], t, LV_ANIM_ON);
    lv_roller_set_selected(m_Roller[2], u, LV_ANIM_ON);

    if (m_SpinValue.m_Unit != SpinValue::UNIT_NIL) {
      uint32_t i = 0;
      switch (m_SpinValue.m_Unit) {
        case SpinValue::UNIT_MS:
          i = 0;
          break;
        case SpinValue::UNIT_SEC:
          i = 1;
          break;
        case SpinValue::UNIT_MIN:
          i = 2;
          break;
        default:
          break;
      }

      lv_roller_set_selected(m_RollerUnit, i, LV_ANIM_ON);
    }
  }
}

}  // namespace Furble
