#ifndef CANONEOSSMARTPHONE_H
#define CANONEOSSMARTPHONE_H

#include "CanonEOS.h"

namespace Furble {
/**
 * Canon EOS Partial Abstract Base.
 */
class CanonEOSSmartphone: public CanonEOS {
 public:
  CanonEOSSmartphone(const void *data, size_t len) : CanonEOS(data, len) {}
  CanonEOSSmartphone(NimBLEAdvertisedDevice *pDevice) : CanonEOS(pDevice) {}
  ~CanonEOSSmartphone(void);

 protected:
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

  bool connect(NimBLEClient *pClient, ezProgressBar &progress_bar);
  void shutterPress(void);
  void shutterRelease(void);
  void focusPress(void);
  void focusRelease(void);
  void disconnect(void);
};

}  // namespace Furble
#endif
