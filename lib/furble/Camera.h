#ifndef CAMERA_H
#define CAMERA_H

#include <atomic>

#include <NimBLEAddress.h>
#include <NimBLEClient.h>
#include <NimBLEDevice.h>

#include "FurbleTypes.h"

#define MAX_NAME (64)

namespace Furble {

/**
 * Represents a single target camera.
 */
class Camera {
 public:
  /**
   * Camera types.
   */
  enum class Type : uint32_t {
    FUJIFILM = 1,
    CANON_EOS_M6 = 2,
    CANON_EOS_RP = 3,
    MOBILE_DEVICE = 4,
  };

  /**
   * GPS data type.
   */
  typedef struct _gps_t {
    double latitude;
    double longitude;
    double altitude;
  } gps_t;

  /**
   * Time synchronisation type.
   */
  typedef struct _timesync_t {
    unsigned int year;
    unsigned int month;
    unsigned int day;
    unsigned int hour;
    unsigned int minute;
    unsigned int second;
  } timesync_t;

  ~Camera();

  /**
   * Wrapper for protected pure virtual Camera::connect().
   */
  bool connect(esp_power_level_t power);

  /**
   * Disconnect from the target.
   */
  virtual void disconnect(void) = 0;

  /**
   * Send a shutter button press command.
   */
  virtual void shutterPress(void) = 0;

  /**
   * Send a shutter button release command.
   */
  virtual void shutterRelease(void) = 0;

  /**
   * Send a focus button press command.
   */
  virtual void focusPress(void) = 0;

  /**
   * Send a focus button release command.
   */
  virtual void focusRelease(void) = 0;

  /**
   * Update geotagging data.
   */
  virtual void updateGeoData(const gps_t &gps, const timesync_t &timesync) = 0;

  virtual size_t getSerialisedBytes(void) const = 0;
  virtual bool serialise(void *buffer, size_t bytes) const = 0;

  /**
   * Checks if the client is still connected.
   */
  virtual bool isConnected(void) const;

  /**
   * Camera is active (ie. connect() has succeeded previously).
   */
  bool isActive(void) const;

  /**
   * Set camera activity state.
   */
  void setActive(bool active);

  const Type &getType(void) const;

  const std::string &getName(void) const;

  const NimBLEAddress &getAddress(void) const;

  float getConnectProgress(void) const;

 protected:
  Camera(Type type);
  std::atomic<float> m_Progress;

  /**
   * Connect to the target camera such that it is ready for shutter control.
   *
   * This should include connection and pairing as needed for the target
   * device.
   *
   * @return true if the client is now ready for shutter control
   */
  virtual bool connect(void) = 0;

  NimBLEAddress m_Address = NimBLEAddress{};
  NimBLEClient *m_Client;
  std::string m_Name;
  bool m_Connected = false;

 private:
  const uint16_t m_MinInterval = BLE_GAP_INITIAL_CONN_ITVL_MIN;
  const uint16_t m_MaxInterval = BLE_GAP_INITIAL_CONN_ITVL_MAX;
  // allow a packet to skip
  const uint16_t m_Latency = 1;
  // double the disconnect timeout
  const uint16_t m_Timeout = (2 * BLE_GAP_INITIAL_SUPERVISION_TIMEOUT);

  const Type m_Type;

  bool m_Active = false;
};
}  // namespace Furble

#endif
