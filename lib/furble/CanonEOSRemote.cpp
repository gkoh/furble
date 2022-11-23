#include <NimBLEAddress.h>
#include <NimBLEAdvertisedDevice.h>
#include <NimBLEDevice.h>
#include <NimBLERemoteCharacteristic.h>
#include <NimBLERemoteService.h>

#include "CanonEOSRemote.h"

namespace Furble {

/**
 * Connect to a Canon EOS.
 *
 * The EOS uses the 'just works' BLE bonding to pair, all bond management is
 * handled by the underlying NimBLE and ESP32 libraries.
 */
bool CanonEOSRemote::connect(ezProgressBar &progress_bar) {
  Serial.println("Connecting");
  if (!m_Client->connect(m_Address)) {
    Serial.println("Connection failed!!!");
    return false;
  }

  Serial.println("Connected");
  progress_bar.value(10.0f);

  Serial.println("Securing");
  if (!m_Client->secureConnection()) {
    return false;
  }
  Serial.println("Secured!");
  progress_bar.value(20.0f);

  Serial.println("Identifying 1!");
  if (!write_prefix(CANON_EOS_SVC_REMOTE_UUID, CANON_EOS_CHR_REMOTE_PAIR_UUID, 0x03,
                    (uint8_t *)FURBLE_STR, strlen(FURBLE_STR))) {
    return false;
  }

  progress_bar.value(100.0f);

  return true;
}

void CanonEOSRemote::shutterPress(void) {
  uint8_t cmd = (CANON_EOS_REMOTE_MODE_IMMEDIATE | CANON_EOS_REMOTE_BUTTON_RELEASE);
  write_value(CANON_EOS_SVC_REMOTE_UUID, CANON_EOS_CHR_REMOTE_SHUTTER_UUID, &cmd, sizeof(cmd));
}

void CanonEOSRemote::shutterRelease(void) {
  uint8_t cmd = CANON_EOS_REMOTE_MODE_IMMEDIATE;
  write_value(CANON_EOS_SVC_REMOTE_UUID, CANON_EOS_CHR_REMOTE_SHUTTER_UUID, &cmd, sizeof(cmd));
}

void CanonEOSRemote::focusPress(void) {
  uint8_t cmd = (CANON_EOS_REMOTE_MODE_IMMEDIATE | CANON_EOS_REMOTE_BUTTON_FOCUS);
  write_value(CANON_EOS_SVC_REMOTE_UUID, CANON_EOS_CHR_REMOTE_SHUTTER_UUID, &cmd, sizeof(cmd));
}

void CanonEOSRemote::focusRelease(void) {
  uint8_t cmd = CANON_EOS_REMOTE_MODE_IMMEDIATE;
  write_value(CANON_EOS_SVC_REMOTE_UUID, CANON_EOS_CHR_REMOTE_SHUTTER_UUID, &cmd, sizeof(cmd));
}

void CanonEOSRemote::disconnect(void) {
  m_Client->disconnect();
}

}  // namespace Furble
