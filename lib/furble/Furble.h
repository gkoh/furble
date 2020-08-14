#ifndef FURBLE_H
#define FURBLE_H

#include <M5ez.h>
#include <Preferences.h>
#include <NimBLEClient.h>
#include <NimBLEAdvertisedDevice.h>
#include <vector>

#define FURBLE_STR "furble"

#define MAX_NAME (64)

#define XT30_TOKEN_LEN (4)
#define UUID128_LEN (16)
#define UUID128_AS_32_LEN (UUID128_LEN / sizeof(uint32_t))

typedef enum {
  FURBLE_FUJIFILM_XT30 = 1,
  FURBLE_CANON_EOS_M6 = 2,
} device_type_t;

typedef struct _uuid128_t {
  union {
    uint32_t uint32[UUID128_AS_32_LEN];
    uint8_t uint8[UUID128_LEN];
  };
} uuid128_t;

namespace Furble {

class Device {
  public:
    virtual bool connect(NimBLEClient *pClient, ezProgressBar &progress_bar)=0;
    virtual void disconnect(void)=0;
    virtual void shutterPress(void)=0;
    virtual void shutterRelease(void)=0;

    const char *getName(void);
    void save(void);
    void remove(void);

    static void loadDevices(std::vector<Furble::Device *>&device_list);

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
    virtual device_type_t getDeviceType(void)=0;
    virtual size_t getSerialisedBytes(void)=0;
    virtual bool serialise(void *buffer, size_t bytes)=0;

    void fillSaveName(char *name);
};

class FujifilmXT30: public Device {
  public:
    FujifilmXT30(const void *data, size_t len);
    FujifilmXT30(NimBLEAdvertisedDevice *pDevice);
    ~FujifilmXT30(void);

    /**
     * Determine if the advertised BLE device is a Fujifilm X-T30.
     */
    static bool matches(NimBLEAdvertisedDevice *pDevice);

    bool connect(NimBLEClient *pClient, ezProgressBar &progress_bar);
    void shutterPress(void);
    void shutterRelease(void);
    void disconnect(void);
    void print(void);

  private:
    device_type_t getDeviceType(void);
    size_t getSerialisedBytes(void);
    bool serialise(void *buffer, size_t bytes);

    uint8_t m_Token[XT30_TOKEN_LEN] = {0};
};

class CanonEOSM6: public Device {
  public:
    CanonEOSM6(const void *data, size_t len);
    CanonEOSM6(NimBLEAdvertisedDevice *pDevice);
    ~CanonEOSM6(void);

    /**
     * Determine if the advertised BLE device is a Canon EOS M6.
     */
    static bool matches(NimBLEAdvertisedDevice *pDevice);

    const char *getName(void);
    bool connect(NimBLEClient *pClient, ezProgressBar &progress_bar);
    void shutterPress(void);
    void shutterRelease(void);
    void disconnect(void);

  private:
    device_type_t getDeviceType(void);
    size_t getSerialisedBytes(void);
    bool serialise(void *buffer, size_t bytes);

    uuid128_t m_Uuid;
};

}

#endif
