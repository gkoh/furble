/*
 * Minimal NimBLE-cpp glue for furble/lib/furble only.
 * BLE ops: fprintf TODO + abort. Value types: libc implementations.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <array>
#include <string>
#include <vector>

#include "NimBLEDevice.h"
#include "NimBLEClient.h"
#include "NimBLEScan.h"
#include "NimBLEServer.h"
#include "NimBLEAddress.h"
#include "NimBLEUUID.h"
#include "NimBLEAdvertisedDevice.h"
#include "NimBLERemoteService.h"
#include "NimBLERemoteCharacteristic.h"
#include "NimBLERemoteValueAttribute.h"
#include "NimBLEAttValue.h"
#include "NimBLEUtils.h"
#include "NimBLEConnInfo.h"

#define STUB()                                              \
    do {                                                    \
        fprintf(stderr, "TODO: %s\n", __PRETTY_FUNCTION__); \
        abort();                                            \
    } while (0)

/* -------------------------------------------------------------------------- */
/* NimBLEDevice — only symbols used by furble                                 */
/* -------------------------------------------------------------------------- */

/* Private statics referenced by the class layout / possible ODR from headers */
bool                       NimBLEDevice::m_synced      = false;
bool                       NimBLEDevice::m_initialized = false;
uint32_t                   NimBLEDevice::m_passkey     = 123456;
ble_gap_event_listener     NimBLEDevice::m_listener{};
uint8_t                    NimBLEDevice::m_ownAddrType = BLE_OWN_ADDR_PUBLIC;
std::vector<NimBLEAddress> NimBLEDevice::m_whiteList{};
NimBLEDeviceCallbacks      NimBLEDevice::defaultDeviceCallbacks{};
NimBLEDeviceCallbacks*     NimBLEDevice::m_pDeviceCallbacks = &NimBLEDevice::defaultDeviceCallbacks;

#if MYNEWT_VAL(BLE_ROLE_OBSERVER)
NimBLEScan* NimBLEDevice::m_pScan = nullptr;
#endif

#if MYNEWT_VAL(BLE_ROLE_PERIPHERAL)
NimBLEServer* NimBLEDevice::m_pServer = nullptr;
#endif

#if MYNEWT_VAL(BLE_ROLE_CENTRAL)
std::array<NimBLEClient*, MYNEWT_VAL(BLE_MAX_CONNECTIONS)> NimBLEDevice::m_pClients{};
#endif

bool NimBLEDevice::init(const std::string& deviceName) {
    (void)deviceName;
    STUB();
}

bool NimBLEDevice::setOwnAddrType(uint8_t type) {
    (void)type;
    STUB();
}

void NimBLEDevice::setSecurityAuth(bool bonding, bool mitm, bool sc) {
    (void)bonding;
    (void)mitm;
    (void)sc;
    STUB();
}

void NimBLEDevice::setSecurityIOCap(uint8_t iocap) {
    (void)iocap;
    STUB();
}

void NimBLEDevice::setSecurityInitKey(uint8_t initKey) {
    (void)initKey;
    STUB();
}

void NimBLEDevice::setSecurityRespKey(uint8_t respKey) {
    (void)respKey;
    STUB();
}

bool NimBLEDevice::setPower(int8_t dbm, NimBLETxPowerType type) {
    (void)dbm;
    (void)type;
    STUB();
}

#if MYNEWT_VAL(BLE_ROLE_OBSERVER)
NimBLEScan* NimBLEDevice::getScan() {
    STUB();
}
#endif

#if MYNEWT_VAL(BLE_ROLE_PERIPHERAL)
NimBLEServer* NimBLEDevice::createServer() {
    STUB();
}
#endif

#if MYNEWT_VAL(BLE_ROLE_CENTRAL)
NimBLEClient* NimBLEDevice::createClient() {
    STUB();
}
#endif

