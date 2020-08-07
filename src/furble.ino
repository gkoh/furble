#include <Furble.h>
#include <M5ez.h>
#include <NimBLEDevice.h>
#include <Preferences.h>

static Preferences preferences;

const size_t FUJI_XT30_TOKEN_LEN = 7;
const uint8_t FUJI_XT30_ID_0 = 0xd8;
const uint8_t FUJI_XT30_ID_1 = 0x04;
const uint8_t FUJI_XT30_TYPE_TOKEN = 0x02;

const uint32_t SCAN_DURATION = 10;

static NimBLEScan* pScan = nullptr;

static std::vector<Furble::Device *> connect_list;

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

class AdvertisedCallback: public NimBLEAdvertisedDeviceCallbacks {
  void onResult(NimBLEAdvertisedDevice *pDevice) {
    ez.msgBox("Scanning",
              "Found ... " + String(connect_list.size()), "", false);
    if (is_fuji_xt30(pDevice)) {
      connect_list.push_back(new Furble::FujifilmXT30(pDevice));
    }
  }
};

void remote_control(Furble::Device *device) {
  Serial.println("Remote Control");
  m5.Axp.SetLDO2(false);
  digitalWrite(M5_LED, LOW);
  while (true) {
    m5.update();

    if (m5.BtnA.wasPressed()) {
      device->shutterPress();
    }

    if (m5.BtnA.wasReleased()) {
      device->shutterRelease();
    }

    if (m5.BtnB.wasPressed()) {
      break;
    }

    delay(50);
  }

  m5.Axp.SetLDO2(true);
  digitalWrite(M5_LED, HIGH);
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
  Furble::Device::loadDevices(connect_list);
  menu_connect(false);
}

static void menu_connect(bool save) {
  ezMenu submenu(FURBLE_STR " - Connect");
  submenu.buttons("OK#down");

  for (int i = 0; i < connect_list.size(); i++) {
    submenu.addItem(connect_list[i]->getName());
  }
  submenu.addItem("Back");
  submenu.downOnLast("first");
  int16_t i = submenu.runOnce();

  Furble::Device *device = connect_list[i-1];

  NimBLEClient *pClient = NimBLEDevice::createClient();
  ezProgressBar progress_bar(FURBLE_STR, "Connecting ...", "");
  if (device->connect(pClient, progress_bar)) {
    if (save) {
      device->save();
    }
    remote_control(device);
  }
}

static void menu_delete() {
  std::vector<Furble::Device *> devices;
  ezMenu submenu(FURBLE_STR " - Delete");
  submenu.buttons("OK#down");
  Furble::Device::loadDevices(devices);

  for (size_t i = 0; i < devices.size(); i++) {
    submenu.addItem(devices[i]->getName());
  }
  submenu.addItem("Back");
  submenu.downOnLast("first");

  int16_t i = submenu.runOnce();
  Furble::Device *device = connect_list[i-1];
  device->remove();
}

static void mainmenu_poweroff(void) {
  m5.Axp.PowerOff();
}

void loop() {
  ezMenu mainmenu(FURBLE_STR);
  mainmenu.buttons("OK#down");
  mainmenu.addItem("Connect", do_saved);
  mainmenu.addItem("Scan", do_scan);
  mainmenu.addItem("Delete Saved", menu_delete);
  mainmenu.addItem("Power Off", mainmenu_poweroff);
  mainmenu.downOnLast("first");
  mainmenu.run();
}
