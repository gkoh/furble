#ifndef FUJIFILM_BASIC_H
#define FUJIFILM_BASIC_H

#include "Fujifilm.h"

namespace Furble {
/**
 * Fujifilm basic.
 *
 * Unsecured communications. Originally for Fujifilm cameras, but no longer
 * working for newer firmware with secured Bluetooth connections.
 */
class FujifilmBasic: public Fujifilm {
 public:
  FujifilmBasic(const void *data, size_t len);
  FujifilmBasic(const NimBLEAdvertisedDevice *pDevice);

  /**
   * Determine if the advertised BLE device is a Fujifilm basic.
   */
  static bool matches(const NimBLEAdvertisedDevice *pDevice);

  size_t getSerialisedBytes(void) const override final;
  bool serialise(void *buffer, size_t bytes) const override final;

 protected:
  bool _connect(void) override final;

 private:
  static constexpr size_t ADV_LEN = 7;
  static constexpr size_t TOKEN_LEN = 4;

  static constexpr uint8_t TYPE_TOKEN = 0x02;

  /**
   * Advertisement manufacturer data.
   */
  typedef struct __attribute__((packed)) {
    fujifilm_adv_t adv;
    uint8_t token[TOKEN_LEN];
  } adv_basic_t;

  /**
   * Non-volatile storage type.
   */
  typedef struct _nvs_t {
    char name[MAX_NAME];      /** Human readable device name. */
    uint64_t address;         /** Device MAC address. */
    uint8_t type;             /** Address type. */
    uint8_t token[TOKEN_LEN]; /** Pairing token. */
  } nvs_t;

  // Primary service UUID
  static const NimBLEUUID PRI1_SVC_UUID;

  // Secondary service UUID
  static const NimBLEUUID PRI2_SVC_UUID;

  // Shutter service UUID
  const NimBLEUUID SVC_SHUTTER_UUID {0x6514eb81, 0x4e8f, 0x458d, 0xaa2ae691336cdfac};

  void print_token(const std::array<uint8_t, TOKEN_LEN> &token);

  std::array<uint8_t, TOKEN_LEN> m_Token = {0};
};

}  // namespace Furble
#endif