#if MYNEWT_VAL(BLE_ROLE_PERIPHERAL) || MYNEWT_VAL(BLE_ROLE_CENTRAL)
bool NimBLEDevice::injectConfirmPasskey(const NimBLEConnInfo& peerInfo, bool accept) {
    (void)peerInfo;
    (void)accept;
    STUB();
}

bool NimBLEDevice::injectPassKey(const NimBLEConnInfo& peerInfo, uint32_t pin) {
    (void)peerInfo;
    (void)pin;
    STUB();
}
#endif

#if MYNEWT_VAL(BLE_ROLE_CENTRAL) || MYNEWT_VAL(BLE_ROLE_PERIPHERAL)
bool NimBLEDevice::deleteBond(const NimBLEAddress& address) {
    (void)address;
    STUB();
}

bool NimBLEDevice::isBonded(const NimBLEAddress& address) {
    (void)address;
    STUB();
}
#endif

/* -------------------------------------------------------------------------- */
/* NimBLEUtils                                                                */
/* -------------------------------------------------------------------------- */

std::string NimBLEUtils::dataToHexString(const uint8_t* source, uint8_t length) {
    static const char hex[] = "0123456789abcdef";
    size_t            n     = (size_t)length * 2;
    char*             buf   = (char*)malloc(n + 1);
    if (!buf) {
        return std::string();
    }
    for (uint8_t i = 0; i < length; i++) {
        buf[(size_t)i * 2]     = hex[(source[i] >> 4) & 0x0f];
        buf[(size_t)i * 2 + 1] = hex[source[i] & 0x0f];
    }
    buf[n] = '\0';
    std::string out(buf, n);
    free(buf);
    return out;
}

/* -------------------------------------------------------------------------- */
/* NimBLEAddress                                                              */
/* -------------------------------------------------------------------------- */

NimBLEAddress::NimBLEAddress(ble_addr_t address) : ble_addr_t{address} {}

NimBLEAddress::NimBLEAddress(const uint8_t address[BLE_DEV_ADDR_LEN], uint8_t type) {
    this->type = type;
    if (address) {
        memcpy(this->val, address, BLE_DEV_ADDR_LEN);
    } else {
        memset(this->val, 0, BLE_DEV_ADDR_LEN);
    }
}

NimBLEAddress::NimBLEAddress(const uint64_t& address, uint8_t type) {
    this->type = type;
    uint64_t a = address;
    for (int i = 0; i < BLE_DEV_ADDR_LEN; i++) {
        this->val[i] = (uint8_t)(a & 0xff);
        a >>= 8;
    }
}

uint8_t NimBLEAddress::getType() const {
    return this->type;
}

std::string NimBLEAddress::toString() const {
    char buf[18];
    snprintf(buf,
             sizeof(buf),
             "%02x:%02x:%02x:%02x:%02x:%02x",
             this->val[5],
             this->val[4],
             this->val[3],
             this->val[2],
             this->val[1],
             this->val[0]);
    return std::string(buf);
}

bool NimBLEAddress::operator==(const NimBLEAddress& rhs) const {
    return this->type == rhs.type && memcmp(this->val, rhs.val, BLE_DEV_ADDR_LEN) == 0;
}

NimBLEAddress::operator uint64_t() const {
    uint64_t a = 0;
    for (int i = BLE_DEV_ADDR_LEN - 1; i >= 0; i--) {
        a = (a << 8) | this->val[i];
    }
    return a;
}

/* -------------------------------------------------------------------------- */
/* NimBLEUUID                                                                 */
/* -------------------------------------------------------------------------- */

NimBLEUUID::NimBLEUUID(uint16_t uuid) {
    m_uuid.u16.u.type = BLE_UUID_TYPE_16;
    m_uuid.u16.value  = uuid;
}

