#include <NimBLEAddress.h>
#include <NimBLEAdvertisedDevice.h>
#include <NimBLEDevice.h>
#include <NimBLERemoteCharacteristic.h>
#include <NimBLERemoteService.h>

#include "CanonEOS.h"

namespace Furble {

static volatile uint8_t pair_result = 0x00;

CanonEOS::CanonEOS(const void *data, size_t len) {
  if (len != sizeof(eos_t))
    throw;

  const eos_t *eos = (eos_t *)data;
  m_Name = std::string(eos->name);
  m_Address = NimBLEAddress(eos->address, eos->type);
  memcpy(&m_Uuid, &eos->uuid, sizeof(uuid128_t));
  m_Client = NimBLEDevice::createClient();
}

CanonEOS::CanonEOS(NimBLEAdvertisedDevice *pDevice) {
  m_Name = pDevice->getName();
  m_Address = pDevice->getAddress();
  Serial.println("Name = " + String(m_Name.c_str()));
  Serial.println("Address = " + String(m_Address.toString().c_str()));
  Device::getUUID128(&m_Uuid);
  m_Client = NimBLEDevice::createClient();
}

CanonEOS::~CanonEOS(void) {
  NimBLEDevice::deleteClient(m_Client);
  m_Client = nullptr;
}

bool CanonEOS::write_value(const char *serviceUUID,
                           const char *characteristicUUID,
                           uint8_t *data,
                           size_t length) {
  NimBLERemoteService *pSvc = m_Client->getService(serviceUUID);
  if (pSvc) {
    NimBLERemoteCharacteristic *pChr = pSvc->getCharacteristic(characteristicUUID);
    return ((pChr != nullptr) && pChr->canWrite() && pChr->writeValue(data, length, true));
  }

  return false;
}

bool CanonEOS::write_prefix(const char *serviceUUID,
                            const char *characteristicUUID,
                            uint8_t prefix,
                            uint8_t *data,
                            size_t length) {
  uint8_t buffer[length + 1] = {0};
  buffer[0] = prefix;
  memcpy(&buffer[1], data, length);
  return write_value(serviceUUID, characteristicUUID, buffer, length + 1);
}

size_t CanonEOS::getSerialisedBytes(void) {
  return sizeof(eos_t);
}

bool CanonEOS::serialise(void *buffer, size_t bytes) {
  if (bytes != sizeof(eos_t)) {
    return false;
  }
  eos_t *x = (eos_t *)buffer;
  strncpy(x->name, m_Name.c_str(), MAX_NAME);
  x->address = (uint64_t)m_Address;
  x->type = m_Address.getType();
  memcpy(&x->uuid, &m_Uuid, sizeof(uuid128_t));

  return true;
}

}  // namespace Furble
