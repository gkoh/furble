#ifndef HIDSERVER_H
#define HIDSERVER_H

#include <NimBLEHIDDevice.h>
#include <NimBLEServer.h>

namespace Furble {

class HIDServer;

/**
 * HID server callbacks.
 */
class HIDServerCallbacks {
 public:
  virtual void onComplete(const NimBLEAddress &address, const std::string &name);
};

/**
 * BLE HID Server class.
 */
class HIDServer: public NimBLEServerCallbacks {
 public:
  static HIDServer *getInstance(void);

  /**
   * Start advertising.
   *
   * @param[in] address If specified, start directed advertising.
   */
  void start(NimBLEAddress *address = nullptr, HIDServerCallbacks *hidCallbacks = nullptr);

  void stop(void);

  NimBLECharacteristic *getInput(void);
  NimBLEConnInfo getConnInfo(NimBLEAddress &address);
  void disconnect(NimBLEAddress &address);
  bool isConnected(const NimBLEAddress &address);

 private:
  HIDServer();
  ~HIDServer();

  static HIDServer *hidServer;  // singleton

  void onAuthenticationComplete(const NimBLEConnInfo &connInfo, const std::string &name) override;
  void onIdentity(const NimBLEConnInfo &connInfo) override;

  NimBLEServer *m_Server = nullptr;
  NimBLEHIDDevice *m_HID = nullptr;
  NimBLECharacteristic *m_Input = nullptr;
  NimBLEAdvertising *m_Advertising = nullptr;
  HIDServerCallbacks *m_hidCallbacks = nullptr;
};

}  // namespace Furble

#endif