NimBLEUUID::NimBLEUUID(uint32_t first, uint16_t second, uint16_t third, uint64_t fourth) {
    m_uuid.u128.u.type = BLE_UUID_TYPE_128;
    uint8_t* p         = m_uuid.u128.value;
    p[0]               = (uint8_t)((fourth >> 0) & 0xff);
    p[1]               = (uint8_t)((fourth >> 8) & 0xff);
    p[2]               = (uint8_t)((fourth >> 16) & 0xff);
    p[3]               = (uint8_t)((fourth >> 24) & 0xff);
    p[4]               = (uint8_t)((fourth >> 32) & 0xff);
    p[5]               = (uint8_t)((fourth >> 40) & 0xff);
    p[6]               = (uint8_t)((fourth >> 48) & 0xff);
    p[7]               = (uint8_t)((fourth >> 56) & 0xff);
    p[8]               = (uint8_t)((third >> 0) & 0xff);
    p[9]               = (uint8_t)((third >> 8) & 0xff);
    p[10]              = (uint8_t)((second >> 0) & 0xff);
    p[11]              = (uint8_t)((second >> 8) & 0xff);
    p[12]              = (uint8_t)((first >> 0) & 0xff);
    p[13]              = (uint8_t)((first >> 8) & 0xff);
    p[14]              = (uint8_t)((first >> 16) & 0xff);
    p[15]              = (uint8_t)((first >> 24) & 0xff);
}

std::string NimBLEUUID::toString() const {
    char buf[40];
    if (m_uuid.u.type == BLE_UUID_TYPE_16) {
        snprintf(buf, sizeof(buf), "0x%04x", m_uuid.u16.value);
        return std::string(buf);
    }
    if (m_uuid.u.type == BLE_UUID_TYPE_32) {
        snprintf(buf, sizeof(buf), "0x%08x", m_uuid.u32.value);
        return std::string(buf);
    }
    const uint8_t* v = m_uuid.u128.value;
    snprintf(buf,
             sizeof(buf),
             "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
             v[15],
             v[14],
             v[13],
             v[12],
             v[11],
             v[10],
             v[9],
             v[8],
             v[7],
             v[6],
             v[5],
             v[4],
             v[3],
             v[2],
             v[1],
             v[0]);
    return std::string(buf);
}

bool NimBLEUUID::operator==(const NimBLEUUID& rhs) const {
    if (m_uuid.u.type != rhs.m_uuid.u.type) {
        return false;
    }
    if (m_uuid.u.type == BLE_UUID_TYPE_16) {
        return m_uuid.u16.value == rhs.m_uuid.u16.value;
    }
    if (m_uuid.u.type == BLE_UUID_TYPE_32) {
        return m_uuid.u32.value == rhs.m_uuid.u32.value;
    }
    if (m_uuid.u.type == BLE_UUID_TYPE_128) {
        return memcmp(m_uuid.u128.value, rhs.m_uuid.u128.value, 16) == 0;
    }
    return true;
}

/* -------------------------------------------------------------------------- */
/* NimBLEAttValue                                                             */
/* -------------------------------------------------------------------------- */

NimBLEAttValue::NimBLEAttValue(uint16_t init_len, uint16_t max_len)
    : m_attr_value(nullptr), m_attr_max_len(max_len), m_attr_len(0), m_capacity(init_len) {
    if (init_len) {
        m_attr_value = (uint8_t*)calloc(1, init_len);
        if (!m_attr_value) {
            m_capacity = 0;
        }
    }
}

NimBLEAttValue::NimBLEAttValue(const uint8_t* value, uint16_t len, uint16_t max_len) : NimBLEAttValue(len, max_len) {
    if (value && len) {
        setValue(value, len);
    }
}

NimBLEAttValue::~NimBLEAttValue() {
    free(m_attr_value);
    m_attr_value = nullptr;
}

