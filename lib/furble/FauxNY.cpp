#include <esp_random.h>

#include "FauxNY.h"

namespace Furble {

FauxNY::FauxNY(const void *data, size_t len) : Camera(Type::FAUXNY, PairType::SAVED) {
  if (len != sizeof(fauxNY_t)) {
    abort();
  }

  const auto *fauxNY = static_cast<const fauxNY_t *>(data);
  m_Name = std::string(fauxNY->name);
  m_ID = fauxNY->id;
  m_Address = NimBLEAddress(m_ID, 0);
}

FauxNY::FauxNY(void) : Camera(Type::FAUXNY, PairType::NEW) {
  m_ID = esp_random();
  m_Name = std::string("FauxNY-") + std::to_string(m_ID % 42);
  m_Address = NimBLEAddress(m_ID, 0);
}

bool FauxNY::matches(void) {
  return true;
}

bool FauxNY::_connect(void) {
  ESP_LOGI(m_FauxNYStr, "Connecting");
  m_Progress = 0;

  for (int i = 0; i < 100; i += 1) {
    vTaskDelay(pdMS_TO_TICKS(25));
    m_Progress = i;
  }

  m_Progress = 100;

  return true;
}

void FauxNY::shutterPress(void) {
  ESP_LOGI(m_FauxNYStr, "shutterPress()");
}

void FauxNY::shutterRelease(void) {
  ESP_LOGI(m_FauxNYStr, "shutterRelease()");
}

void FauxNY::focusPress(void) {
  ESP_LOGI(m_FauxNYStr, "focusPress()");
}

void FauxNY::focusRelease(void) {
  ESP_LOGI(m_FauxNYStr, "focusRelease()");
}

void FauxNY::updateGeoData(const gps_t &gps, const timesync_t &timesync) {
  ESP_LOGI(m_FauxNYStr, "updateGeoData()");
};

void FauxNY::_disconnect(void) {
  ESP_LOGI(m_FauxNYStr, "Disconnecting");
  m_Connected = false;
}

size_t FauxNY::getSerialisedBytes(void) const {
  return sizeof(fauxNY_t);
}

bool FauxNY::serialise(void *buffer, size_t bytes) const {
  if (bytes != sizeof(fauxNY_t)) {
    return false;
  }

  auto *fauxNY = static_cast<fauxNY_t *>(buffer);
  strncpy(fauxNY->name, m_Name.c_str(), MAX_NAME);
  fauxNY->address = (uint64_t)m_Address;
  fauxNY->type = m_Address.getType();
  fauxNY->id = m_ID;

  return true;
}

}  // namespace Furble
