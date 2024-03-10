#include <NimBLEAdvertisedDevice.h>

#include "Camera.h"

namespace Furble {

bool Camera::connect(esp_power_level_t power, progressFunc pFunc, void *pCtx) {
  bool connected = this->connect(pFunc, pCtx);
  if (connected) {
    // Set BLE transmit power after connection is established.
    NimBLEDevice::setPower(power);
  }

  return connected;
}

const char *Camera::getName(void) {
  return m_Name.c_str();
}

void Camera::fillSaveName(char *name) {
  snprintf(name, 16, "%08llX", (uint64_t)m_Address);
}

void Camera::updateProgress(progressFunc pFunc, void *ctx, float value) {
  if (pFunc != nullptr) {
    (pFunc)(ctx, value);
  }
}

/**
 * Generate a 32-bit PRNG.
 */
static uint32_t xorshift(uint32_t x) {
  /* Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs" */
  x ^= x << 13;
  x ^= x << 17;
  x ^= x << 5;
  return x;
}

void Camera::getUUID128(uuid128_t *uuid) {
  uint32_t chip_id = (uint32_t)ESP.getEfuseMac();
  for (size_t i = 0; i < UUID128_AS_32_LEN; i++) {
    chip_id = xorshift(chip_id);
    uuid->uint32[i] = chip_id;
  }
}

bool Camera::isConnected(void) {
  return m_Client->isConnected();
}

}  // namespace Furble
