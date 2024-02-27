#include <Furble.h>
#include <M5ez.h>
#include <NimBLEDevice.h>
#include <TinyGPS++.h>

#include <M5Unified.h>

const uint32_t SCAN_DURATION = 10;

static NimBLEScan *pScan = nullptr;

static std::vector<Furble::Device *> connect_list;

bool load_gps_enable();

static TinyGPSPlus gps;
HardwareSerial GroveSerial(2);
static const uint32_t GPS_BAUD = 9600;
static const uint16_t GPS_SERVICE_MS = 250;
static const uint32_t GPS_MAX_AGE_MS = 60 * 1000;

static const uint8_t CURRENT_POSITION = LEFTMOST + 1;
static const uint8_t GPS_HEADER_POSITION = CURRENT_POSITION + 1;

static bool gps_enable = false;
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
  if (!gps_enable) {
    return GPS_SERVICE_MS;
  }

  while (Serial2.available() > 0) {
    gps.encode(Serial2.read());
  }

  if ((gps.location.age() < GPS_MAX_AGE_MS) && gps.location.isValid()
      && (gps.date.age() < GPS_MAX_AGE_MS) && gps.date.isValid()
      && (gps.time.age() < GPS_MAX_AGE_MS) && gps.time.age()) {
    gps_has_fix = true;
  } else {
    gps_has_fix = false;
  }

  return GPS_SERVICE_MS;
}

/**
 * Update geotag data.
 */
static void update_geodata(Furble::Device *device) {
  if (!gps_enable) {
    return;
  }

  if (gps.location.isUpdated() && gps.location.isValid() && gps.date.isUpdated()
      && gps.date.isValid() && gps.time.isValid() && gps.time.isValid()) {
    Furble::Device::gps_t dgps = {gps.location.lat(), gps.location.lng(), gps.altitude.meters()};
    Furble::Device::timesync_t timesync = {gps.date.year(), gps.date.month(),  gps.date.day(),
                                           gps.time.hour(), gps.time.minute(), gps.time.second()};

    device->updateGeoData(dgps, timesync);
    ez.header.draw("gps");
  }
}

/**
 * Draw GPS enable/fix widget.
 */
static void gps_draw_widget(uint16_t x, uint16_t y) {
  if (!gps_enable) {
    return;
  }

  int16_t r = (ez.theme->header_height * 0.8) / 2;
  int16_t cx = x + r;
  int16_t cy = (ez.theme->header_height / 2);

  if (gps_has_fix) {
    // With fix, draw solid circle
    M5.Lcd.fillCircle(cx, cy, r, ez.theme->header_fgcolor);
  } else {
    // No fix, empty circle
    M5.Lcd.drawCircle(cx, cy, r, ez.theme->header_fgcolor);
  }
}

static void current_draw_widget(uint16_t x, uint16_t y) {
  // hard disable for now
  return;

  M5.Lcd.fillRect(x, 0, y, ez.theme->header_height, ez.theme->header_bgcolor);
  M5.Lcd.setTextColor(ez.theme->header_fgcolor);
  M5.Lcd.setTextDatum(TL_DATUM);
  int32_t ma = M5.Power.getBatteryCurrent();
  Serial.println(ma);
  char s[32] = {0};
  snprintf(s, 32, "%d", ma);
  M5.Lcd.drawString(s, x + ez.theme->header_hmargin, ez.theme->header_tmargin + 2);
}

static uint16_t current_service(void) {
  ez.header.draw("current");
  return 1000;
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

    M5.update();

    if (M5.BtnB.wasReleased()) {
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
    M5.update();

    update_geodata(device);

#ifdef M5STACK_CORE2
    if (M5.BtnC.wasPressed()) {
      break;
    }
#else
    if (M5.BtnPWR.wasClicked()) {
      break;
    }
#endif

    if (M5.BtnA.wasPressed()) {
      device->shutterPress();
    }

    if (M5.BtnA.wasReleased()) {
      device->shutterRelease();
    }

    if (M5.BtnB.wasPressed()) {
      device->focusPress();
    }

    if (M5.BtnB.wasReleased()) {
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

static void menu_settings(void) {
  ezMenu submenu(FURBLE_STR " - Settings");

  submenu.buttons("OK#down");
  submenu.addItem("Backlight", ez.backlight.menu);
  submenu.addItem("GPS", settings_menu_gps);
  submenu.addItem("Theme", ez.theme->menu);
  submenu.addItem("Transmit Power", settings_menu_tx_power);
  submenu.addItem("About", about);
  submenu.addItem("Back");
  submenu.downOnLast("first");
  submenu.run();
}

static void mainmenu_poweroff(void) {
  M5.Power.powerOff();
}

void setup() {
  gps_enable = load_gps_enable();

  Serial.begin(115200);
  Serial2.begin(GPS_BAUD, SERIAL_8N1, 33, 32);

#include <themes/dark.h>
#include <themes/default.h>
#include <themes/mono_furble.h>

  ez.begin();

  uint8_t width = 4 * M5.Lcd.textWidth("5") + ez.theme->header_hmargin * 2;
  ez.header.insert(CURRENT_POSITION, "current", width, current_draw_widget);
  ez.header.insert(GPS_HEADER_POSITION, "gps", ez.theme->header_height * 0.8, gps_draw_widget);
  ez.addEvent(service_grove_gps, millis() + 500);
  ez.addEvent(current_service, millis() + 500);

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
