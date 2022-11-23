#ifndef CANONEOS_H
#define CANONEOS_H

#include "Device.h"

namespace Furble {
/**
 * Canon EOS Partial Abstract Base.
 */
class CanonEOS: public Device {
 public:
  CanonEOS(const void *data, size_t len);
  CanonEOS(NimBLEAdvertisedDevice *pDevice);
  ~CanonEOS(void);

 protected:
  typedef struct _eos_t {
    char name[MAX_NAME]; /** Human readable device name. */
    uint64_t address;    /** Device MAC address. */
    uint8_t type;        /** Address type. */
    uuid128_t uuid;      /** Our UUID. */
  } eos_t;

  /**
   * Write a value to a characteristic.
   */
  bool write_value(const char *serviceUUID,
                   const char *characteristicUUID,
                   uint8_t *data,
                   size_t length);

  /**
   * Write a value with a prefix to a characteristic.
   */
  bool write_prefix(const char *serviceUUID,
                    const char *characteristicUUID,
                    uint8_t prefix,
                    uint8_t *data,
                    size_t length);

  uuid128_t m_Uuid;

 private:
  size_t getSerialisedBytes(void);
  bool serialise(void *buffer, size_t bytes);
};

}  // namespace Furble
#endif
