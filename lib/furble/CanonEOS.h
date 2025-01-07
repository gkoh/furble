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

  const NimBLEUUID SVC_IDEN_UUID {0x00010000, 0x0000, 0x1000, 0x0000d8492fffa821};
  /** 0xf108 */
  const NimBLEUUID CHR_NAME_UUID {0x00010006, 0x0000, 0x1000, 0x0000d8492fffa821};
  /** 0xf104 */
  const NimBLEUUID CHR_IDEN_UUID {0x0001000a, 0x0000, 0x1000, 0x0000d8492fffa821};

  const NimBLEUUID SVC_MODE_UUID {0x00030000, 0x0000, 0x1000, 0x0000d8492fffa821};
  /** 0xf307 */
  const NimBLEUUID CHR_MODE_UUID {0x00030010, 0x0000, 0x1000, 0x0000d8492fffa821};

  const NimBLEUUID SVC_SHUTTER_UUID {0x00030000, 0x0000, 0x1000, 0x0000d8492fffa821};
  /** 0xf311 */
  const NimBLEUUID CHR_SHUTTER_UUID {0x00030030, 0x0000, 0x1000, 0x0000d8492fffa821};

  static constexpr uint8_t PAIR_ACCEPT = 0x02;
  static constexpr uint8_t PAIR_REJECT = 0x03;
  static constexpr uint8_t MODE_PLAYBACK = 0x01;
  static constexpr uint8_t MODE_SHOOT = 0x02;
  static constexpr uint8_t MODE_WAKE = 0x03;

  bool write_value(NimBLEClient *pClient,
                   const NimBLEUUID &serviceUUID,
                   const NimBLEUUID &characteristicUUID,
                   const uint8_t *data,
                   size_t length);

  bool write_prefix(NimBLEClient *pClient,
                    const NimBLEUUID &serviceUUID,
                    const NimBLEUUID &characteristicUUID,
                    const uint8_t prefix,
                    const uint8_t *data,
                    size_t length);

  void shutterPress(void) override;
  void shutterRelease(void) override;
  void focusPress(void) override;
  void focusRelease(void) override;
  void updateGeoData(const gps_t &gps, const timesync_t &timesync) override;
  size_t getSerialisedBytes(void) const override;
  bool serialise(void *buffer, size_t bytes) const override;

  Device::uuid128_t m_Uuid;

 protected:
  bool _connect(void) override;
  void _disconnect(void) override;

 private:
  volatile uint8_t m_PairResult = 0x00;

  void pairCallback(NimBLERemoteCharacteristic *, uint8_t *, size_t, bool);
};

}  // namespace Furble
#endif
