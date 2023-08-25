#ifndef CANONEOSREMOTE_H
#define CANONEOSREMOTE_H

#include "CanonEOS.h"

namespace Furble {

/**
 * Canon EOS Partial Abstract Base.
 */
class CanonEOSRemote: public CanonEOS {
 public:
  CanonEOSRemote(const void *data, size_t len) : CanonEOS(data, len) {}
  CanonEOSRemote(NimBLEAdvertisedDevice *pDevice) : CanonEOS(pDevice) {}
  ~CanonEOSRemote(void) {}

 protected:
  const char *CANON_EOS_SVC_REMOTE_UUID = "00050000-0000-1000-0000-d8492fffa821";
  const char *CANON_EOS_CHR_REMOTE_PAIR_UUID = "00050002-0000-1000-0000-d8492fffa821";
  const char *CANON_EOS_CHR_REMOTE_SHUTTER_UUID = "00050003-0000-1000-0000-d8492fffa821";

  const uint8_t CANON_EOS_REMOTE_BUTTON_RELEASE = 0x80;
  const uint8_t CANON_EOS_REMOTE_BUTTON_FOCUS = 0x40;
  const uint8_t CANON_EOS_REMOTE_MODE_IMMEDIATE = 0x0C;

  bool connect(ezProgressBar &progress_bar);
  void shutterPress(void);
  void shutterRelease(void);
  void focusPress(void);
  void focusRelease(void);
  void disconnect(void);
};

}  // namespace Furble
#endif
