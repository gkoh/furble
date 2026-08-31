/*
 * Minimal NimBLE-cpp glue for furble/lib/furble only.
 * BLE ops route through the pak runtime bluetooth.h calls; value types and
 * advertised-device getters are backed by pak data. Scanning and the server
 * role remain aborting stubs (web grants devices through the host picker).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <ctime>
#include <array>
#include <string>
#include <vector>
extern "C" {
#include <bluetooth.h>
#include <runtime.h>
}

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

/*
 * Thread-local pak binding. module.cpp sets it around CameraList::match() and
 * Camera::connect() so createClient() can hand the granted device to the
 * NimBLEClient without touching NimBLEDevice::createClient scan paths.
 */
static thread_local struct PakBt *t_pakCtx   = nullptr;
static thread_local struct PakBtDevice *t_pakDev = nullptr;

void nimble_set_pak_device(struct PakBt *ctx, struct PakBtDevice *dev) {
    t_pakCtx = ctx;
    t_pakDev = dev;
}

/* Private statics referenced by the class layout / possible ODR from headers */
bool                       NimBLEDevice::m_synced      = false;
bool                       NimBLEDevice::m_initialized = false;
uint32_t                   NimBLEDevice::m_passkey     = 123456;
ble_gap_event_listener     NimBLEDevice::m_listener{};
uint8_t                    NimBLEDevice::m_ownAddrType = BLE_OWN_ADDR_PUBLIC;
std::vector<NimBLEAddress> NimBLEDevice::m_whiteList{};
NimBLEDeviceCallbacks      NimBLEDevice::defaultDeviceCallbacks{};
NimBLEDeviceCallbacks*     NimBLEDevice::m_pDeviceCallbacks = &NimBLEDevice::defaultDeviceCallbacks;
int NimBLEDeviceCallbacks::onStoreStatus(struct ble_store_status_event* event, void* arg) {
    (void)event;
    (void)arg;
    return 0;
}

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
    m_initialized = true;
    return true;
}

bool NimBLEDevice::setOwnAddrType(uint8_t type) {
    (void)type;
    return true;
}

void NimBLEDevice::setSecurityAuth(bool bonding, bool mitm, bool sc) {
    (void)bonding;
    (void)mitm;
    (void)sc;
}

void NimBLEDevice::setSecurityIOCap(uint8_t iocap) {
    (void)iocap;
}

void NimBLEDevice::setSecurityInitKey(uint8_t initKey) {
    (void)initKey;
}

void NimBLEDevice::setSecurityRespKey(uint8_t respKey) {
    (void)respKey;
}

bool NimBLEDevice::setPower(int8_t dbm, NimBLETxPowerType type) {
    (void)dbm;
    (void)type;
    return true;
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
    if (!t_pakCtx || !t_pakDev) {
        return nullptr;
    }
    return new NimBLEClient(NimBLEAddress(), t_pakCtx, t_pakDev);
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
    return true;
}

