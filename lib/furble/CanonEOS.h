#ifndef CANONEOS_H
#define CANONEOS_H

#include "Device.h"

namespace Furble {
/**
 * Canon EOS Partial Abstract Base.
 */
class CanonEOS: public Device {
 public:
  CanonEOS(const void *data, size_t len);
  CanonEOS(NimBLEAdvertisedDevice *pDevice);
  ~CanonEOS(void);

 protected:
  typedef struct _eos_t {
    char name[MAX_NAME]; /** Human readable device name. */
    uint64_t address;    /** Device MAC address. */
    uint8_t type;        /** Address type. */
    uuid128_t uuid;      /** Our UUID. */
  } eos_t;

  const char *CANON_EOS_SVC_IDEN_UUID = "00010000-0000-1000-0000-d8492fffa821";
  /** 0xf108 */
  const char *CANON_EOS_CHR_NAME_UUID = "00010006-0000-1000-0000-d8492fffa821";
  /** 0xf104 */
  const char *CANON_EOS_CHR_IDEN_UUID = "0001000a-0000-1000-0000-d8492fffa821";

  const char *CANON_EOS_SVC_UNK0_UUID = "00020000-0000-1000-0000-d8492fffa821";
  /** 0xf204 */
  const char *CANON_EOS_CHR_UNK0_UUID = "00020002-0000-1000-0000-d8492fffa821";

  const char *CANON_EOS_SVC_UNK1_UUID = "00030000-0000-1000-0000-d8492fffa821";
  /** 0xf307 */
  const char *CANON_EOS_CHR_UNK1_UUID = "00030010-0000-1000-0000-d8492fffa821";

  const char *CANON_EOS_SVC_SHUTTER_UUID = "00030000-0000-1000-0000-d8492fffa821";
  /** 0xf311 */
  const char *CANON_EOS_CHR_SHUTTER_UUID = "00030030-0000-1000-0000-d8492fffa821";

  const uint8_t CANON_EOS_PAIR_ACCEPT = 0x02;
  const uint8_t CANON_EOS_PAIR_REJECT = 0x03;

  bool write_value(NimBLEClient *pClient,
                   const char *serviceUUID,
                   const char *characteristicUUID,
                   uint8_t *data,
                   size_t length);

  bool write_prefix(NimBLEClient *pClient,
                    const char *serviceUUID,
                    const char *characteristicUUID,
                    uint8_t prefix,
                    uint8_t *data,
                    size_t length);

  bool connect(NimBLEClient *pClient, ezProgressBar &progress_bar);
  void shutterPress(void);
  void shutterRelease(void);
  void focusPress(void);
  void focusRelease(void);
  void updateGeoData(gps_t &gps, timesync_t &timesync);
  void disconnect(void);
  size_t getSerialisedBytes(void);
  bool serialise(void *buffer, size_t bytes);

  uuid128_t m_Uuid;
};

}  // namespace Furble
#endif
