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
  virtual void onConnect(NimBLEAddress address);
  virtual void onComplete(NimBLEAddress address);
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
  void start(unsigned int duration,
             HIDServerCallbacks *hidCallbacks,
             NimBLEAddress *address = nullptr);

  void stop(void);

  NimBLECharacteristic *getInput(void);
  NimBLEConnInfo getConnInfo(NimBLEAddress &address);
  void disconnect(NimBLEAddress &address);
  bool isConnected(void);

 private:
  HIDServer();
  ~HIDServer();

  static HIDServer *hidServer;  // singleton

  void onConnect(NimBLEServer *pServer, ble_gap_conn_desc *desc);
  void onDisconnect(NimBLEServer *pServer, ble_gap_conn_desc *desc);
  void onAuthenticationComplete(ble_gap_conn_desc *desc);

  bool m_Connected = false;
  NimBLEServer *m_Server = nullptr;
  NimBLEHIDDevice *m_HID = nullptr;
  NimBLECharacteristic *m_Input = nullptr;
  NimBLEAdvertising *m_Advertising = nullptr;
  HIDServerCallbacks *m_hidCallbacks = nullptr;
};

}  // namespace Furble

#endif