void NimBLEAttValue::deepCopy(const NimBLEAttValue& source) {
    m_attr_max_len = source.m_attr_max_len;
    m_attr_len     = 0;
    m_capacity     = 0;
    m_attr_value   = nullptr;
    if (source.m_attr_len && source.m_attr_value) {
        m_capacity   = source.m_attr_len;
        m_attr_value = (uint8_t*)malloc(m_capacity);
        if (m_attr_value) {
            memcpy(m_attr_value, source.m_attr_value, source.m_attr_len);
            m_attr_len = source.m_attr_len;
        }
    }
}

bool NimBLEAttValue::setValue(const uint8_t* value, uint16_t len) {
    if (len > m_attr_max_len) {
        return false;
    }
    if (len > m_capacity) {
        uint8_t* n = (uint8_t*)realloc(m_attr_value, len ? len : 1);
        if (!n) {
            return false;
        }
        m_attr_value = n;
        m_capacity   = len;
    }
    if (value && len) {
        memcpy(m_attr_value, value, len);
    }
    m_attr_len = len;
    return true;
}

uint8_t NimBLEAttValue::operator[](int pos) const {
    if (!m_attr_value || pos < 0 || (uint16_t)pos >= m_attr_len) {
        return 0;
    }
    return m_attr_value[pos];
}

NimBLEAttValue& NimBLEAttValue::operator=(NimBLEAttValue&& source) {
    if (this != &source) {
        free(m_attr_value);
        m_attr_value        = source.m_attr_value;
        m_attr_max_len      = source.m_attr_max_len;
        m_attr_len          = source.m_attr_len;
        m_capacity          = source.m_capacity;
        source.m_attr_value = nullptr;
        source.m_attr_len   = 0;
        source.m_capacity   = 0;
    }
    return *this;
}

NimBLEAttValue& NimBLEAttValue::operator=(const NimBLEAttValue& source) {
    if (this != &source) {
        free(m_attr_value);
        m_attr_value = nullptr;
        deepCopy(source);
    }
    return *this;
}

NimBLEAttValue& NimBLEAttValue::append(const uint8_t* value, uint16_t len) {
    if (!value || !len) {
        return *this;
    }
    uint16_t newLen = (uint16_t)(m_attr_len + len);
    if (newLen > m_attr_max_len) {
        return *this;
    }
    if (newLen > m_capacity) {
        uint8_t* n = (uint8_t*)realloc(m_attr_value, newLen);
        if (!n) {
            return *this;
        }
        m_attr_value = n;
        m_capacity   = newLen;
    }
    memcpy(m_attr_value + m_attr_len, value, len);
    m_attr_len = newLen;
    return *this;
}

/* -------------------------------------------------------------------------- */
/* NimBLEClient                                                               */
/* -------------------------------------------------------------------------- */

#if MYNEWT_VAL(BLE_ROLE_CENTRAL)

bool NimBLEClient::connect(const NimBLEAddress& address, bool deleteAttributes, bool asyncConnect, bool exchangeMTU) {
    (void)address;
    (void)deleteAttributes;
    (void)asyncConnect;
    (void)exchangeMTU;
    STUB();
}

bool NimBLEClient::disconnect(uint8_t reason) {
    (void)reason;
    STUB();
}

void NimBLEClient::setSelfDelete(bool deleteOnDisconnect, bool deleteOnConnectFail) {
    (void)deleteOnDisconnect;
    (void)deleteOnConnectFail;
    STUB();
}

bool NimBLEClient::isConnected() const {
    STUB();
}

void NimBLEClient::setClientCallbacks(NimBLEClientCallbacks* pClientCallbacks, bool deleteCallbacks) {
    (void)pClientCallbacks;
    (void)deleteCallbacks;
    STUB();
}

bool NimBLEClient::secureConnection(bool async) const {
    (void)async;
    STUB();
}

void NimBLEClient::setConnectTimeout(uint32_t timeout) {
    (void)timeout;
    STUB();
}

NimBLEConnInfo NimBLEClient::getConnInfo() const {
    STUB();
}

