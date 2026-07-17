#ifndef NIKON_H
#define NIKON_H

#include <cstdint>
#include <memory>

#include "Camera.h"
#include "NikonBase.h"
#include "Scan.h"

namespace Furble {
/**
 * Nikon camera support.
 *
 * Unifies both Remote and Smart protocols.
 * Originally implemented and tested against Nikon Coolpix B600 in remote mode.
 */
class Nikon: public Camera, public NimBLEScanCallbacks {
 public:
  Nikon(const void *data, size_t len);
  Nikon(const NimBLEAdvertisedDevice *pDevice);
  ~Nikon();

  /** Determine if the advertised BLE device is a Nikon camera. */
  static bool matches(const NimBLEAdvertisedDevice *pDevice);

  void shutterPress(void) override;
  void shutterRelease(void) override;
  void focusPress(void) override;
  void focusRelease(void) override;
  void updateGeoData(const gps_t &gps, const timesync_t &timesync) override;
  size_t getSerialisedBytes(void) const override;
  bool serialise(void *buffer, size_t bytes) const override;

 protected:
  bool _connect(void) override final;
  void _disconnect(void) override final;

 private:
  static constexpr uint16_t COMPANY_ID = 0x0399;

  /** Connect saved advertised manufacturer data. */
  typedef struct __attribute__((packed)) _nikon_adv_t {
    uint16_t companyID;
    uint32_t device;
    uint8_t zero;
  } nikon_adv_t;

  /** Non-volatile storage type. */
  typedef struct _nikon_t {
    char name[MAX_NAME];         /** Human readable device name. */
    uint64_t address;            /** Device MAC address. */
    uint8_t type;                /** Address type. */
    NikonBase::Pairing::id_t id; /** Unique identifiers. */
  } nikon_t;

  // Re-pair scan time
  static constexpr uint32_t SCAN_TIME_MS = 60000;

  uint64_t m_Timestamp;
  NikonBase::Pairing::id_t m_ID;
  QueueHandle_t m_Queue;
  std::unique_ptr<NikonBase> m_Nikon = nullptr;

  /** Advertised device has requisite service UUID. */
  static bool matchesServiceUUID(const NimBLEAdvertisedDevice *pDevice);

  /** Called during scanning for connection to saved device. */
  void onResult(const NimBLEAdvertisedDevice *pDevice) override final;
};

}  // namespace Furble
#endif
