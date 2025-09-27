#include <NimBLEAddress.h>
#include <NimBLEAdvertisedDevice.h>
#include <NimBLEDevice.h>
#include <NimBLERemoteCharacteristic.h>
#include <NimBLERemoteService.h>

#include "CanonEOS.h"
#include "Device.h"

namespace Furble {

CanonEOS::CanonEOS(Type type, const void *data, size_t len) : Camera(type, PairType::SAVED) {
  if (len != sizeof(eos_t))
    abort();

  const eos_t *eos = static_cast<const eos_t *>(data);
  m_Name = std::string(eos->name);
  m_Address = NimBLEAddress(eos->address, eos->type);
  memcpy(&m_Uuid, &eos->uuid, sizeof(Device::uuid128_t));
}

CanonEOS::CanonEOS(Type type, const NimBLEAdvertisedDevice *pDevice) : Camera(type, PairType::NEW) {
  m_Name = pDevice->getName();
  m_Address = pDevice->getAddress();
  ESP_LOGI(LOG_TAG, "Name = %s", m_Name.c_str());
  ESP_LOGI(LOG_TAG, "Address = %s", m_Address.toString().c_str());
  m_Uuid = Device::getUUID128();
}

void CanonEOS::_disconnect(void) {
  m_Client->disconnect();
}

bool CanonEOS::writePrefix(const NimBLEUUID &serviceUUID,
                           const NimBLEUUID &characteristicUUID,
                           const uint8_t prefix,
                           const void *data,
                           uint16_t length) {
  uint8_t buffer[length + 1] = {0};
  buffer[0] = prefix;
  memcpy(&buffer[1], data, length);
  return m_Client->setValue(serviceUUID, characteristicUUID, {&buffer[0], (uint16_t)(length + 1)});
}

size_t CanonEOS::getSerialisedBytes(void) const {
  return sizeof(eos_t);
}

bool CanonEOS::serialise(void *buffer, size_t bytes) const {
  if (bytes != sizeof(eos_t)) {
    return false;
  }
  eos_t *x = static_cast<eos_t *>(buffer);
  strncpy(x->name, m_Name.c_str(), MAX_NAME);
  x->address = (uint64_t)m_Address;
  x->type = m_Address.getType();
  memcpy(&x->uuid, &m_Uuid, sizeof(Device::uuid128_t));

  return true;
}

}  // namespace Furble
