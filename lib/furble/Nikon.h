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

  bool processStage(const uint8_t *data, const size_t length);
  uint8_t getStage(void);

 protected:
  bool _connect(void) override final;
  void _disconnect(void) override final;

 private:
  class Pairing {
   public:
    static const size_t MSG_SIZE = 17;

    virtual bool process(const std::array<uint8_t, MSG_SIZE> data) = 0;
    virtual bool process(const uint8_t *data, const size_t length) = 0;
    uint8_t getStage(void);
    const std::array<uint8_t, MSG_SIZE> *getMessage(void) const;

   protected:
    enum class Type {
      REMOTE,
      SMART_DEVICE,
    };
    Pairing(const Pairing::Type type);

    /** Identifier. */
    typedef struct __attribute__((packed)) _id_t {
      uint32_t device;  // sent in manufacturer data in reconnect
      uint32_t nonce;
    } id_t;

    /** Pairing message. */
    typedef struct __attribute__((packed)) _msg_t {
      uint8_t stage;
      uint64_t timestamp;
      union {
        uint64_t key;
        id_t id;
      };
    } msg_t;

    msg_t m_Msg;

   private:
    const Pairing::Type m_Type;
  };

  class RemotePairing: public Pairing {
   public:
    RemotePairing(void);
    bool process(const std::array<uint8_t, MSG_SIZE> data) final;
    bool process(const uint8_t *data, const size_t length) final;
  };

  class SmartPairing: public Pairing, Blowfish {
   public:
    SmartPairing(void);
    bool process(const std::array<uint8_t, MSG_SIZE> data) final;
    bool process(const uint8_t *data, const size_t length) final;
    void search(void);
    std::array<uint32_t, 2> hash(const uint32_t *src, size_t len);

   private:
    void scramble(uint32_t *pL, uint32_t *pR);

    static const std::vector<uint8_t> KEY;
    static const std::array<std::array<uint32_t, 2>, 8> SALT;
    uint8_t m_Salt = 0;
  };

  static constexpr uint16_t COMPANY_ID = 0x0399;

  /** Identifier. */
  typedef struct __attribute__((packed)) _id_t {
    uint32_t device;  // sent in manufacturer data in reconnect
    uint32_t nonce;
  } id_t;

  /** Pairing message. */
  typedef struct __attribute__((packed)) _pair_msg_t {
    uint8_t stage;
    uint64_t timestamp;
    union {
      uint64_t key;
      id_t id;
    };
  } pair_msg_t;

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
    id_t id;             /** Unique identifiers. */
  } nikon_t;

  // Re-pair scan time
  static constexpr uint32_t SCAN_TIME_MS = 60000;

  // Service UUID from advertisement data
  static const NimBLEUUID SERVICE_UUID;

  // Subscription UUIDs
  const NimBLEUUID NOT1_CHR_UUID {0x0002008, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};
  const NimBLEUUID NOT2_CHR_UUID {0x000200a, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};

  // Stage UUIDs
  const NimBLEUUID PAIR_CHR_UUID {0x00002000, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};

  // Identifier UUIDs
  const NimBLEUUID ID_CHR_UUID {0x00002002, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};

  // Time and Location UUIDs
  const NimBLEUUID TIME_CHR_UUID {0x00002006, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};
  const NimBLEUUID GEO_CHR_UUID {0x00002007, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};

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

  id_t m_ID;
  NimBLERemoteCharacteristic *m_PairChr = nullptr;
  Pairing *m_Pairing;
  pair_msg_t m_PairMsg = {0x00, 0x00, 0x00};
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
   * Hash the inputs with the blowfish key schedule.
   */
  void bf_hash(uint32_t *src, uint32_t *dest, uint16_t length);

  /**
   * Called during scanning for connection to saved device.
   */
  void onResult(const NimBLEAdvertisedDevice *pDevice) override final;
};

}  // namespace Furble
#endif
