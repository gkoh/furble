#include <M5Unified.h>
#include <TinyGPS++.h>
#include <lvgl.h>

#include "FurbleControl.h"
#include "FurbleGPS.h"
#include "FurbleSettings.h"

void gps_task(void *param) {
  Furble::GPS *gps = static_cast<Furble::GPS *>(param);
  gps->task();
}

namespace Furble {

GPS &GPS::getInstance() {
  static GPS instance;

  if (instance.m_Task == NULL) {
    const int baud = Settings::load<uint32_t>(Settings::GPS_BAUD);
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
    uart_driver_install(instance.m_UART, BUFFER_SIZE, 0, QUEUE_SIZE, &instance.m_Queue,
                        ESP_INTR_FLAG_IRAM);
    uart_param_config(instance.m_UART, &uart_config);
    uart_set_pin(instance.m_UART, TX, RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_enable_pattern_det_baud_intr(instance.m_UART, '\n', 1, 9, 0, 0);
    uart_pattern_queue_reset(instance.m_UART, QUEUE_SIZE);
    uart_flush(instance.m_UART);

    BaseType_t err = xTaskCreate(gps_task, LOG_TAG, 4096, &instance, 3, &instance.m_Task);
    if (err != pdTRUE) {
      ESP_LOGE(LOG_TAG, "Failed to create gps task.");
      abort();
    }
  }

  return instance;
}

void GPS::init(void) {
  getInstance().reloadSetting();
}

void GPS::setIcon(lv_obj_t *icon) {
  m_Icon = icon;
}

void GPS::reset(void) {
  uart_flush(m_UART);
  xQueueReset(m_Queue);
}

void GPS::task(void) {
  while (true) {
    uart_event_t event;
    if (!m_Enabled) {
      vTaskDelay(pdMS_TO_TICKS(100));
      continue;
    } else if (xQueueReceive(m_Queue, &event, pdMS_TO_TICKS(100))) {
      switch (event.type) {
        case UART_DATA:
          break;
        case UART_FIFO_OVF:
          ESP_LOGW(LOG_TAG, "GPS HW FIFO overflow");
          reset();
          break;
        case UART_BUFFER_FULL:
          ESP_LOGW(LOG_TAG, "GPS ring buffer full");
          reset();
          break;
        case UART_BREAK:
          ESP_LOGW(LOG_TAG, "GPS rx break");
          break;
        case UART_PARITY_ERR:
          ESP_LOGE(LOG_TAG, "GPS parity error");
          break;
        case UART_FRAME_ERR:
          ESP_LOGE(LOG_TAG, "GPS frame error");
          break;
        case UART_PATTERN_DET:
          serviceSerial();
          break;
        default:
          ESP_LOGW(LOG_TAG, "unknown uart event type: %d", event.type);
          break;
      }
    }
  }
}

void GPS::enable(void) {
  const uint32_t baud = Settings::load<uint32_t>(Settings::GPS_BAUD);

  uart_set_baudrate(m_UART, baud);

  // power on
  M5.Power.setExtOutput(true, m5::ext_PA);
}

void GPS::disable(void) {
  // power off
  M5.Power.setExtOutput(false, m5::ext_PA);
}

/** Refresh the setting from NVS. */
void GPS::reloadSetting(void) {
  m_Enabled = Settings::load<bool>(Settings::GPS);
  if (m_Enabled) {
    enable();
  } else {
    disable();
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
        gps->update();
      },
      SERVICE_MS, this);
}

/** Send GPS data updates to the control task. */
void GPS::update(void) {
  if (!m_Enabled) {
    return;
  }

  if ((m_GPS.location.age() < MAX_AGE_MS) && m_GPS.location.isValid()
      && (m_GPS.date.age() < MAX_AGE_MS) && m_GPS.date.isValid() && (m_GPS.time.age() < MAX_AGE_MS)
      && m_GPS.time.isValid()) {
    m_HasFix = (m_GPS.location.FixQuality() != TinyGPSLocation::Quality::Invalid);
  } else {
    m_HasFix = false;
  }

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

    Control::getInstance().updateGPS(dgps, timesync);
  }

  if (m_Icon != NULL) {
    lv_image_set_src(m_Icon, m_HasFix ? LV_SYMBOL_GPS : LV_SYMBOL_WARNING);
  }
}

/** Read and decode the GPS data from serial port. */
void GPS::serviceSerial(void) {
  std::array<uint8_t, BUFFER_SIZE> buffer;

  if (!m_Enabled) {
    return;
  }

  int bytes = uart_read_bytes(m_UART, buffer.data(), buffer.size(), 1);
  if (bytes > 0) {
    m_GPS.encode(reinterpret_cast<char *>(buffer.data()), bytes);
  }
}

TinyGPSPlus &GPS::get(void) {
  return m_GPS;
}

}  // namespace Furble
