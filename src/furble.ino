#include <M5ez.h>
#include <NimBLEDevice.h>
#include <Preferences.h>

#define FURBLE_STR "furble"

static Preferences preferences;

const size_t FUJI_XT30_TOKEN_LEN = 7;
const uint8_t FUJI_XT30_ID_0 = 0xd8;
const uint8_t FUJI_XT30_ID_1 = 0x04;
const uint8_t FUJI_XT30_TYPE_TOKEN = 0x02;

const uint32_t SCAN_DURATION = 10;

#define TOKEN_LEN (4)

typedef struct _xt30_t {
  char name[64];    /** Human readabled device name. */
  uint64_t address; /** Device MAC address. */
  uint8_t type;     /** Address type. */
  uint8_t token[TOKEN_LEN]; /** Pairing token. */
} xt30_t;

static NimBLEScan* pScan = nullptr;

static std::vector<xt30_t> connect_list;

const char *FUJI_XT30_SVC_PAIR_UUID = "91f1de68-dff6-466e-8b65-ff13b0f16fb8";
const char *FUJI_XT30_CHR_PAIR_UUID = "aba356eb-9633-4e60-b73f-f52516dbd671";
const char *FUJI_XT30_CHR_IDEN_UUID = "85b9163e-62d1-49ff-a6f5-054b4630d4a1";

const char *FUJI_XT30_SVC_READ_UUID = "4e941240-d01d-46b9-a5ea-67636806830b";
const char *FUJI_XT30_CHR_READ_UUID = "bf6dc9cf-3606-4ec9-a4c8-d77576e93ea4";

const char *FUJI_XT30_SVC_CONF_UUID = "4c0020fe-f3b6-40de-acc9-77d129067b14";
const char *FUJI_XT30_CHR_IND1_UUID = "a68e3f66-0fcc-4395-8d4c-aa980b5877fa";
const char *FUJI_XT30_CHR_IND2_UUID = "bd17ba04-b76b-4892-a545-b73ba1f74dae";
const char *FUJI_XT30_CHR_NOT1_UUID = "f9150137-5d40-4801-a8dc-f7fc5b01da50";
const char *FUJI_XT30_CHR_NOT2_UUID = "ad06c7b7-f41a-46f4-a29a-712055319122";
const char *FUJI_XT30_CHR_NOT3_UUID = "049ec406-ef75-4205-a390-08fe209c51f0";

const char *FUJI_XT30_SVC_SHUTTER_UUID = "6514eb81-4e8f-458d-aa2a-e691336cdfac";
const char *FUJI_XT30_CHR_SHUTTER_UUID = "7fcf49c6-4ff0-4777-a03d-1a79166af7a8";

const uint8_t FUJI_XT30_SHUTTER_CMD[2] = {0x01, 0x00};
const uint8_t FUJI_XT30_SHUTTER_PRESS[2] = {0x02, 0x00};
const uint8_t FUJI_XT30_SHUTTER_RELEASE[2] = {0x00, 0x00};

/**
 * Add AdvertisedDevice to connection list.
 */
static void addAdvertisedDevice(NimBLEAdvertisedDevice *pDevice) {
  Serial.println(pDevice->toString().c_str());
  xt30_t x = { 0 };
  const char *data = pDevice->getManufacturerData().data();
  strcpy(&x.name[0], pDevice->getName().c_str());
  x.address = (uint64_t)pDevice->getAddress();
  x.type = pDevice->getAddressType();
  x.token[0] = data[3];
  x.token[1] = data[4];
  x.token[2] = data[5];
  x.token[3] = data[6];

  connect_list.push_back(x);
}

/**
 * Add saved connection to connection list.
 */
static void addSavedDevice(void) {
  preferences.begin(FURBLE_STR, true);
  size_t len = preferences.getBytesLength("XT30");
  if (len == sizeof(xt30_t)) {
    xt30_t x = { 0 };
    preferences.getBytes("XT30", &x, len);
    connect_list.push_back(x);
    Serial.println("Retrieve connection");
    print_connection(&x);
  }

  preferences.end();
}

