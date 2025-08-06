#ifndef FUJIFILM_SECURE_H
#define FUJIFILM_SECURE_H

#include <NimBLEUUID.h>

#include "Fujifilm.h"

namespace Furble {
/**
 * Fujifilm secure.
 *
 * Secured Bluetooth communications. For cameras running newer firmware from
 * circa July 2025 onwards.
 */
class FujifilmSecure: public Fujifilm {
 public:
  FujifilmSecure(const void *data, size_t len);
  FujifilmSecure(const NimBLEAdvertisedDevice *pDevice);

  /**
   * Determine if the advertised BLE device is a Fujifilm secure camera.
   */
  static bool matches(const NimBLEAdvertisedDevice *pDevice);

  size_t getSerialisedBytes(void) const override final;
  bool serialise(void *buffer, size_t bytes) const override final;

 protected:
  bool _connect(void) override final;

 private:
  /**
   * Non-volatile storage type.
   */
  typedef struct _nvs_t {
    char name[MAX_NAME]; /** Human readable device name. */
    uint64_t address;    /** Device MAC address. */
    uint8_t type;        /** Address type. */
  } nvs_t;

  // Advertised service UUID
  static const NimBLEUUID SERVICE_UUID;

  // Primary service UUID
  static const NimBLEUUID PRI_SVC_UUID;

  // Unknown service UUID
  const NimBLEUUID UNK0_SVC_UUID {0x123d8f06, 0x62a1, 0x4935, 0x9322833c531ee225};

  // Service UUID for NOT1
  const NimBLEUUID NOT1_SVC_UUID {0x4c0020fe, 0xf3b6, 0x40de, 0xacc977d129067b14};
  const NimBLEUUID NOT1_CHR_UUID {0xe6692c5c, 0xb7cd, 0x44f4, 0x95fceda07ce32560};

  // Service UUID for other notifications
  const NimBLEUUID NOTX_SVC_UUID {0x4e941240, 0xd01d, 0x46b9, 0xa5ea67636806830b};
  const NimBLEUUID NOT2_CHR_UUID {0xaab609c4, 0x94dd, 0x4d89, 0xbc60665d5090b828};
  const NimBLEUUID NOT3_CHR_UUID {0x2a125640, 0x706d, 0x4dd1, 0xb420c0f4ab93c361};
  const NimBLEUUID NOT4_CHR_UUID {0x82a9f452, 0xc5ce, 0x4ef5, 0x82033fc9a47f8171};
  const NimBLEUUID NOT5_CHR_UUID {0xdeef7187, 0x3f43, 0x4364, 0x9e2211a8c8a15951};
  const NimBLEUUID NOT6_CHR_UUID {0xc95d91ae, 0xb247, 0x4d6d, 0x86617dd5d6a0f85b};

  // Shutter service UUID
  const NimBLEUUID SHUTTER_SVC_UUID {0x6514eb81, 0x4e8f, 0x458d, 0xaa2ae691336cdfac};

  bool subscribeNotification(const NimBLEUUID &svc, const NimBLEUUID &chr);
};

}  // namespace Furble
#endif
