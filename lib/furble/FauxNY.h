#ifndef FAUXNY_H
#define FAUXNY_H

#include "Camera.h"
#include "Device.h"

namespace Furble {
/**
 * FauxNY fake virtual camera
 */
class FauxNY: public Camera {
 public:
  FauxNY(const void *data, size_t len);
  FauxNY(void);

  static bool matches(void);

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
  typedef struct _fauxNY_t {
    char name[MAX_NAME]; /** Human readable device name. */
    uint64_t address;    /** Device MAC address. */
    uint8_t type;        /** Address type. */
    uint32_t id;         /** Device ID. */
  } fauxNY_t;

  static constexpr const char *m_FauxNYStr = "FauxNY";
  uint64_t m_ID;
};

}  // namespace Furble
#endif
