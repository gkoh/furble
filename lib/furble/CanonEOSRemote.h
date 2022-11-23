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
  bool connect(NimBLEClient *pClient, ezProgressBar &progress_bar);
  void shutterPress(void);
  void shutterRelease(void);
  void focusPress(void);
  void focusRelease(void);
  void disconnect(void);
};

}  // namespace Furble
#endif
