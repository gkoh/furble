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
bool CanonEOSRemote::connect(NimBLEClient *pClient, ezProgressBar &progress_bar) {
  m_Client = pClient;

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

  return true;
}

void CanonEOSRemote::shutterPress(void) {
  // do nothing
}

void CanonEOSRemote::shutterRelease(void) {
  // do nothing
}

void CanonEOSRemote::focusPress(void) {
  // do nothing
}

void CanonEOSRemote::focusRelease(void) {
  // do nothing
}

void CanonEOSRemote::disconnect(void) {
  m_Client->disconnect();
}

}  // namespace Furble