void NimBLEClient::setConnectionParams(uint16_t minInterval,
                                       uint16_t maxInterval,
                                       uint16_t latency,
                                       uint16_t timeout,
                                       uint16_t scanInterval,
                                       uint16_t scanWindow) {
    (void)minInterval;
    (void)maxInterval;
    (void)latency;
    (void)timeout;
    (void)scanInterval;
    (void)scanWindow;
    STUB();
}

NimBLERemoteService* NimBLEClient::getService(const NimBLEUUID& uuid) {
    (void)uuid;
    STUB();
}

NimBLEAttValue NimBLEClient::getValue(const NimBLEUUID& serviceUUID, const NimBLEUUID& characteristicUUID) {
    (void)serviceUUID;
    (void)characteristicUUID;
    STUB();
}

bool NimBLEClient::setValue(const NimBLEUUID&     serviceUUID,
                            const NimBLEUUID&     characteristicUUID,
                            const NimBLEAttValue& value,
                            bool                  response) {
    (void)serviceUUID;
    (void)characteristicUUID;
    (void)value;
    (void)response;
    STUB();
}

/* Default callback bodies (vtable for Camera / Ricoh subclasses) */
void NimBLEClientCallbacks::onConnect(NimBLEClient* pClient) {
    (void)pClient;
}

void NimBLEClientCallbacks::onConnectFail(NimBLEClient* pClient, int reason) {
    (void)pClient;
    (void)reason;
}

void NimBLEClientCallbacks::onDisconnect(NimBLEClient* pClient, int reason) {
    (void)pClient;
    (void)reason;
}

bool NimBLEClientCallbacks::onConnParamsUpdateRequest(NimBLEClient* pClient, const ble_gap_upd_params* params) {
    (void)pClient;
    (void)params;
    return true;
}

void NimBLEClientCallbacks::onPassKeyEntry(NimBLEConnInfo& connInfo) {
    (void)connInfo;
}

uint32_t NimBLEClientCallbacks::onPassKeyDisplay(NimBLEConnInfo& connInfo) {
    (void)connInfo;
    return 123456;
}

void NimBLEClientCallbacks::onAuthenticationComplete(NimBLEConnInfo& connInfo) {
    (void)connInfo;
}

void NimBLEClientCallbacks::onConfirmPasskey(NimBLEConnInfo& connInfo, uint32_t pin) {
    (void)connInfo;
    (void)pin;
}

void NimBLEClientCallbacks::onIdentity(NimBLEConnInfo& connInfo) {
    (void)connInfo;
}

void NimBLEClientCallbacks::onMTUChange(NimBLEClient* pClient, uint16_t MTU) {
    (void)pClient;
    (void)MTU;
}

void NimBLEClientCallbacks::onPhyUpdate(NimBLEClient* pClient, uint8_t txPhy, uint8_t rxPhy) {
    (void)pClient;
    (void)txPhy;
    (void)rxPhy;
}

#endif /* BLE_ROLE_CENTRAL */

/* -------------------------------------------------------------------------- */
/* NimBLEScan                                                                 */
/* -------------------------------------------------------------------------- */

#if MYNEWT_VAL(BLE_ROLE_OBSERVER)

bool NimBLEScan::start(uint32_t duration, bool isContinue, bool restart) {
    (void)duration;
    (void)isContinue;
    (void)restart;
    STUB();
}

bool NimBLEScan::isScanning() {
    STUB();
}

void NimBLEScan::setScanCallbacks(NimBLEScanCallbacks* pScanCallbacks, bool wantDuplicates) {
    (void)pScanCallbacks;
    (void)wantDuplicates;
    STUB();
}

void NimBLEScan::setActiveScan(bool active) {
    (void)active;
    STUB();
}

void NimBLEScan::setInterval(uint16_t intervalMs) {
    (void)intervalMs;
    STUB();
}

