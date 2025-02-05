#ifndef NIKON_H
#define NIKON_H

#include <NimBLERemoteCharacteristic.h>

#include "Camera.h"
#include "Scan.h"

namespace Furble {
/**
 * Nikon Coolpix B600.
 */
class Nikon: public Camera, public NimBLEScanCallbacks {
 public:
  Nikon(const void *data, size_t len);
  Nikon(const NimBLEAdvertisedDevice *pDevice);
  ~Nikon();

  /**
   * Determine if the advertised BLE device is a Nikon Coolpix B600.
   */
  static bool matches(const NimBLEAdvertisedDevice *pDevice);

  void shutterPress(void) override;
  void shutterRelease(void) override;
  void focusPress(void) override;
  void focusRelease(void) override;
  void updateGeoData(const gps_t &gps, const timesync_t &timesync) override;
  size_t getSerialisedBytes(void) const override;
  bool serialise(void *buffer, size_t bytes) const override;

 protected:
  bool _connect(void) override;
  void _disconnect(void) override;

 private:
  static constexpr uint16_t COMPANY_ID = 0x0399;

  /** Identifier. */
  typedef struct __attribute__((packed)) _id_t {
    uint32_t device;  // sent in manufacturer data in reconnect
    uint32_t nonce;
  } id_t;

  /** Connect saved advertised manufacturer data. */
  typedef struct __attribute__((packed)) _nikon_adv_t {
    uint16_t companyID;
    uint32_t device;
    uint8_t zero;
  } nikon_adv_t;

  /** Pairing message. */
  typedef struct __attribute__((packed)) _pair_msg_t {
    uint8_t stage;
    uint64_t timestamp;
    union {
      uint64_t key;
      id_t id;
    };
  } pair_msg_t;

  /** Time synchronisation. */
  typedef struct __attribute__((packed)) _nikon_time_t {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
  } nikon_time_t;

  typedef struct __attribute__((packed)) _timesync_msg_t {
    nikon_time_t time;
    uint8_t pad[3];
  } timesync_msg_t;

  /** Location synchronisation. */
  typedef struct __attribute__((packed)) _nikon_geo_t {
    uint16_t header;             // 0x7f00
    uint8_t latitude_direction;  // {N|S}
    uint8_t latitude_degrees;
    uint8_t latitude_minutes;
    uint8_t latitude_seconds;
    uint8_t latitude_fraction;
    uint8_t longitude_direction;  // {E|W}
    uint8_t longitude_degrees;
    uint8_t longitude_minutes;
    uint8_t longitude_seconds;
    uint8_t longitude_fraction;
    uint8_t unknown0[2];
    uint16_t altitude;
    nikon_time_t time;
    uint8_t unknown1[2];
    uint8_t standard[6];  // WGS-84
    uint8_t pad[10];
  } nikon_geo_t;

  /**
   * Non-volatile storage type.
   */
  typedef struct _nikon_t {
    char name[MAX_NAME]; /** Human readable device name. */
    uint64_t address;    /** Device MAC address. */
    uint8_t type;        /** Address type. */
    id_t id;             /** Unique identifiers. */
  } nikon_t;

  // Re-pair scan time
  static constexpr uint32_t SCAN_TIME_MS = 60000;

  // Service UUID from advertisement data
  static const NimBLEUUID SERVICE_UUID;

  // Subscription UUIDs
  const NimBLEUUID NOT1_CHR_UUID {0x0002008, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};
  const NimBLEUUID NOT2_CHR_UUID {0x000200a, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};
  const NimBLEUUID IND0_CHR_UUID {0x0002000, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};

  // Unknown UUIDs
  const NimBLEUUID UNK1_CHR_UUID {0x00002000, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};

  // Identifier UUIDs
  const NimBLEUUID ID_CHR_UUID {0x00002002, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};

  // Time and Location UUIDs
  const NimBLEUUID TIME_CHR_UUID {0x00002006, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};
  const NimBLEUUID GEO_CHR_UUID {0x00002007, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};

  // Remote UUIDs
  const NimBLEUUID R1_CHR_UUID {0x00002080, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};
  const NimBLEUUID W1_CHR_UUID {0x00002082, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};
  const NimBLEUUID REMOTE_SHUTTER_CHR_UUID {0x00002083, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};
  const NimBLEUUID IND1_CHR_UUID {0x00002084, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};
  const NimBLEUUID R2_CHR_UUID {0x00002086, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};
  const NimBLEUUID IND2_CHR_UUID {0x00002087, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};

  // Notification responses
  static const std::array<uint8_t, 2> SUCCESS;
  static const std::array<uint8_t, 2> GEO;  // ? Geo request?

  // Task notification bits
  static constexpr uint32_t NOTIFY_SUCCESS = (1 << 1);

  // Control modes
  static const uint8_t MODE_SHUTTER = 0x02;
  static const uint8_t MODE_VIDEO = 0x03;
  static const uint8_t MODE_MENU = 0x04;
  static const uint8_t MODE_PLAYBACK = 0x05;

  // Control commands
  static const uint8_t CMD_PRESS = 0x02;
  static const uint8_t CMD_RELEASE = 0x00;

  id_t m_ID;
  pair_msg_t m_RemotePair[4] = {
      {0x01, 0x00, 0x00},
      {0x02, 0x00, 0x00},
      {0x03, 0x00, 0x00},
      {0x04, 0x00, 0x00},
  };

  QueueHandle_t m_Queue;
  TaskHandle_t m_Task;

  /**
   * Convert decimal degrees to degrees, minutes, seconds and fraction.
   */
  void degreesToDMS(double value,
                    uint8_t &degrees,
                    uint8_t &minutes,
                    uint8_t &seconds,
                    uint8_t &fraction);

  /**
   * Called during scanning for connection to saved device.
   */
  void onResult(const NimBLEAdvertisedDevice *pDevice) override final;
};

}  // namespace Furble
#endif
