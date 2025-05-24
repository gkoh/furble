#include <Esp.h>
#include <NimBLEDevice.h>

#include "Device.h"
#include "FurbleTypes.h"

namespace Furble {

Device::uuid128_t Device::m_Uuid;
char Device::m_StringID[DEVICE_ID_STR_MAX];
std::string Device::m_ID;

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

void Device::init(esp_power_level_t power) {
  uint32_t chip_id = (uint32_t)ESP.getEfuseMac();
  for (size_t i = 0; i < UUID128_AS_32_LEN; i++) {
    chip_id = xorshift(chip_id);
    m_Uuid.uint32[i] = chip_id;
  }

  // truncate ID to 5 hex characters (arbitrary, just make it 'nice' to read)
  snprintf(m_StringID, DEVICE_ID_STR_MAX, "%s-%05x", FURBLE_STR, m_Uuid.uint32[0] & 0xFFFFF);

  m_ID = std::string(m_StringID);

  // Combine address and data for scan duplicate detection
  NimBLEDevice::setScanFilterMode(CONFIG_BTDM_SCAN_DUPL_TYPE_DATA_DEVICE);

  NimBLEDevice::init(m_ID);
  NimBLEDevice::setPower(power);
  NimBLEDevice::setSecurityAuth(true, false, true);
  // NimBLEDevice::setOwnAddrType(BLE_OWN_ADDR_PUBLIC);
}

Device::uuid128_t Device::getUUID128(void) {
  return m_Uuid;
}

const std::string Device::getStringID(void) {
  return m_ID;
}

}  // namespace Furble