/**
 * Determine if the advertised BLE device is a Fujifilm X-T30.
 */
static bool is_fuji_xt30(NimBLEAdvertisedDevice *pDevice) {
  if (pDevice->haveManufacturerData() &&
        pDevice->getManufacturerData().length() == FUJI_XT30_TOKEN_LEN) {
    const char *data = pDevice->getManufacturerData().data();
    if (data[0] == FUJI_XT30_ID_0 &&
        data[1] == FUJI_XT30_ID_1 &&
        data[2] == FUJI_XT30_TYPE_TOKEN) {
      return true;
    }
  }
  return false;
}

class ClientCallback: public NimBLEClientCallbacks {
  void onDisconnect(NimBLEClient* pClient) {
    Serial.println("Disconnected!");
  }
};

static ClientCallback clientCB;

class AdvertisedCallback: public NimBLEAdvertisedDeviceCallbacks {
  void onResult(NimBLEAdvertisedDevice *pDevice) {
    ez.msgBox("Scanning",
              "Found ... " + String(connect_list.size()), "", false);
    if (is_fuji_xt30(pDevice)) {
      addAdvertisedDevice(pDevice);
    }
  }
};

/**
 * Connect to Fujifilm X-T30.
 */
static bool connect_fuji_xt30(NimBLEClient *pClient, const xt30_t *target) {
  ezProgressBar progress_bar(FURBLE_STR, "Connecting ...", "");
  progress_bar.value(10.0f);

  NimBLERemoteService *pSvc = nullptr;
  NimBLERemoteCharacteristic *pChr = nullptr;

  Serial.println("Connecting");
  if (!pClient->connect(NimBLEAddress(target->address), target->type))
    return false;

  Serial.println("Connected");
  progress_bar.value(20.0f);
  pSvc = pClient->getService(FUJI_XT30_SVC_PAIR_UUID);
  if (pSvc == nullptr) return false;

  Serial.println("Pairing");
  pChr = pSvc->getCharacteristic(FUJI_XT30_CHR_PAIR_UUID);
  if (pChr == nullptr) return false;

  if (!pChr->canWrite()) return false;
  if (!pChr->writeValue(&target->token[0], TOKEN_LEN))
    return false;
  Serial.println("Paired!");
  progress_bar.value(30.0f);

  Serial.println("Identifying");
  pChr = pSvc->getCharacteristic(FUJI_XT30_CHR_IDEN_UUID);
  if (!pChr->canWrite()) return false;
  if (!pChr->writeValue(FURBLE_STR)) return false;
  Serial.println("Identified!");
  progress_bar.value(40.0f);

  Serial.println("Configuring");
  pSvc = pClient->getService(FUJI_XT30_SVC_CONF_UUID);
  // indications
  pSvc->getCharacteristic(FUJI_XT30_CHR_IND1_UUID)->subscribe(false);
  progress_bar.value(50.0f);
  pSvc->getCharacteristic(FUJI_XT30_CHR_IND2_UUID)->subscribe(false);
  progress_bar.value(60.0f);
  // notifications
  pSvc->getCharacteristic(FUJI_XT30_CHR_NOT1_UUID)->subscribe(true);
  progress_bar.value(70.0f);
  pSvc->getCharacteristic(FUJI_XT30_CHR_NOT2_UUID)->subscribe(true);
  progress_bar.value(80.0f);
  pSvc->getCharacteristic(FUJI_XT30_CHR_NOT3_UUID)->subscribe(true);
  progress_bar.value(90.0f);

  Serial.println("Configured");

  progress_bar.value(100.0f);

  return true;
}

