#ifndef FURBLE_H
#define FURBLE_H

#include <M5ez.h>
#include <Preferences.h>
#include <NimBLEClient.h>
#include <NimBLEAdvertisedDevice.h>
#include <vector>

#define FURBLE_STR "furble"

#define XT30_TOKEN_LEN (4)

typedef enum {
  FURBLE_FUJIFILM_XT30 = 1,
} device_type_t;

namespace Furble {

class Device {
  public:
    virtual const char *getName(void)=0;
    virtual bool connect(NimBLEClient *pClient, ezProgressBar &progress_bar)=0;
    virtual void shutterPress(void)=0;
    virtual void shutterRelease(void)=0;

    void save(void);
    void remove(void);

    static void loadDevices(std::vector<Furble::Device *>device_list);

  protected:
    NimBLEAddress m_Address = NimBLEAddress("");

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

    const char *getName(void);
    bool connect(NimBLEClient *pClient, ezProgressBar &progress_bar);
    void shutterPress(void);
    void shutterRelease(void);
    void disconnect(void);
    void print(void);

  private:
    device_type_t getDeviceType(void);
    size_t getSerialisedBytes(void);
    bool serialise(void *buffer, size_t bytes);

    NimBLEClient *m_Client;
    std::string m_Name;
    uint8_t m_Token[XT30_TOKEN_LEN] = {0};
};

class NVS {
  public:
    std::vector<Furble::Device>loadDevices(void);
    bool saveDevice(Furble::Device &device);

  private:
};

}

#endif
