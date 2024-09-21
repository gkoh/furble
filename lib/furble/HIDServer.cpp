#include <NimBLEDevice.h>

#include "HIDServer.h"

#define HID_GENERIC_REMOTE (0x180)

// AUTO-GENERATED by WaratahCmd.exe (https://github.com/microsoft/hidtools)
static const uint8_t hidReportDescriptor[] = {
    0x05,
    0x0C,  // UsagePage(Consumer[0x000C])
    0x09,
    0x01,  // UsageId(Consumer Control[0x0001])
    0xA1,
    0x01,  // Collection(Application)
    0x85,
    0x01,  //     ReportId(1)
    0x09,
    0xE9,  //     UsageId(Volume Increment[0x00E9])
    0x15,
    0x00,  //     LogicalMinimum(0)
    0x25,
    0x01,  //     LogicalMaximum(1)
    0x95,
    0x01,  //     ReportCount(1)
    0x75,
    0x01,  //     ReportSize(1)
    0x81,
    0x02,  //     Input(Data, Variable, Absolute, NoWrap, Linear, PreferredState, NoNullPosition,
           //     BitField)
    0xC0,  // EndCollection()
};

namespace Furble {

HIDServer *HIDServer::hidServer = nullptr;

HIDServer::HIDServer() {
  // configure advertising as a BLE server
  m_Server = NimBLEDevice::createServer();
  m_Server->setCallbacks(this);
  m_Server->advertiseOnDisconnect(false);

  m_HID = new NimBLEHIDDevice(m_Server);
  m_Input = m_HID->inputReport(1);

  // set manufacturer name
  m_HID->manufacturer()->setValue("Maker Community");
  // set USB vendor and product ID
  m_HID->pnp(0x02, 0xe502, 0xa111, 0x0210);
  // information about HID device: device is not localized, device can be connected
  m_HID->hidInfo(0x00, 0x02);

  m_HID->reportMap((uint8_t *)hidReportDescriptor, sizeof(hidReportDescriptor));

  // advertise the services
  m_Advertising = m_Server->getAdvertising();
  m_Advertising->setAppearance(HID_GENERIC_REMOTE);
  m_Advertising->addServiceUUID(m_HID->hidService()->getUUID());
}

HIDServer::~HIDServer() {
  m_hidCallbacks = nullptr;
}

HIDServer *HIDServer::getInstance(void) {
  if (hidServer == nullptr) {
    hidServer = new HIDServer();
  }

  return hidServer;
}

void HIDServer::start(NimBLEAddress *address, HIDServerCallbacks *hidCallback) {
  m_hidCallbacks = hidCallback;
  m_HID->startServices();

  m_Server->getPeerNameOnConnect((address == nullptr) ? true : false);

  // Cannot get directed advertising working properly.
  // m_Advertising->setAdvertisementType((address == nullptr) ? BLE_GAP_CONN_MODE_UND :
  // BLE_GAP_CONN_MODE_DIR);
  m_Advertising->setAdvertisementType(BLE_GAP_CONN_MODE_UND);
  m_Advertising->start(BLE_HS_FOREVER, nullptr, address);
}

void HIDServer::stop(void) {
  m_Advertising->stop();
  m_Server->stopAdvertising();
}

void HIDServer::onAuthenticationComplete(const NimBLEConnInfo &connInfo, const std::string &name) {
  NimBLEAddress address = connInfo.getIdAddress();
  if (m_hidCallbacks != nullptr) {
    m_hidCallbacks->onComplete(address, name);
  }
}

void HIDServer::onIdentity(const NimBLEConnInfo &connInfo) {
  ESP_LOGI("HID", "identity resolved: address = %s, type = %d",
           connInfo.getIdAddress().toString().c_str(), connInfo.getIdAddress().getType());
}

NimBLECharacteristic *HIDServer::getInput(void) {
  return m_Input;
}

NimBLEConnInfo HIDServer::getConnInfo(NimBLEAddress &address) {
  return m_Server->getPeerInfo(address);
}

void HIDServer::disconnect(NimBLEAddress &address) {
  NimBLEConnInfo info = m_Server->getPeerInfo(address);
  m_Server->disconnect(info.getConnHandle());
}

bool HIDServer::isConnected(const NimBLEAddress &address) {
  NimBLEConnInfo info = m_Server->getPeerInfo(address);

  return (!info.getIdAddress().isNull());
}
}  // namespace Furble
