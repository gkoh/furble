#ifndef DEVICE_H
#define DEVICE_H

#include <cstdint>

#define DEVICE_ID_STR_MAX (16)
#define UUID128_LEN (16)
#define UUID128_AS_32_LEN (UUID128_LEN / sizeof(uint32_t))

namespace Furble {

class Device {
 public:
  /**
   * UUID type.
   */
  typedef struct _uuid128_t {
    union {
      uint32_t uint32[UUID128_AS_32_LEN];
      uint8_t uint8[UUID128_LEN];
    };
  } uuid128_t;

  /**
   * Initialise the device.
   */
  static void init(void);

  /**
   * Return a device consistent 128-bit UUID.
   */
  static void getUUID128(uuid128_t *uuid);

  /**
   * Return pseudo-unique identifier string of this device.
   */
  static const char *getStringID(void);

 private:
  static uuid128_t g_Uuid;
  static char g_StringID[DEVICE_ID_STR_MAX];
};
}  // namespace Furble

#endif
