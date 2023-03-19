#include <Furble.h>
#include <M5ez.h>
#include <NimBLEDevice.h>
#include <TinyGPS++.h>

#ifdef M5STICKC_PLUS
#include <M5StickCPlus.h>
#else
#include <M5StickC.h>
#endif

const uint32_t SCAN_DURATION = 10;

static NimBLEScan *pScan = nullptr;

static std::vector<Furble::Device *> connect_list;

static TinyGPSPlus gps;
HardwareSerial GroveSerial(2);
static const uint32_t GPS_BAUD = 9600;
static const uint16_t GPS_SERVICE_MS = 250;
static const uint16_t GPS_NO_FIX_LED_MS = 500;
static const uint16_t GPS_FIX_LED_MS = 200;
static const uint32_t GPS_MAX_AGE = 60 * 1000;

static bool gps_has_fix = false;

/**
 * BLE Advertisement callback.
 */
class AdvertisedCallback: public NimBLEAdvertisedDeviceCallbacks {
  void onResult(NimBLEAdvertisedDevice *pDevice) {
    Furble::Device::match(pDevice, connect_list);
    ez.msgBox("Scanning", "Found ... " + String(connect_list.size()), "", false);
  }
};

/**
 * GPS serial event service handler.
 */
static uint16_t service_grove_gps(void) {
  while (Serial2.available() > 0) {
    gps.encode(Serial2.read());
  }

  if ((gps.location.age() < GPS_MAX_AGE) && gps.location.isValid() &&
    (gps.date.age() < GPS_MAX_AGE) && gps.date.isValid() &&
    (gps.time.age() < GPS_MAX_AGE) && gps.time.age()) {
    gps_has_fix = true;
  }
  else {
    gps_has_fix = false;
  }

  return GPS_SERVICE_MS;
}

/**
 * GPS fix LED flash.
 */
static uint16_t service_gps_fix_led(void) {
  static bool flash = false;

  flash = !flash;
  digitalWrite(M5_LED, flash ? HIGH : LOW);

  return (gps_has_fix ? GPS_FIX_LED_MS : GPS_NO_FIX_LED_MS);
}

/**
 * Update geotag data.
 */
static void update_geodata(Furble::Device *device) {
  if (gps.location.isUpdated() && gps.location.isValid() && gps.date.isUpdated()
    && gps.date.isValid() && gps.time.isValid() && gps.time.isValid()) {
    Furble::Device::gps_t dgps = { gps.location.lat(),
      gps.location.lng(),
      gps.altitude.meters() };
    Furble::Device::timesync_t timesync = { gps.date.year(), gps.date.month(),
    gps.date.day(), gps.time.hour(), gps.time.minute(), gps.time.second() };

    device->updateGeoData(dgps, timesync);
  }
}

/**
 * Display the version.
 */
static void about(void) {
  String version = FURBLE_VERSION;
  if (version.length() < 1) {
    version = "unknown";
  }

  ez.msgBox(FURBLE_STR " - About", "Version: " + version, "Back", true);
}

static void remote_control(Furble::Device *device) {
  Serial.println("Remote Control");
  ez.msgBox("Remote Shutter", "Shutter Control: A\nFocus: B\nBack: Power", "", false);
  while (true) {
    m5.update();

    update_geodata(device);

    // Source code in AXP192 says 0x02 is short press.
    if (m5.Axp.GetBtnPress() == 0x02) {
      break;
    }

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

    ez.yield();
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
  if (i == 0)
    return;

  Furble::Device *device = connect_list[i - 1];

  update_geodata(device);

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

static void gps_info(void) {
  Serial.println("GPS Information");
  char buffer[256] = { 0x0 };

  while (true) {
    snprintf(buffer, 256, "%s (%d) \n%.2f, %.2f\n%.2f metres\n%4u-%02u-%02u %02u:%02u:%02u\n",
    gps.location.isValid() && gps.date.isValid() && gps.time.isValid() ? "Valid" : "Invalid",
    gps.location.age(),
    gps.location.lat(),
    gps.location.lng(),
    gps.altitude.meters(),
    gps.date.year(),
    gps.date.month(),
    gps.date.day(),
    gps.time.hour(),
    gps.time.minute(),
    gps.time.second());
    ez.msgBox("GPS Information", buffer, "", false);
    m5.update();

    if (m5.BtnB.wasPressed()) {
      break;
    }

    ez.yield();
    delay(250);
  }
}

static void menu_settings(void) {
  ezMenu submenu(FURBLE_STR " - Settings");

  submenu.buttons("OK#down");
  submenu.addItem("Backlight", ez.backlight.menu);
  submenu.addItem("GPS Information", gps_info);
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
  Serial2.begin(GPS_BAUD, SERIAL_8N1, 33, 32);

  pinMode(M5_LED, OUTPUT);
  digitalWrite(M5_LED, HIGH);

#include <themes/dark.h>
#include <themes/default.h>
#include <themes/mono_furble.h>

  ez.begin();
  ez.addEvent(service_grove_gps, millis() + 500);
  ez.addEvent(service_gps_fix_led);
  NimBLEDevice::init(FURBLE_STR);
  NimBLEDevice::setSecurityAuth(true, true, true);

  // Set BLE transmit power
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
