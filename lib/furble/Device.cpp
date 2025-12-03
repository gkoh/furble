#include <NimBLEDevice.h>
#include <esp_mac.h>

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

uint64_t Device::getEfuseMac(void) {
  uint64_t _chipmacid = 0LL;
  esp_efuse_mac_get_default((uint8_t *)(&_chipmacid));
  return _chipmacid;
}

void Device::init(esp_power_level_t power) {
  uint32_t chip_id = (uint32_t)getEfuseMac();
  for (size_t i = 0; i < UUID128_AS_32_LEN; i++) {
    chip_id = xorshift(chip_id);
    m_Uuid.uint32[i] = chip_id;
  }

  // truncate ID to 5 hex characters (arbitrary, just make it 'nice' to read)
  snprintf(m_StringID, DEVICE_ID_STR_MAX, "%s-%05lx", FURBLE_STR, m_Uuid.uint32[0] & 0xFFFFF);

  m_ID = std::string(m_StringID);

  // Combine address and data for scan duplicate detection
  NimBLEDevice::setScanFilterMode(CONFIG_BTDM_SCAN_DUPL_TYPE_DATA_DEVICE);

  NimBLEDevice::init(m_ID);
  NimBLEDevice::setPower(power);

  // enable bonding, mitm and secure connection
  NimBLEDevice::setSecurityAuth(true, true, true);

  // claim YESNO to enable numeric LE security
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_YESNO);

  // configure RPA (where possible) and distribute encryption key and ID key (IRK)
  NimBLEDevice::setSecurityInitKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);
  NimBLEDevice::setSecurityRespKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);
  NimBLEDevice::setOwnAddrType(BLE_OWN_ADDR_RPA_PUBLIC_DEFAULT);
}

Device::uuid128_t Device::getUUID128(void) {
  return m_Uuid;
}

const std::string Device::getStringID(void) {
  return m_ID;
}

}  // namespace Furble
