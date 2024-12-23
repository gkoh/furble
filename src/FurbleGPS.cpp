#include <M5Unified.h>
#include <TinyGPS++.h>
#include <lvgl.h>

#include "FurbleControl.h"
#include "FurbleGPS.h"
#include "FurbleSettings.h"

namespace Furble {

GPS &GPS::getInstance() {
  static GPS instance;

  return instance;
}

void GPS::init(void) {
  Serial2.begin(BAUD, SERIAL_8N1, RX, TX);

  getInstance().reloadSetting();
}

void GPS::setIcon(lv_obj_t *icon) {
  m_Icon = icon;
}

/** Refresh the setting from NVS. */
void GPS::reloadSetting(void) {
  m_Enabled = Settings::load<bool>(Settings::GPS);
}

/** Is GPS enabled? */
bool GPS::isEnabled(void) {
  return m_Enabled;
}

/** Start timer event to service/update GPS. */
void GPS::startService(void) {
  m_Timer = lv_timer_create(
      [](lv_timer_t *timer) {
        auto *gps = static_cast<GPS *>(lv_timer_get_user_data(timer));
        gps->serviceSerial();
        gps->update();
      },
      SERVICE_MS, this);
}

/** Send GPS data updates to the control task. */
void GPS::update(void) {
  if (!m_Enabled || !m_HasFix) {
    return;
  }

  if (m_GPS.location.isUpdated() && m_GPS.location.isValid() && m_GPS.date.isUpdated()
      && m_GPS.date.isValid() && m_GPS.time.isValid() && m_GPS.time.isValid()) {
    Camera::gps_t dgps = {
        m_GPS.location.lat(),
        m_GPS.location.lng(),
        m_GPS.altitude.meters(),
    };
    Camera::timesync_t timesync = {
        m_GPS.date.year(), m_GPS.date.month(),  m_GPS.date.day(),
        m_GPS.time.hour(), m_GPS.time.minute(), m_GPS.time.second(),
    };

    Control::getInstance().updateGPS(dgps, timesync);
  }
}

/** Read and decode the GPS data from serial port. */
void GPS::serviceSerial(void) {
  if (!m_Enabled) {
    return;
  }

  while (Serial2.available() > 0) {
    m_GPS.encode(Serial2.read());
  }

  if ((m_GPS.location.age() < MAX_AGE_MS) && m_GPS.location.isValid()
      && (m_GPS.date.age() < MAX_AGE_MS) && m_GPS.date.isValid() && (m_GPS.time.age() < MAX_AGE_MS)
      && m_GPS.time.age()) {
    m_HasFix = true;
  } else {
    m_HasFix = false;
  }

  if (m_Icon != NULL) {
    lv_image_set_src(m_Icon, m_HasFix ? LV_SYMBOL_GPS : LV_SYMBOL_WARNING);
  }
}

TinyGPSPlus &GPS::get(void) {
  return m_GPS;
}
}  // namespace Furble
