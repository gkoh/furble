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

/** Install UART driver. */
void GPS::installDriver(uint32_t baud) {
  const uart_config_t uart_config = {
      .baud_rate = (int)baud,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      .rx_flow_ctrl_thresh = 0,
      .source_clk = UART_SCLK_REF_TICK,
      .flags = {},
  };
  uart_driver_install(m_UART, BUFFER_SIZE, 0, 0, NULL, 0);
  uart_param_config(m_UART, &uart_config);
  uart_set_pin(m_UART, TX, RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
  uart_flush(m_UART);
}

/** Delete UART driver. */
void GPS::deleteDriver(void) {
  if (uart_is_driver_installed(m_UART)) {
    uart_driver_delete(m_UART);
  }
}

/** Refresh the setting from NVS. */
void GPS::reloadSetting(void) {
  m_Enabled = Settings::load<bool>(Settings::GPS);
  if (m_Enabled) {
    const int baud = Settings::load<uint32_t>(Settings::GPS_BAUD);
    deleteDriver();
    installDriver(baud);
  } else {
    deleteDriver();
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

#if 0
  if (m_HasFix) {
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

    // update every 1s
    if (++count > (1000 / SERVICE_MS)) {
      count = 0;
      Control::getInstance().updateGPS(dgps, timesync);
    }
  }
}

/** Read and decode the GPS data from serial port. */
void GPS::serviceSerial(void) {
  static uint8_t lostFix = 0;
  std::array<uint8_t, BUFFER_SIZE> buffer;

  if (!m_Enabled) {
    return;
  }

  int bytes = uart_read_bytes(m_UART, buffer.data(), buffer.size(), 1);
  if (bytes > 0) {
    for (size_t i = 0; i < bytes; i++) {
      m_GPS.encode(buffer[i]);
    }
  }

  if ((m_GPS.location.age() < MAX_AGE_MS) && m_GPS.location.isValid()
      && (m_GPS.date.age() < MAX_AGE_MS) && m_GPS.date.isValid() && (m_GPS.time.age() < MAX_AGE_MS)
      && m_GPS.time.isValid()) {
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
