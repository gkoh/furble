#ifndef DEVICE_H
#define DEVICE_H

#define MAX_NAME (64)

#define UUID128_LEN (16)
#define UUID128_AS_32_LEN (UUID128_LEN / sizeof(uint32_t))

namespace Furble {
/**
 * Represents a single target camera.
 */
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

  /**
   * Connect to the target camera such that it is ready for shutter control.
   *
   * This should include connection and pairing as needed for the target
   * device.
   *
   * @return true if the client is now ready for shutter control
   */
  virtual bool connect(NimBLEClient *pClient, ezProgressBar &progress_bar) = 0;

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
  virtual void updateGeoData(gps_t &gps, timesync_t &timesync) = 0;

  const char *getName(void);
  void save(void);
  void remove(void);

  static void loadDevices(std::vector<Furble::Device *> &device_list);

  /**
   * Add matching devices to the list.
   */
  static void match(NimBLEAdvertisedDevice *pDevice, std::vector<Furble::Device *> &list);

  /**
   * Generate a device consistent 128-bit UUID.
   */
  static void getUUID128(uuid128_t *uuid);

 protected:
  NimBLEAddress m_Address = NimBLEAddress("");
  NimBLEClient *m_Client;
  std::string m_Name;

 private:
  virtual device_type_t getDeviceType(void) = 0;
  virtual size_t getSerialisedBytes(void) = 0;
  virtual bool serialise(void *buffer, size_t bytes) = 0;

  void fillSaveName(char *name);
};
}  // namespace Furble

#endif
