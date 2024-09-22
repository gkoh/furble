#include <NimBLEAddress.h>
#include <NimBLEAdvertisedDevice.h>
#include <NimBLEDevice.h>
#include <NimBLERemoteCharacteristic.h>
#include <NimBLERemoteService.h>

#include "CanonEOSM6.h"

namespace Furble {

const size_t CANON_EOS_M6_ADV_DATA_LEN = 21;
const uint8_t CANON_EOS_M6_ID_0 = 0xa9;
const uint8_t CANON_EOS_M6_ID_1 = 0x01;
const uint8_t CANON_EOS_M6_XX_2 = 0x01;
const uint8_t CANON_EOS_M6_XX_3 = 0xc5;
const uint8_t CANON_EOS_M6_XX_4 = 0x32;

bool CanonEOSM6::matches(const NimBLEAdvertisedDevice *pDevice) {
  if (pDevice->haveManufacturerData()
      && pDevice->getManufacturerData().length() == CANON_EOS_M6_ADV_DATA_LEN) {
    const char *data = pDevice->getManufacturerData().data();
    if (data[0] == CANON_EOS_M6_ID_0 && data[1] == CANON_EOS_M6_ID_1 && data[2] == CANON_EOS_M6_XX_2
        && data[3] == CANON_EOS_M6_XX_3 && data[4] == CANON_EOS_M6_XX_4) {
      // All remaining bits should be zero.
      uint8_t zero = 0;
      for (size_t i = 5; i < CANON_EOS_M6_ADV_DATA_LEN; i++) {
        zero |= data[i];
      }
      return (zero == 0);
    }
  }
  return false;
}

}  // namespace Furble
