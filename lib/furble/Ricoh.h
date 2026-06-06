#ifndef RICOH_H
#define RICOH_H

#include <NimBLERemoteCharacteristic.h>

#include "Camera.h"

namespace Furble {
/**
 * RICOH GR III / IIIx / IV BLE remote.
 *
 * Shutter control via ShootingFlavor + OperationRequest characteristics (single-write
 * capture; no half-press/release). GPS geotagging deferred — stub validates and logs.
 * MITM LE Secure Connections pairing with numeric comparison.
 *
 * Protocol reference: dm-zharov/ricoh-gr-bluetooth-api, Android HCI snoop analysis.
 */
class Ricoh: public Camera {
 public:
  Ricoh(const void *data, size_t len);
  Ricoh(const NimBLEAdvertisedDevice *pDevice);

  static bool matches(const NimBLEAdvertisedDevice *pDevice);

  void shutterPress(void) override final;
  void shutterRelease(void) override final;
  void focusPress(void) override final;
  void focusRelease(void) override final;
  void updateGeoData(const gps_t &gps, const timesync_t &timesync) override final;

  size_t getSerialisedBytes(void) const override final;
  bool serialise(void *buffer, size_t bytes) const override final;

 private:
  typedef struct _ricoh_t {
    char name[MAX_NAME];
    uint64_t address;
    uint8_t type;
  } ricoh_t;

  enum class OperationCode : uint8_t {
    NOP = 0,
    START = 1,
    STOP = 2,
  };

  enum class OperationParameter : uint8_t {
    NO_AF = 0,
    AF = 1,
    GREEN_BUTTON = 2,
  };

  enum class ShootingFlavor : uint8_t {
    IMMEDIATE = 0,
    TIMER_2S = 2,
  };

  // Camera Information Service
  static const NimBLEUUID INFO_SVC_UUID;
  static const NimBLEUUID MODEL_CHR_UUID;

  // Camera Service
  static const NimBLEUUID CAMERA_SVC_UUID;
  static const NimBLEUUID POWER_CHR_UUID;
  static const NimBLEUUID OPERATION_MODE_CHR_UUID;

  // Shooting Service
  static const NimBLEUUID SHOOTING_SVC_UUID;
  static const NimBLEUUID SHOOTING_FLAVOR_CHR_UUID;
  static const NimBLEUUID OPERATION_REQUEST_CHR_UUID;
  static const NimBLEUUID CAPTURE_STATUS_CHR_UUID;
  static const NimBLEUUID SELF_TIMER_CHR_UUID;

  // Bluetooth Control Service
  static const NimBLEUUID BT_CONTROL_SVC_UUID;
  static const NimBLEUUID PAIRED_DEVICE_NAME_CHR_UUID;

  // GPS Information Service
  static const NimBLEUUID GPS_SVC_UUID;
  static const NimBLEUUID GPS_INFO_CHR_UUID;

  // Location Control Service
  static const NimBLEUUID LOCATION_CONTROL_SVC_UUID;
  static const NimBLEUUID LOCATION_CONTROL_CHR_UUID;

  NimBLERemoteCharacteristic *m_Power = nullptr;
  NimBLERemoteCharacteristic *m_OperationMode = nullptr;
  NimBLERemoteCharacteristic *m_ShootingFlavor = nullptr;
  NimBLERemoteCharacteristic *m_OperationRequest = nullptr;
  NimBLERemoteCharacteristic *m_CaptureStatus = nullptr;
  NimBLERemoteCharacteristic *m_SelfTimer = nullptr;
  NimBLERemoteCharacteristic *m_PairedDeviceName = nullptr;
  NimBLERemoteCharacteristic *m_GpsInfo = nullptr;
  NimBLERemoteCharacteristic *m_LocationControl = nullptr;

  bool _connect(void) override final;
  void _disconnect(void) override final;

  void onPassKeyEntry(NimBLEConnInfo &connInfo) override;
  uint32_t onPassKeyDisplay(NimBLEConnInfo &connInfo) override;
  void onConfirmPasskey(NimBLEConnInfo &connInfo, uint32_t pin) override;
  void onAuthenticationComplete(NimBLEConnInfo &connInfo) override;

  static bool nameMatches(const std::string &name);
  void clearRemoteState(void);
  bool writeByte(NimBLERemoteCharacteristic *pChr, uint8_t value, const char *label);
  bool writeOperation(OperationCode code, OperationParameter parameter);
  bool subscribeCharacteristic(NimBLERemoteCharacteristic *pChr, const char *label);
  bool setShootingFlavor(ShootingFlavor flavor);
};

}  // namespace Furble
#endif