bool NimBLEDevice::isBonded(const NimBLEAddress& address) {
    (void)address;
    return false;
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

static ble_uuid_any_t uuid_from_string(const char *str) {
    ble_uuid_any_t u = {};
    unsigned int   first, second, third, fourth_hi;
    uint64_t       fourth_lo;
    if (!str || sscanf(str, "%8x-%4x-%4x-%4x-%12" SCNx64,
                       &first, &second, &third, &fourth_hi, &fourth_lo) != 5) {
        u.u.type      = BLE_UUID_TYPE_16;
        u.u16.value   = 0;
        return u;
    }
    u.u.type    = BLE_UUID_TYPE_128;
    uint8_t *p  = u.u128.value;
    p[0]        = (uint8_t)((fourth_lo >> 0) & 0xff);
    p[1]        = (uint8_t)((fourth_lo >> 8) & 0xff);
    p[2]        = (uint8_t)((fourth_lo >> 16) & 0xff);
    p[3]        = (uint8_t)((fourth_lo >> 24) & 0xff);
    p[4]        = (uint8_t)((fourth_lo >> 32) & 0xff);
    p[5]        = (uint8_t)((fourth_lo >> 40) & 0xff);
    p[6]        = (uint8_t)((fourth_hi >> 0) & 0xff);
    p[7]        = (uint8_t)((fourth_hi >> 8) & 0xff);
    p[8]        = (uint8_t)((third >> 0) & 0xff);
    p[9]        = (uint8_t)((third >> 8) & 0xff);
    p[10]       = (uint8_t)((second >> 0) & 0xff);
    p[11]       = (uint8_t)((second >> 8) & 0xff);
    p[12]       = (uint8_t)((first >> 0) & 0xff);
    p[13]       = (uint8_t)((first >> 8) & 0xff);
    p[14]       = (uint8_t)((first >> 16) & 0xff);
    p[15]       = (uint8_t)((first >> 24) & 0xff);
    return u;
}

NimBLEUUID::NimBLEUUID(const ble_uuid_any_t& uuid) {
    m_uuid = uuid;
}

NimBLEUUID::NimBLEUUID(const std::string& uuid) {
    m_uuid = uuid_from_string(uuid.c_str());
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

int NimBLEClient::onPakEvent(struct PakBt *ctx,
                             enum PakBtEvent event,
                             struct PakBtDevice *dev,
                             struct PakGattCharacteristic *chr,
                             void *arg) {
    (void)dev;
    auto *self = static_cast<NimBLEClient *>(arg);
    if (!self) {
        return 0;
    }
    if (event == PAK_BT_EVENT_DISCONNECTED) {
        self->m_connStatus = DISCONNECTED;
        if (self->m_pClientCallbacks) {
            self->m_pClientCallbacks->onDisconnect(self, BLE_ERR_REM_USER_CONN_TERM);
        }
        return 0;
    }
    if (event != PAK_BT_EVENT_GATT_CHAR_CHANGED || !chr || !ctx) {
        return 0;
    }
    NimBLEUUID uuid(chr->uuid);
    for (auto *svc : self->m_svcVec) {
        auto *c = svc->getCharacteristic(uuid);
        if (!c || !c->m_notifyCallback) {
            continue;
        }
        uint8_t buf[512];
        unsigned int len = pak_bt_read_characteristic_cached_value(ctx, chr, buf, sizeof(buf));
        if (len == 0) {
            continue;
        }
        c->m_notifyCallback(c, buf, len, true);
        return 0;
    }
    return 0;
}

bool NimBLEClient::connect(const NimBLEAddress& address, bool deleteAttributes, bool asyncConnect, bool exchangeMTU) {
    (void)address;
    (void)deleteAttributes;
    (void)asyncConnect;
    (void)exchangeMTU;
    if (!m_ctx || !m_dev || m_connStatus == CONNECTED) {
        return false;
    }
    if (pak_bt_device_connect(m_ctx, m_dev) != 0) {
        return false;
    }
    pak_bt_set_device_callback(m_ctx, m_dev, &NimBLEClient::onPakEvent, this);
    m_connStatus = CONNECTED;
    if (m_pClientCallbacks) {
        m_pClientCallbacks->onConnect(this);
    }
    return true;
}

bool NimBLEClient::disconnect(uint8_t reason) {
    (void)reason;
    if (!m_ctx || !m_dev) {
        return false;
    }
    pak_bt_device_disconnect(m_ctx, m_dev);
    m_connStatus = DISCONNECTED;
    return true;
}

void NimBLEClient::setSelfDelete(bool deleteOnDisconnect, bool deleteOnConnectFail) {
    (void)deleteOnDisconnect;
    (void)deleteOnConnectFail;
}

bool NimBLEClient::isConnected() const {
    return m_connStatus == CONNECTED;
}

void NimBLEClient::setClientCallbacks(NimBLEClientCallbacks* pClientCallbacks, bool deleteCallbacks) {
    (void)deleteCallbacks;
    m_pClientCallbacks = pClientCallbacks;
}

bool NimBLEClient::secureConnection(bool async) const {
    (void)async;
    return m_connStatus == CONNECTED;
}

void NimBLEClient::setConnectTimeout(uint32_t timeout) {
    (void)timeout;
}

int NimBLEClient::getRssi() const {
    return 0;
}

NimBLEConnInfo NimBLEClient::getConnInfo() const {
    /* Web Bluetooth pairing is implicit; report the link as secured so
     * camera connect paths (Ricoh) accept it. */
    ble_gap_conn_desc desc{};
    desc.conn_handle = 1;
    desc.role        = BLE_GAP_ROLE_MASTER;
    desc.sec_state.encrypted     = 1;
    desc.sec_state.authenticated = 1;
    desc.sec_state.bonded        = 1;
    desc.sec_state.key_size      = 16;
    return NimBLEConnInfo(desc);
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
}

NimBLEClient::NimBLEClient(const NimBLEAddress& peerAddress)
    : m_peerAddress(peerAddress) {
    m_pClientCallbacks = nullptr;
    m_connStatus       = DISCONNECTED;
}

NimBLEClient::NimBLEClient(const NimBLEAddress& peerAddress, struct PakBt *ctx, struct PakBtDevice *dev)
    : m_peerAddress(peerAddress), m_ctx(ctx), m_dev(dev) {
    m_pClientCallbacks = nullptr;
    m_connStatus       = DISCONNECTED;
}

NimBLEClient::~NimBLEClient() {
    for (auto *svc : m_svcVec) {
        delete svc;
    }
    m_svcVec.clear();
}

NimBLERemoteService* NimBLEClient::getService(const NimBLEUUID& uuid) {
    if (!m_ctx || !m_dev) {
        return nullptr;
    }
    for (auto *svc : m_svcVec) {
        if (svc->getUUID() == uuid) {
            return svc;
        }
    }
    for (int i = 0;; i++) {
        struct PakGattService *svc = pak_bt_get_gatt_service(m_ctx, m_dev, i);
        if (!svc) {
            break;
        }
        if (NimBLEUUID(svc->uuid) == uuid) {
            auto *ns = new NimBLERemoteService(this, m_ctx, m_dev, svc);
            m_svcVec.push_back(ns);
            return ns;
        }
        pak_bt_unref_gatt_service(m_ctx, svc);
    }
    return nullptr;
}

NimBLERemoteService* NimBLEClient::getService(const char* uuid) {
    return getService(NimBLEUUID(uuid));
}

const std::vector<NimBLERemoteService*>& NimBLEClient::getServices(bool refresh) {
    if (refresh) {
        for (auto *svc : m_svcVec) {
            delete svc;
        }
        m_svcVec.clear();
    }
    if (!m_ctx || !m_dev || !m_svcVec.empty()) {
        return m_svcVec;
    }
    for (int i = 0;; i++) {
        struct PakGattService *svc = pak_bt_get_gatt_service(m_ctx, m_dev, i);
        if (!svc) {
            break;
        }
        auto *ns = new NimBLERemoteService(this, m_ctx, m_dev, svc);
        m_svcVec.push_back(ns);
    }
    return m_svcVec;
}

std::vector<NimBLERemoteService*>::iterator NimBLEClient::begin() {
    return m_svcVec.begin();
}

std::vector<NimBLERemoteService*>::iterator NimBLEClient::end() {
    return m_svcVec.end();
}

void NimBLEClient::deleteServices() {
    for (auto *svc : m_svcVec) {
        delete svc;
    }
    m_svcVec.clear();
}

NimBLEAttValue NimBLEClient::getValue(const NimBLEUUID& serviceUUID, const NimBLEUUID& characteristicUUID) {
    NimBLERemoteService *svc = getService(serviceUUID);
    if (!svc) {
        return NimBLEAttValue();
    }
    return svc->getValue(characteristicUUID);
}

bool NimBLEClient::setValue(const NimBLEUUID&     serviceUUID,
                            const NimBLEUUID&     characteristicUUID,
                            const NimBLEAttValue& value,
                            bool                  response) {
    NimBLERemoteService *svc = getService(serviceUUID);
    if (!svc) {
        return false;
    }
    NimBLERemoteCharacteristic *chr = svc->getCharacteristic(characteristicUUID);
    if (!chr) {
        return false;
    }
    return chr->writeValue(value.data(), value.size(), response);
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

NimBLEAdvertisedDevice::NimBLEAdvertisedDevice(struct PakBt *ctx, struct PakBtDevice *dev): m_dev(dev), m_ctx(ctx) {
    if (dev) {
        uint8_t mac[BLE_DEV_ADDR_LEN] = {};
        unsigned int octets[BLE_DEV_ADDR_LEN];
        int n = sscanf(dev->mac_address, "%2x:%2x:%2x:%2x:%2x:%2x",
                       &octets[0], &octets[1], &octets[2], &octets[3], &octets[4], &octets[5]);
        if (n != BLE_DEV_ADDR_LEN) {
            n = sscanf(dev->mac_address, "%2x-%2x-%2x-%2x-%2x-%2x",
                       &octets[0], &octets[1], &octets[2], &octets[3], &octets[4], &octets[5]);
        }
        if (n == BLE_DEV_ADDR_LEN) {
            for (int i = 0; i < BLE_DEV_ADDR_LEN; i++) {
                mac[i] = (uint8_t)octets[BLE_DEV_ADDR_LEN - 1 - i];
            }
            m_address = NimBLEAddress(mac, BLE_ADDR_PUBLIC);
        }
    }
}

const NimBLEAddress& NimBLEAdvertisedDevice::getAddress() const {
    return m_address;
}

std::string NimBLEAdvertisedDevice::getManufacturerData(uint8_t index) const {
    if (!m_dev || !m_ctx) {
        return std::string();
    }
    uint8_t buf[512];
    unsigned int len = pak_bt_get_manufacturer_data(m_ctx, m_dev, index, buf, sizeof(buf));
    return std::string((const char *)buf, len);
}

uint8_t NimBLEAdvertisedDevice::getManufacturerDataCount() const {
    if (!m_dev || !m_ctx) {
        return 0;
    }
    uint8_t count = 0;
    while (count < 0xff) {
        uint8_t byte;
        if (pak_bt_get_manufacturer_data(m_ctx, m_dev, count, &byte, 1) == 0) {
            break;
        }
        count++;
    }
    return count;
}

std::string NimBLEAdvertisedDevice::getName() const {
    if (!m_dev) {
        return std::string();
    }
    return std::string(m_dev->name, strnlen(m_dev->name, sizeof(m_dev->name)));
}

int8_t NimBLEAdvertisedDevice::getRSSI() const {
    /* RSSI is not exposed by the pak runtime yet. */
    return 0;
}

NimBLEUUID NimBLEAdvertisedDevice::getServiceUUID(uint8_t index) const {
    if (!m_dev || !m_ctx) {
        return NimBLEUUID();
    }
    struct PakGattService *svc = pak_bt_get_gatt_service(m_ctx, m_dev, index);
    if (!svc) {
        return NimBLEUUID();
    }
    NimBLEUUID uuid(svc->uuid);
    pak_bt_unref_gatt_service(m_ctx, svc);
    return uuid;
}

bool NimBLEAdvertisedDevice::isAdvertisingService(const NimBLEUUID& uuid) const {
    if (!m_dev || !m_ctx) {
        return false;
    }
    struct PakGattService *svc = pak_bt_get_gatt_service_uuid(m_ctx, m_dev, uuid.toString().c_str());
    if (svc) {
        pak_bt_unref_gatt_service(m_ctx, svc);
        return true;
    }
    return false;
}

bool NimBLEAdvertisedDevice::haveManufacturerData() const {
    if (!m_dev || !m_ctx) {
        return false;
    }
    uint8_t byte;
    return pak_bt_get_manufacturer_data(m_ctx, m_dev, 0, &byte, 1) > 0;
}

bool NimBLEAdvertisedDevice::haveServiceUUID() const {
    if (!m_dev || !m_ctx) {
        return false;
    }
    struct PakGattService *svc = pak_bt_get_gatt_service(m_ctx, m_dev, 0);
    if (svc) {
        pak_bt_unref_gatt_service(m_ctx, svc);
        return true;
    }
    return false;
}

#endif

/* -------------------------------------------------------------------------- */
/* Remote GATT                                                                */
/* -------------------------------------------------------------------------- */

#if MYNEWT_VAL(BLE_ROLE_CENTRAL)

NimBLERemoteService::NimBLERemoteService(NimBLEClient* pClient,
                                         struct PakBt *ctx,
                                         struct PakBtDevice *dev,
                                         struct PakGattService *svc)
    : NimBLEAttribute(NimBLEUUID(svc->uuid), svc->handle),
      m_pClient(pClient),
      m_endHandle(svc->handle),
      m_ctx(ctx),
      m_dev(dev),
      m_svc(svc) {}

NimBLERemoteService::~NimBLERemoteService() {
    for (auto *c : m_vChars) {
        delete c;
    }
    m_vChars.clear();
    if (m_svc) {
        pak_bt_unref_gatt_service(m_ctx, m_svc);
        m_svc = nullptr;
    }
}

NimBLERemoteCharacteristic* NimBLERemoteService::getCharacteristic(const NimBLEUUID& uuid) const {
    if (!m_svc || !m_ctx) {
        return nullptr;
    }
    for (auto *c : m_vChars) {
        if (c->getUUID() == uuid) {
            return c;
        }
    }
    for (int i = 0;; i++) {
        struct PakGattCharacteristic *chr = pak_bt_get_gatt_characteristic(m_ctx, m_svc, i);
        if (!chr) {
            break;
        }
        if (NimBLEUUID(chr->uuid) == uuid) {
            auto *nc = new NimBLERemoteCharacteristic(this, m_ctx, chr);
            m_vChars.push_back(nc);
            return nc;
        }
        pak_bt_unref_gatt_characteristic(m_ctx, chr);
    }
    return nullptr;
}

NimBLERemoteCharacteristic* NimBLERemoteService::getCharacteristic(const char* uuid) const {
    return getCharacteristic(NimBLEUUID(uuid));
}

NimBLEClient* NimBLERemoteService::getClient(void) const {
    return m_pClient;
}

NimBLEAttValue NimBLERemoteService::getValue(const NimBLEUUID& characteristicUuid) const {
    NimBLERemoteCharacteristic *chr = getCharacteristic(characteristicUuid);
    if (!chr) {
        return NimBLEAttValue();
    }
    return chr->readValue();
}

bool NimBLERemoteService::setValue(const NimBLEUUID& characteristicUuid, const NimBLEAttValue& value) const {
    NimBLERemoteCharacteristic *chr = getCharacteristic(characteristicUuid);
    if (!chr) {
        return false;
    }
    return chr->writeValue(value.data(), value.size(), true);
}

NimBLERemoteCharacteristic::NimBLERemoteCharacteristic(const NimBLERemoteService* pRemoteService,
                                                       struct PakBt *ctx,
                                                       struct PakGattCharacteristic *chr)
    : NimBLERemoteValueAttribute(uuid_from_string(chr->uuid), chr->handle),
      m_pRemoteService(pRemoteService),
      m_ctx(ctx),
      m_chr(chr) {
    m_properties = (uint8_t)chr->flags;
}

NimBLERemoteCharacteristic::~NimBLERemoteCharacteristic() {
    if (m_chr) {
        pak_bt_unref_gatt_characteristic(m_ctx, m_chr);
        m_chr = nullptr;
    }
}

NimBLEClient* NimBLERemoteCharacteristic::getClient() const {
    return m_pRemoteService ? m_pRemoteService->getClient() : nullptr;
}

bool NimBLERemoteCharacteristic::canBroadcast() const {
    return m_properties & BLE_GATT_CHR_F_BROADCAST;
}

bool NimBLERemoteCharacteristic::canRead() const {
    return m_properties & BLE_GATT_CHR_F_READ;
}

bool NimBLERemoteCharacteristic::canWriteNoResponse() const {
    return m_properties & BLE_GATT_CHR_F_WRITE_NO_RSP;
}

bool NimBLERemoteCharacteristic::canWrite() const {
    return m_properties & BLE_GATT_CHR_F_WRITE;
}

bool NimBLERemoteCharacteristic::canNotify() const {
    return m_properties & BLE_GATT_CHR_F_NOTIFY;
}

bool NimBLERemoteCharacteristic::canIndicate() const {
    return m_properties & BLE_GATT_CHR_F_INDICATE;
}

bool NimBLERemoteCharacteristic::canWriteSigned() const {
    return m_properties & BLE_GATT_CHR_F_AUTH_SIGN_WRITE;
}

bool NimBLERemoteCharacteristic::hasExtendedProps() const {
    return m_properties & BLE_GATT_CHR_F_EXTENDED;
}

bool NimBLERemoteCharacteristic::subscribe(bool notifications, const notify_callback notifyCallback, bool response) const {
    (void)response;
    if (!m_chr || !m_ctx) {
        return false;
    }
    if (notifyCallback) {
        /* Pin the isNotify flag at subscribe time; the pak event callback
         * carries no notify/indicate distinction. */
        m_notifyCallback = [notifyCallback, notifications](NimBLERemoteCharacteristic *c,
                                                           uint8_t *data,
                                                           size_t length,
                                                           bool) {
            notifyCallback(c, data, length, notifications);
        };
    }
    return pak_bt_set_cccd(m_ctx, m_chr, notifications ? 1 : 2) == 0;
}

bool NimBLERemoteCharacteristic::unsubscribe(bool response) const {
    (void)response;
    if (!m_chr || !m_ctx) {
        return false;
    }
    return pak_bt_set_cccd(m_ctx, m_chr, 0) == 0;
}

NimBLEAttValue NimBLERemoteValueAttribute::readValue(time_t* timestamp) {
    const auto *chr = dynamic_cast<const NimBLERemoteCharacteristic*>(this);
    if (!chr || !chr->m_chr || !chr->m_ctx) {
        return NimBLEAttValue();
    }
    if (timestamp) {
        *timestamp = time(nullptr);
    }
    if (pak_bt_read_characteristic(chr->m_ctx, chr->m_chr, PAK_BT_BLOCK) != 0) {
        return NimBLEAttValue();
    }
    uint8_t buf[512];
    unsigned int len = pak_bt_read_characteristic_cached_value(chr->m_ctx, chr->m_chr, buf, sizeof(buf));
    return NimBLEAttValue(buf, (uint16_t)len);
}

bool NimBLERemoteValueAttribute::writeValue(const uint8_t* data, size_t length, bool response) const {
    const auto *chr = dynamic_cast<const NimBLERemoteCharacteristic*>(this);
    if (!chr || !chr->m_chr || !chr->m_ctx) {
        return false;
    }
    return pak_bt_write_characteristic(chr->m_ctx, chr->m_chr, data, (unsigned int)length,
                                       response ? PAK_BT_BLOCK : PAK_BT_NO_BLOCK) == 0;
}

#endif /* BLE_ROLE_CENTRAL */
