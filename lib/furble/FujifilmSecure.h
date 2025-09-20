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
class FujifilmSecure: public Fujifilm, public NimBLEScanCallbacks {
 public:
  FujifilmSecure(const void *data, size_t len);
  FujifilmSecure(const NimBLEAdvertisedDevice *pDevice);
  ~FujifilmSecure(void);

  /**
   * Determine if the advertised BLE device is a Fujifilm secure camera.
   */
  static bool matches(const NimBLEAdvertisedDevice *pDevice);

  size_t getSerialisedBytes(void) const override final;
  bool serialise(void *buffer, size_t bytes) const override final;

 protected:
  bool _connect(void) override final;

 private:
  static constexpr size_t SERIAL_LEN = 5;

  /** Serial advertisement. */
  typedef struct __attribute__((packed)) {
    uint8_t data[SERIAL_LEN];
  } serial_t;

  /** Advertisement data. */
  typedef struct __attribute__((packed)) {
    fujifilm_adv_t adv;
    serial_t serial;
  } adv_secure_t;

  /**
   * Non-volatile storage type.
   */
  typedef struct _nvs_t {
    char name[MAX_NAME]; /** Human readable device name. */
    uint64_t address;    /** Device MAC address. */
    uint8_t type;        /** Address type. */
    serial_t serial;     /** Camera serial. */
  } nvs_t;

  /** Subscription item. */
  typedef struct _sub_t {
    std::string name;          /** Subscription name. */
    const NimBLEUUID &service; /** Service UUID. */
    const NimBLEUUID &uuid;    /** CCC UUID. */
    bool notification;         /** Notification or indication? */
  } sub_t;

  // Advertised service UUID
  static const NimBLEUUID SERVICE_UUID;

  // Primary service UUID
  static const NimBLEUUID PRI_SVC_UUID;

  // Pairing service UUID
  const NimBLEUUID PAIR_SVC_UUID {0x123d8f06, 0x62a1, 0x4935, 0x9322833c531ee225};

  // read and ack - 0x07960000 -> 0x07960020
  const NimBLEUUID STATUS_CHR_UUID {0xf557d96b, 0x8284, 0x4667, 0x8793b971c1deca2a};

  // Identifier UUID
  const NimBLEUUID IDENT_CHR_UUID {0x85b9163e, 0x62d1, 0x49ff, 0xa6f5054b4630d4a1};

  // Unknown SVC UUID - notification 3
  const NimBLEUUID NOT3_SVC_UUID {0x804daa8e, 0xffeb, 0x4ab3, 0x8e756edd7303208d};
  const NimBLEUUID NOT3_CHR_UUID {0x7170fd5a, 0x56d9, 0x4c19, 0xb0437a7047d8e1a0};

  // Unknown UUID - SVC_READ_UUID
  const NimBLEUUID UNK0_CHR_UUID {0x98934b2c, 0x756c, 0x4632, 0xaa2fdcba1bfec824};

  // UUID for NOT6
  const NimBLEUUID NOT6_CHR_UUID {0xe6692c5c, 0xb7cd, 0x44f4, 0x95fceda07ce32560};

  // Service UUID for other notifications
  const NimBLEUUID NOTX_SVC_UUID {0x4e941240, 0xd01d, 0x46b9, 0xa5ea67636806830b};
  const NimBLEUUID NOT4_CHR_UUID {0xbf6dc9cf, 0x3606, 0x4ec9, 0xa4c8d77576e93ea4};
  const NimBLEUUID NOT5_CHR_UUID {0x75823784, 0xfbb7, 0x4b71, 0xabaecd9a34072e3c};
  const NimBLEUUID NOT7_CHR_UUID {0xaab609c4, 0x94dd, 0x4d89, 0xbc60665d5090b828};
  const NimBLEUUID NOT8_CHR_UUID {0x2a125640, 0x706d, 0x4dd1, 0xb420c0f4ab93c361};
  const NimBLEUUID NOT9_CHR_UUID {0x82a9f452, 0xc5ce, 0x4ef5, 0x82033fc9a47f8171};
  const NimBLEUUID NOT10_CHR_UUID {0xdeef7187, 0x3f43, 0x4364, 0x9e2211a8c8a15951};
  const NimBLEUUID GEOTAG_SYNC_INTERVAL_UUID {0xc95d91ae, 0xb247, 0x4d6d, 0x86617dd5d6a0f85b};

  // Shutter service UUID
  const NimBLEUUID SHUTTER_SVC_UUID {0x6514eb81, 0x4e8f, 0x458d, 0xaa2ae691336cdfac};

  // Scan time for previously paired camera
  static constexpr uint32_t SCAN_TIME_MS = 60000;

  /** Called during scanning for connection to saved device. */
  void onResult(const NimBLEAdvertisedDevice *pDevice) override final;

  QueueHandle_t m_Queue = NULL;
  serial_t m_Serial = {0x00};
};

}  // namespace Furble
#endif
