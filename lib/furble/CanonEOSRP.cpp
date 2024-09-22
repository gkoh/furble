#include <NimBLEAddress.h>
#include <NimBLEAdvertisedDevice.h>
#include <NimBLEDevice.h>
#include <NimBLERemoteCharacteristic.h>
#include <NimBLERemoteService.h>

#include "CanonEOSRP.h"

namespace Furble {

const size_t CANON_EOS_RP_ADV_DATA_LEN = 8;
const uint8_t CANON_EOS_RP_ID_0 = 0xa9;
const uint8_t CANON_EOS_RP_ID_1 = 0x01;
const uint8_t CANON_EOS_RP_XX_2 = 0x01;
const uint8_t CANON_EOS_RP_XX_3 = 0xe2;
const uint8_t CANON_EOS_RP_XX_4 = 0x32;
const uint8_t CANON_EOS_RP_XX_5 = 0x00;
const uint8_t CANON_EOS_RP_XX_6 = 0x00;
const uint8_t CANON_EOS_RP_XX_7 = 0x02;

bool CanonEOSRP::matches(const NimBLEAdvertisedDevice *pDevice) {
  if (pDevice->haveManufacturerData()
      && pDevice->getManufacturerData().length() == CANON_EOS_RP_ADV_DATA_LEN) {
    const char *data = pDevice->getManufacturerData().data();
    if (data[0] == CANON_EOS_RP_ID_0 && data[1] == CANON_EOS_RP_ID_1 && data[2] == CANON_EOS_RP_XX_2
        && data[3] == CANON_EOS_RP_XX_3 && data[4] == CANON_EOS_RP_XX_4
        && data[5] == CANON_EOS_RP_XX_5 && data[6] == CANON_EOS_RP_XX_6
        && data[7] == CANON_EOS_RP_XX_7) {
      return true;
    }
  }
  return false;
}

}  // namespace Furble