void remote_control_fuji_xt30(NimBLEClient *pClient) {
  BLERemoteService *pSvc = pClient->getService(FUJI_XT30_SVC_SHUTTER_UUID);
  BLERemoteCharacteristic *pChr = pSvc->getCharacteristic(FUJI_XT30_CHR_SHUTTER_UUID);

  Serial.println("Remote Control");
  m5.Axp.SetLDO2(false);
  digitalWrite(M5_LED, LOW);
  while (true) {
    m5.update();

    if (m5.BtnA.wasPressed()) {
      // press
      pChr->writeValue(&FUJI_XT30_SHUTTER_CMD[0], sizeof(FUJI_XT30_SHUTTER_CMD), true);
      pChr->writeValue(&FUJI_XT30_SHUTTER_PRESS[0], sizeof(FUJI_XT30_SHUTTER_PRESS), true);
    }

    if (m5.BtnA.wasReleased()) {
      // release
      pChr->writeValue(&FUJI_XT30_SHUTTER_CMD[0], sizeof(FUJI_XT30_SHUTTER_CMD), true);
      pChr->writeValue(&FUJI_XT30_SHUTTER_RELEASE[0], sizeof(FUJI_XT30_SHUTTER_RELEASE), true);
    }

    if (m5.BtnB.wasPressed()) {
      break;
    }

    delay(50);
  }

  m5.Axp.SetLDO2(true);
  digitalWrite(M5_LED, HIGH);
}

static void print_connection(const xt30_t *c) {
  Serial.print("Name: ");
  Serial.println(c->name);
  Serial.print("Address: ");
  Serial.println(NimBLEAddress(c->address).toString().c_str());
  Serial.print("Type: ");
  Serial.println(c->type);
}

static void save_connection(const xt30_t *x) {

  preferences.begin(FURBLE_STR, false);
  preferences.putBytes("XT30", x, sizeof(x));
  preferences.end();

  Serial.println("Saved connection");
  print_connection(x);
}

void setup() {
  Serial.begin(115200);
  pinMode(M5_LED, OUTPUT);
  digitalWrite(M5_LED, HIGH);
  ez.begin();
  m5.Axp.ScreenBreath(200);
  NimBLEDevice::init(FURBLE_STR);

  pScan = NimBLEDevice::getScan();
  pScan->setAdvertisedDeviceCallbacks(new AdvertisedCallback());
  pScan->setActiveScan(true);
  pScan->setInterval(6553);
  pScan->setWindow(6553);
}

/**
 * Scan for devices, then present connection menu.
 */
static void do_scan(void) {
  connect_list.clear();
  pScan->clearResults();
  ez.msgBox("Scanning", "Found ... ", "", false);
  pScan->start(SCAN_DURATION, false);
  menu_connect(true);
}

/**
 * Retrieve saved devices, then present connection menu.
 */
static void do_saved(void) {
  connect_list.clear();
  addSavedDevice();
  menu_connect(false);
}

static void menu_connect(bool save) {
  ezMenu submenu(FURBLE_STR " - Connect");
  submenu.buttons("OK#down");

  for (int i = 0; i < connect_list.size(); i++) {
    xt30_t *pTarget = &connect_list[i];
    submenu.addItem(pTarget->name);
  }
  submenu.addItem("Back");
  submenu.downOnLast("first");
  submenu.runOnce();

  int16_t i = submenu.pick();
  xt30_t *pTarget = &connect_list[i-1];

  NimBLEClient *pClient = NimBLEDevice::createClient();
  pClient->setClientCallbacks(&clientCB, false);
  if (connect_fuji_xt30(pClient, pTarget)) {
    if (save) {
      save_connection(pTarget);
    }
    remote_control_fuji_xt30(pClient);
  }

  // Finished, disconnect and destroy.
  NimBLEDevice::deleteClient(pClient);
}

static void mainmenu_poweroff(void) {
  m5.Axp.PowerOff();
}

void loop() {
  ezMenu mainmenu(FURBLE_STR);
  mainmenu.buttons("OK#down");
  mainmenu.addItem("Connect", do_saved);
  mainmenu.addItem("Scan", do_scan);
  mainmenu.addItem("Power Off", mainmenu_poweroff);
  mainmenu.downOnLast("first");
  mainmenu.run();
}
