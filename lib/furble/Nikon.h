#ifndef NIKON_H
#define NIKON_H

#include <NimBLERemoteCharacteristic.h>

#include "Blowfish.h"
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
  bool _connect(void) override final;
  void _disconnect(void) override final;

 private:
  class Pairing {
   public:
    enum class Type {
      REMOTE,
      SMART_DEVICE,
    };

    /** Identifier. */
    typedef struct __attribute__((packed)) _id_t {
      uint32_t device;  // sent in manufacturer data in reconnect
      uint32_t nonce;
    } id_t;

    /** Pairing message. */
    typedef struct __attribute__((packed)) _msg_t {
      uint8_t stage;
      union {
        struct __attribute__((packed)) {
          uint32_t timestampH;
          uint32_t timestampL;
        };
        uint64_t timestamp;
      };
      union {
        Pairing::id_t id;
        char serial[8];
      };
    } msg_t;

    virtual const msg_t *processMessage(const msg_t &msg) = 0;
    const msg_t *getMessage(void) const;
    Type getType(void) const;

   protected:
    Pairing(const Pairing::Type type, const uint64_t timestamp, const Pairing::id_t id);

    msg_t *m_Msg = nullptr;
    std::array<msg_t, 5> m_Stage;

   private:
    const Pairing::Type m_Type;
  };

  class RemotePairing: public Pairing {
   public:
    RemotePairing(const uint64_t timestamp, const Pairing::id_t &id);

    const msg_t *processMessage(const msg_t &msg) final;
  };

  class SmartPairing: public Pairing, Blowfish {
   public:
    SmartPairing(const uint64_t timestamp, const Pairing::id_t id);

    const msg_t *processMessage(const msg_t &msg) final;

    std::array<uint32_t, 2> hash(const uint32_t *src, size_t len) const;

   private:
    void scramble(uint32_t *pL, uint32_t *pR) const;
    int8_t findSaltIndex(const msg_t &msg);

    static const std::vector<uint8_t> KEY;
    static const std::array<std::array<uint32_t, 2>, 8> SALT;
    int8_t m_Salt = -1;
  };

  static constexpr uint16_t COMPANY_ID = 0x0399;

  /** Connect saved advertised manufacturer data. */
  typedef struct __attribute__((packed)) _nikon_adv_t {
    uint16_t companyID;
    uint32_t device;
    uint8_t zero;
  } nikon_adv_t;

  /** Time synchronisation. */
  typedef struct __attribute__((packed)) _nikon_time_t {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
  } nikon_time_t;

  // 10 bytes
  typedef struct __attribute__((packed)) _timesync_msg_t {
    nikon_time_t time;
    uint8_t dst_offset;
    uint8_t tz_offset_hours;
    uint8_t tz_offset_minutes;
  } timesync_msg_t;

  // 41 bytes
  /** Location synchronisation. */
  typedef struct __attribute__((packed)) _nikon_geo_t {
    uint16_t header;             // 0x7f00 = isLat | isLon | isSat | isAlt |
                                 // isPos | isGps | isMap
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
    uint16_t extras;  // always 0x0050? no. of satellites
    uint16_t altitude;
    nikon_time_t time;
    uint8_t subseconds;
    uint8_t valid;        // 0x01 == valid
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
    Pairing::id_t id;    /** Unique identifiers. */
  } nikon_t;

  // Re-pair scan time
  static constexpr uint32_t SCAN_TIME_MS = 60000;

  // Service UUID from advertisement data
  static const NimBLEUUID SERVICE_UUID;

  // Subscription UUIDs
  const NimBLEUUID NOT1_CHR_UUID {0x00002008, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};
  const NimBLEUUID NOT2_CHR_UUID {0x0000200a, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};

  // Stage UUIDs
  const NimBLEUUID PAIR_CHR_UUID {0x00002000, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};

  const NimBLEUUID UNK1_CHR_UUID {0x00002009, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};
  const NimBLEUUID UNK2_CHR_UUID {0x00002080, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};
  const NimBLEUUID UNK3_CHR_UUID {0x00002086, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};
  const NimBLEUUID UNK4_CHR_UUID {0x00002001, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};
  const NimBLEUUID UNK5_CHR_UUID {0x00002082, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};
  const NimBLEUUID UNK6_CHR_UUID {0x00002084, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};
  const NimBLEUUID UNK7_CHR_UUID {0x00002001, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};

  // Identifier UUIDs
  const NimBLEUUID ID_CHR_UUID {0x00002002, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};

  // Time and Location UUIDs
  const NimBLEUUID TIME_CHR_UUID {0x00002006, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};
  const NimBLEUUID GEO_CHR_UUID {0x00002007, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};

  // Unknown UUIDs
  const NimBLEUUID UNK0_CHR_UUID {0x00002009, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};

  // Remote UUIDs
  const NimBLEUUID REMOTE_R1_CHR_UUID {0x00002080, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};
  const NimBLEUUID REMOTE_W1_CHR_UUID {0x00002082, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};
  const NimBLEUUID REMOTE_SHUTTER_CHR_UUID {0x00002083, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};
  const NimBLEUUID REMOTE_IND1_CHR_UUID {0x00002084, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};
  const NimBLEUUID REMOTE_R2_CHR_UUID {0x00002086, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};
  const NimBLEUUID REMOTE_PAIR_CHR_UUID {0x00002087, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};

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

  uint64_t m_Timestamp;
  Pairing::id_t m_ID;
  NimBLERemoteCharacteristic *m_PairChr = nullptr;
  Pairing *m_Pairing = nullptr;

  QueueHandle_t m_Queue;

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
