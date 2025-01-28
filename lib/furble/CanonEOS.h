#ifndef CANONEOS_H
#define CANONEOS_H

#include <NimBLEUUID.h>

#include "Camera.h"
#include "Device.h"

namespace Furble {
/**
 * Canon EOS Partial Abstract Base.
 */
class CanonEOS: public Camera {
 public:
 protected:
  typedef struct _eos_t {
    char name[MAX_NAME];    /** Human readable device name. */
    uint64_t address;       /** Device MAC address. */
    uint8_t type;           /** Address type. */
    Device::uuid128_t uuid; /** Our UUID. */
  } eos_t;

  CanonEOS(Type type, const void *data, size_t len);
  CanonEOS(Type type, const NimBLEAdvertisedDevice *pDevice);

  Device::uuid128_t m_Uuid;

  size_t getSerialisedBytes(void) const override;
  bool serialise(void *buffer, size_t bytes) const override;

 protected:
  void _disconnect(void) override final;

 private:
};

}  // namespace Furble
#endif
