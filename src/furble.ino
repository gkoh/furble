#include <Furble.h>
#include <M5ez.h>
#include <NimBLEDevice.h>

#ifdef M5STICKC_PLUS
#include <M5StickCPlus.h>
#else
#include <M5StickC.h>
#endif

static Preferences preferences;

const uint32_t SCAN_DURATION = 10;

static NimBLEScan* pScan = nullptr;

static std::vector<Furble::Device *> connect_list;

class AdvertisedCallback: public NimBLEAdvertisedDeviceCallbacks {
  void onResult(NimBLEAdvertisedDevice *pDevice) {
    Furble::Device::match(pDevice, connect_list);
    ez.msgBox("Scanning",
              "Found ... " + String(connect_list.size()), "", false);
  }
};

static void remote_control(Furble::Device *device) {
  Serial.println("Remote Control");
  ez.msgBox("Remote Shutter", "Shutter Control: A\nBack: B", "", false);
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

static void menu_remote(Furble::Device *device) {
  ez.backlight.inactivity(NEVER);
  ezMenu submenu(FURBLE_STR " - Connected");
  submenu.buttons("OK#down");
  submenu.addItem("Shutter");
  submenu.addItem("Disconnect");
  submenu.downOnLast("first");

  do {
    submenu.runOnce();
    if (submenu.pickName() == "Shutter") {
      remote_control(device);
    }
  } while (submenu.pickName() != "Disconnect");

  device->disconnect();
  ez.backlight.inactivity(USER_SET);
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
  if (i == 0) return;

  Furble::Device *device = connect_list[i-1];

  NimBLEClient *pClient = NimBLEDevice::createClient();
  ezProgressBar progress_bar(FURBLE_STR, "Connecting ...", "");
  if (device->connect(pClient, progress_bar)) {
    if (save) {
      device->save();
    }
    menu_remote(device);
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
  if (i == 0) return;
  devices[i-1]->remove();
}

static void menu_settings(void) {
  ezMenu submenu(FURBLE_STR " - Settings");

  submenu.buttons("OK#down");
  submenu.addItem("Backlight", ez.backlight.menu);
  submenu.addItem("Theme", ez.theme->menu);
  submenu.addItem("Back");
  submenu.downOnLast("first");
  submenu.run();

  //int16_t i = submenu.runOnce();
  //if (i == 0) return;
}

static void mainmenu_poweroff(void) {
  m5.Axp.PowerOff();
}

void setup() {
  Serial.begin(115200);

#include <themes/default.h>
#include <themes/dark.h>
#include <themes/mono_furble.h>

  ez.begin();
  NimBLEDevice::init(FURBLE_STR);
  NimBLEDevice::setSecurityAuth(true, true, true);

  pScan = NimBLEDevice::getScan();
  pScan->setAdvertisedDeviceCallbacks(new AdvertisedCallback());
  pScan->setActiveScan(true);
  pScan->setInterval(6553);
  pScan->setWindow(6553);
}

void loop() {
  ezMenu mainmenu(FURBLE_STR);
  mainmenu.buttons("OK#down");
  mainmenu.addItem("Connect", do_saved);
  mainmenu.addItem("Scan", do_scan);
  mainmenu.addItem("Delete Saved", menu_delete);
  mainmenu.addItem("Settings", menu_settings);
  mainmenu.addItem("Power Off", mainmenu_poweroff);
  mainmenu.downOnLast("first");
  mainmenu.run();
}
