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
  getInstance().reloadSetting();
}

void GPS::setIcon(lv_obj_t *icon) {
  m_Icon = icon;
}

/** Refresh the setting from NVS. */
void GPS::reloadSetting(void) {
  m_Enabled = Settings::load<bool>(Settings::GPS);
  if (m_Enabled) {
    const int baud = Settings::load<uint32_t>(Settings::GPS_BAUD);
    const uart_config_t uart_config = {
      .baud_rate = baud,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS,
      .rx_flow_ctrl_thresh = 122,
    };
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_2, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_2, TX, RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
#if 0
    m_SerialPort.begin(baud, SERIAL_8N1, RX, TX);
#endif
  }
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
  static uint8_t count = 0;

  if (!m_Enabled || !m_HasFix) {
    return;
  }

  if (m_GPS.location.isUpdated() && m_GPS.location.isValid() && m_GPS.date.isUpdated()
      && m_GPS.date.isValid() && m_GPS.time.isValid() && m_GPS.time.isValid()) {
    Camera::gps_t dgps = {
        m_GPS.location.lat(),
        m_GPS.location.lng(),
        m_GPS.altitude.meters(),
        m_GPS.satellites.value(),
    };
    Camera::timesync_t timesync = {
        m_GPS.date.year(),   m_GPS.date.month(),  m_GPS.date.day(),         m_GPS.time.hour(),
        m_GPS.time.minute(), m_GPS.time.second(), m_GPS.time.centisecond(),
    };

    // only update every 1s
    if (count++ > (1000 / SERVICE_MS)) {
      count = 0;
      Control::getInstance().updateGPS(dgps, timesync);
    }
  }
}

/** Read and decode the GPS data from serial port. */
void GPS::serviceSerial(void) {
  static std::array<uint8_t, BUFFER_SIZE> buffer;
  static uint8_t lostFix = 0;

  if (!m_Enabled) {
    return;
  }

#if 0
  size_t available = m_SerialPort.available();
  if (available > 0) {
    size_t bytes = m_SerialPort.readBytes(buffer.data(), std::min(buffer.size(), available));

    for (size_t i = 0; i < bytes; i++) {
      m_GPS.encode(buffer[i]);
    }
  }
#endif

  if ((m_GPS.location.age() < MAX_AGE_MS) && m_GPS.location.isValid()
      && (m_GPS.date.age() < MAX_AGE_MS) && m_GPS.date.isValid() && (m_GPS.time.age() < MAX_AGE_MS)
      && m_GPS.time.age()) {
    m_HasFix = true;
    lostFix = 0;
  } else {
    lostFix++;
  }

  if (lostFix > 10) {
    // only lose fix after 10 straight losses
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