void NimBLEScan::setWindow(uint16_t windowMs) {
    (void)windowMs;
    STUB();
}

bool NimBLEScan::stop() {
    STUB();
}

void NimBLEScan::clearResults() {
    STUB();
}

void NimBLEScanCallbacks::onDiscovered(const NimBLEAdvertisedDevice* advertisedDevice) {
    (void)advertisedDevice;
}

void NimBLEScanCallbacks::onResult(const NimBLEAdvertisedDevice* advertisedDevice) {
    (void)advertisedDevice;
}

void NimBLEScanCallbacks::onScanEnd(const NimBLEScanResults& scanResults, int reason) {
    (void)scanResults;
    (void)reason;
}

#endif /* BLE_ROLE_OBSERVER */

/* -------------------------------------------------------------------------- */
/* NimBLEServer                                                               */
/* -------------------------------------------------------------------------- */

#if MYNEWT_VAL(BLE_ROLE_PERIPHERAL)

bool NimBLEServer::start() {
    STUB();
}

#endif

/* -------------------------------------------------------------------------- */
/* NimBLEAdvertisedDevice                                                     */
/* -------------------------------------------------------------------------- */

#if MYNEWT_VAL(BLE_ROLE_OBSERVER)

const NimBLEAddress& NimBLEAdvertisedDevice::getAddress() const {
    STUB();
}

std::string NimBLEAdvertisedDevice::getManufacturerData(uint8_t index) const {
    (void)index;
    STUB();
}

std::string NimBLEAdvertisedDevice::getName() const {
    STUB();
}

int8_t NimBLEAdvertisedDevice::getRSSI() const {
    STUB();
}

NimBLEUUID NimBLEAdvertisedDevice::getServiceUUID(uint8_t index) const {
    (void)index;
    STUB();
}

bool NimBLEAdvertisedDevice::isAdvertisingService(const NimBLEUUID& uuid) const {
    (void)uuid;
    STUB();
}

bool NimBLEAdvertisedDevice::haveManufacturerData() const {
    STUB();
}

bool NimBLEAdvertisedDevice::haveServiceUUID() const {
    STUB();
}

#endif

/* -------------------------------------------------------------------------- */
/* Remote GATT                                                                */
/* -------------------------------------------------------------------------- */

#if MYNEWT_VAL(BLE_ROLE_CENTRAL)

NimBLERemoteCharacteristic* NimBLERemoteService::getCharacteristic(const NimBLEUUID& uuid) const {
    (void)uuid;
    STUB();
}

NimBLEAttValue NimBLERemoteService::getValue(const NimBLEUUID& characteristicUuid) const {
    (void)characteristicUuid;
    STUB();
}

bool NimBLERemoteService::setValue(const NimBLEUUID& characteristicUuid, const NimBLEAttValue& value) const {
    (void)characteristicUuid;
    (void)value;
    STUB();
}

bool NimBLERemoteCharacteristic::canRead() const {
    STUB();
}

bool NimBLERemoteCharacteristic::canWriteNoResponse() const {
    STUB();
}

bool NimBLERemoteCharacteristic::canWrite() const {
    STUB();
}

bool NimBLERemoteCharacteristic::canNotify() const {
    STUB();
}

bool NimBLERemoteCharacteristic::canIndicate() const {
    STUB();
}

bool NimBLERemoteCharacteristic::subscribe(bool notifications, const notify_callback notifyCallback, bool response) const {
    (void)notifications;
    (void)notifyCallback;
    (void)response;
    STUB();
}

NimBLEAttValue NimBLERemoteValueAttribute::readValue(time_t* timestamp) {
    (void)timestamp;
    STUB();
}

bool NimBLERemoteValueAttribute::writeValue(const uint8_t* data, size_t length, bool response) const {
    (void)data;
    (void)length;
    (void)response;
    STUB();
}

#endif /* BLE_ROLE_CENTRAL */
