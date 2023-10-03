# 1 "/var/folders/ls/rv_m501s27303fw6jh8dv9_00000gn/T/tmpjjjlrl8q"
#include <Arduino.h>
# 1 "/Users/sbstjn/Workspace/src/sbstjn/furble/src/furble.ino"
#include <Furble.h>
#include <M5ez.h>
#include <NimBLEDevice.h>

#ifdef M5STICKC_PLUS
#include <M5StickCPlus.h>
#endif

#ifdef M5STICKC
#include <M5StickC.h>
#endif

#ifdef M5STACK_CORE2
#include <M5Core2.h>
#endif

const uint32_t SCAN_DURATION = 10;

static NimBLEScan *pScan = nullptr;

static std::vector<Furble::Device *> connect_list;

class AdvertisedCallback: public NimBLEAdvertisedDeviceCallbacks {
  void onResult(NimBLEAdvertisedDevice *pDevice) {
    Furble::Device::match(pDevice, connect_list);
    ez.msgBox("Scanning", "Found ... " + String(connect_list.size()), "", false);
  }
};
static void about(void);
static void do_scan(void);
static void do_saved(void);
static void menu_connect(bool save);
static void menu_delete();
static void menu_settings(void);
static void mainmenu_poweroff(void);
void setup();
void loop();
static void save_tx_power(uint8_t tx_power);
static uint8_t load_tx_power();
esp_power_level_t settings_load_esp_tx_power();
void settings_menu_tx_power(void);
#line 33 "/Users/sbstjn/Workspace/src/sbstjn/furble/src/furble.ino"
static void about(void) {
  String version = FURBLE_VERSION;
  if (version.length() < 1) {
    version = "unknown";
  }

  ez.msgBox(FURBLE_STR " - About", "Version: " + version, "Back", true);
}

static void trigger(Furble::Device *device, int counter) {
  device->focusPress();
  delay(250);
  device->shutterPress();
  delay(50);

  ez.msgBox("Interval Release", String(counter), "Stop", false);
  device->shutterRelease();
}

static void remote_interval(Furble::Device *device) {
  int i = 0;
  int j = 1;

  ez.msgBox("Interval Release", "", "Stop", false);
  trigger(device, j);

  while (true) {
    i++;

    m5.update();

    if (m5.BtnB.wasReleased()) {
      break;
    }

    if (i > 50) {
      i = 0;
      j++;

      trigger(device, j);
    }

    delay(50);
  }
}

static void remote_control(Furble::Device *device) {
  Serial.println("Remote Control");

#ifdef M5STACK_CORE2
  ez.msgBox("Remote Shutter", "", "Release#Focus#Back", false);
#else
  ez.msgBox("Remote Shutter", "Back: Power", "Release#Focus", false);
#endif
  while (true) {
    m5.update();

#ifdef M5STACK_CORE2
    if (m5.BtnC.wasPressed()) {
      break;
    }
#else

    if (m5.Axp.GetBtnPress() == 0x02) {
      break;
    }
#endif

    if (m5.BtnA.wasPressed()) {
      device->shutterPress();
    }

    if (m5.BtnA.wasReleased()) {
      device->shutterRelease();
    }

    if (m5.BtnB.wasPressed()) {
      device->focusPress();
    }

    if (m5.BtnB.wasReleased()) {
      device->focusRelease();
    }

    delay(50);
  }
}




static void do_scan(void) {
  connect_list.clear();
  pScan->clearResults();
  ez.msgBox("Scanning", "Found ... ", "", false);
  pScan->start(SCAN_DURATION, false);
  menu_connect(true);
}




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
  submenu.addItem("Interval");
  submenu.addItem("Disconnect");
  submenu.downOnLast("first");

  do {
    submenu.runOnce();

    if (submenu.pickName() == "Shutter") {
      remote_control(device);
    }

    if (submenu.pickName() == "Interval") {
      remote_interval(device);
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
  if (i == 0)
    return;

  Furble::Device *device = connect_list[i - 1];

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
  if (i == 0)
    return;
  devices[i - 1]->remove();
}

static void menu_settings(void) {
  ezMenu submenu(FURBLE_STR " - Settings");

  submenu.buttons("OK#down");
  submenu.addItem("Backlight", ez.backlight.menu);
  submenu.addItem("Theme", ez.theme->menu);
  submenu.addItem("Transmit Power", settings_menu_tx_power);
  submenu.addItem("About", about);
  submenu.addItem("Back");
  submenu.downOnLast("first");
  submenu.run();
}

static void mainmenu_poweroff(void) {
  m5.Axp.PowerOff();
}

void setup() {
  Serial.begin(115200);

#include <themes/dark.h>
#include <themes/default.h>
#include <themes/mono_furble.h>

  ez.begin();

#ifdef M5STACK_CORE2
  m5.lcd.setRotation(1);
#endif

  NimBLEDevice::init(FURBLE_STR);
  NimBLEDevice::setSecurityAuth(true, true, true);


  esp_power_level_t esp_power = settings_load_esp_tx_power();
  NimBLEDevice::setPower(esp_power);

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
# 1 "/Users/sbstjn/Workspace/src/sbstjn/furble/src/settings.ino"
const char *PREFS_TX_POWER = "txpower";

static Preferences prefs;




static void save_tx_power(uint8_t tx_power) {
  prefs.begin(FURBLE_STR, false);
  prefs.putUChar(PREFS_TX_POWER, tx_power);
  prefs.end();
}




static uint8_t load_tx_power() {
  prefs.begin(FURBLE_STR, true);
  uint8_t power = prefs.getUChar(PREFS_TX_POWER, 1);
  prefs.end();

  return power;
}




esp_power_level_t settings_load_esp_tx_power() {
  uint8_t power = load_tx_power();
  switch (power) {
    case 0:
      return ESP_PWR_LVL_P3;
    case 1:
      return ESP_PWR_LVL_P6;
    case 2:
      return ESP_PWR_LVL_P9;
    default:
      return ESP_PWR_LVL_P3;
  }
  return ESP_PWR_LVL_P3;
}






void settings_menu_tx_power(void) {
  uint8_t power = load_tx_power();
  ezProgressBar power_bar(FURBLE_STR, "Set transmit power", "Adjust#Back");
  power_bar.value(power / 0.03f);
  while (true) {
    String b = ez.buttons.poll();
    if (b == "Adjust") {
      power++;
      if (power > 3) {
        power = 1;
      }
      power_bar.value(power / 0.03f);
    }
    if (b == "Back") {
      break;
    }
  }

  save_tx_power(power);
}