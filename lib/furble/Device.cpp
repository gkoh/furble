#include <Esp.h>

#include "Device.h"
#include "FurbleTypes.h"

namespace Furble {

Device::uuid128_t Device::g_Uuid;
char Device::g_StringID[DEVICE_ID_STR_MAX];

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

void Device::init(void) {
  uint32_t chip_id = (uint32_t)ESP.getEfuseMac();
  for (size_t i = 0; i < UUID128_AS_32_LEN; i++) {
    chip_id = xorshift(chip_id);
    g_Uuid.uint32[i] = chip_id;
  }

  // truncate ID to 5 hex characters (arbitrary, just make it 'nice' to read)
  snprintf(g_StringID, DEVICE_ID_STR_MAX, "%s-%05x", FURBLE_STR, g_Uuid.uint32[0] & 0xFFFFF);
}

void Device::getUUID128(uuid128_t *uuid) {
  *uuid = g_Uuid;
}

const char *Device::getStringID(void) {
  return g_StringID;
}

}  // namespace Furble
