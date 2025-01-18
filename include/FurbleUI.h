#ifndef FURBLE_UI_H
#define FURBLE_UI_H

#include <mutex>
#include <unordered_map>

#include <lvgl.h>

#include "FurbleControl.h"
#include "FurbleGPS.h"
#include "FurbleSettings.h"
#include "interval.h"

namespace Furble {
class UI {
 public:
  UI(const interval_t &interval);

  void task(void);

  /** Set inactivity timeout in multiples of 30s. */
  void setInactivityTimeout(uint8_t timeout);

  /** Check and/or handle inactivity. */
  void processInactivity(void);

  /**
   * Display/hide navigation bar.
   */
  static void displayNavigationBar(bool show);

  /** Configure shutter control. */
  void configShutterControl(void);

  /** Configure menu control. */
  void configMenuControl(void);

  /** Configure slider control. */
  void configSliderControl(void);

  /** Display shutter intervalometer menu .*/
  void showShutterIntervalometer(bool show);

  /** Lock shutter. */
  void shutterLock(Control &control);

  /** Unlock shutter. */
  void shutterUnlock(Control &control);

  /** Is shutter locked? */
  bool isShutterLocked(void);

  bool m_FocusPressed = false;

 private:
  typedef struct {
    lv_obj_t *main;
    lv_group_t *group;
    lv_obj_t *page;
    lv_obj_t *button;
  } menu_t;

  typedef struct {
    GPS *gps;
    lv_obj_t *gpsIcon;
    lv_obj_t *batteryIcon;
    lv_obj_t *reconnectIcon;
    lv_obj_t *gpsBaud;
    lv_obj_t *gpsData;
    bool screenLocked;
  } status_t;

  class Intervalometer {
   public:
    class Spinner {
     public:
      Spinner(Intervalometer *intervalometer, SpinValue::nvs_t nvs, bool infinite = false)
          : m_Intervalometer {intervalometer}, m_SpinValue {nvs}, m_Infinite {infinite} {};

      static constexpr const char *m_SpinDigitRoller = "0\n1\n2\n3\n4\n5\n6\n7\n8\n9";
      static constexpr const char *m_SpinUnitsRoller = "msec\nsecs\nmins";

      void update(void);
      void updateLabels(void);

      Intervalometer *m_Intervalometer;
      SpinValue m_SpinValue;
      lv_obj_t *m_Button;
      lv_obj_t *m_Label;
      lv_obj_t *m_Value;
      const bool m_Infinite;  // Can support infinite?
      lv_obj_t *m_RowInfinite;
      lv_obj_t *m_SwitchInfinite;

      lv_obj_t *m_RowSpinners;
      // array of rollers, 0 = hundred, 1 = ten, 2 = one
      std::array<lv_obj_t *, 3> m_Roller = {nullptr, nullptr, nullptr};
      lv_obj_t *m_RollerUnit = nullptr;
    };

    typedef enum {
      STATE_IDLE,
      STATE_SHUTTER_OPEN,
      STATE_WAIT,
      STATE_FINISHED,
    } state_t;

    Intervalometer(const interval_t &interval);

    void save(void);

    state_t m_State;
    Spinner m_Count;
    Spinner m_Delay;
    Spinner m_Shutter;
  };

  typedef enum { MODE_SCAN, MODE_DELETE, MODE_CONNECT, MODE_MULTICONNECT } CameraListMode_t;

  static std::mutex m_Mutex;

  static const uint32_t m_KeyLeft = LV_KEY_LEFT;
  static const uint32_t m_KeyEnter = LV_KEY_ENTER;
  static const uint32_t m_KeyRight = LV_KEY_RIGHT;

  static constexpr const char *m_Title = FURBLE_STR;
  static const uint8_t m_BrightnessSteps = 16;

  // main menu
  static constexpr const char *m_ConnectStr = "Connect";
  static constexpr const char *m_ScanStr = "Scan";
  static constexpr const char *m_DeleteStr = "Delete";
  static constexpr const char *m_SettingsStr = "Settings";

  // connected
  static constexpr const char *m_ConnectedStr = "Connected";
  static constexpr const char *m_RemoteShutter = "Shutter";
  static constexpr const char *m_RemoteInterval = "Interval";
  // dodgy hack, add a space so map key is unique
  static constexpr const char *m_IntervalometerRunStr = "Intervalometer ";

  // settings
  static constexpr const char *m_BacklightStr = "Backlight";
  static constexpr const char *m_FeaturesStr = "Features";
  static constexpr const char *m_GPSStr = "GPS";
  static constexpr const char *m_IntervalometerStr = "Intervalometer";
  static constexpr const char *m_ThemeStr = "Theme";
  static constexpr const char *m_TransmitPowerStr = "Transmit Power";
  static constexpr const char *m_AboutStr = "About";

  // settings->gps
  static constexpr const char *m_GPSDataStr = "GPS Data";

  // settings->intervalometer
  static constexpr const char *m_IntervalCountStr = "Count";
  static constexpr const char *m_IntervalDelayStr = "Delay";
  static constexpr const char *m_IntervalShutterStr = "Shutter";

  static constexpr uint8_t BYTES_PER_PIXEL = (LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB565));
  static constexpr int32_t MAX_WIDTH = 320;
  static constexpr int32_t MAX_HEIGHT = 240;

  typedef std::array<uint8_t, UI::MAX_WIDTH *(MAX_HEIGHT / 10) * UI::BYTES_PER_PIXEL>
      display_buffer_t;

  static LV_ATTRIBUTE_MEM_ALIGN display_buffer_t m_Buffer1;
  static LV_ATTRIBUTE_MEM_ALIGN display_buffer_t m_Buffer2;

  static lv_obj_t *m_ConnectMessageBox;
  static lv_obj_t *m_ConnectLabel;
  static lv_obj_t *m_ConnectBar;
  static lv_obj_t *m_ConnectCancel;
  static lv_timer_t *m_ConnectTimer;
  static lv_timer_t *m_IntervalTimer;
  static lv_obj_t *m_IntervalStateLabel;
  static lv_obj_t *m_IntervalCountLabel;
  static lv_obj_t *m_IntervalRemainingLabel;
  static lv_timer_t *m_IntervalPageRefresh;
  static uint32_t m_IntervalNext;

  GPS &m_GPS;

  int32_t m_Width;
  int32_t m_Height;

  uint8_t m_MinimumBrightness;

  lv_indev_t *m_ButtonL;
  lv_indev_t *m_ButtonO;
  lv_indev_t *m_ButtonR;
  lv_indev_t *m_Touch;
  lv_group_t *m_Group;

  lv_display_t *m_Display = nullptr;
  lv_obj_t *m_Screen = nullptr;
  lv_obj_t *m_Root = nullptr;
  lv_obj_t *m_Header = nullptr;
  lv_obj_t *m_Content = nullptr;
  static lv_obj_t *m_NavBar;

  static lv_obj_t *m_Left;
  static lv_obj_t *m_OK;
  static lv_obj_t *m_Right;

  lv_obj_t *m_IntervalStart = nullptr;
  Intervalometer m_Intervalometer;

  status_t m_Status;
  bool m_ShutterLock = false;
  lv_obj_t *m_ShutterLockLabel = nullptr;
  uint32_t m_InactivityTimeout;

  static menu_t m_MainMenu;

  static std::unordered_map<const char *, menu_t> m_Menu;

  lv_obj_t *m_PowerOff = nullptr;

  static bool m_PMICHack;
  static bool m_PMICClicked;
  static void buttonPWRRead(lv_indev_t *drv, lv_indev_data_t *data);
  static void buttonPEKRead(lv_indev_t *drv, lv_indev_data_t *data);
  static void buttonARead(lv_indev_t *drv, lv_indev_data_t *data);
  static void buttonBRead(lv_indev_t *drv, lv_indev_data_t *data);
  static void buttonCRead(lv_indev_t *drv, lv_indev_data_t *data);
  static void touchRead(lv_indev_t *drv, lv_indev_data_t *data);

  /** Flush display. */
  static void displayFlush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);

  /** LVGL tick function. */
  static uint32_t tick(void);

  void initInputDevices(void);

  static void setTheme(std::string name);

  void prepareShutterControl(void);

  /** Add icon to the root window header. */
  lv_obj_t *addIcon(const char *symbol);

  /** Set the icon symbol in the root window header. */
  void setIcon(lv_obj_t *icon, const char *symbol);

  /** Add a menu item. */
  static lv_obj_t *addMenuItem(const menu_t &menu,
                               const char *icon,
                               const char *text,
                               bool checkbox = false);

  /** Add a menu switch item. */
  void addSettingItem(lv_obj_t *page, const char *symbol, Settings::type_t setting);

  /** Add camera menu item. */
  static lv_obj_t *addCameraItem(Camera *camera, const menu_t &menu, const CameraListMode_t mode);

  /** Create a menu entry. */
  menu_t &addMenu(const char *entry,
                  const char *symbol,
                  bool button = true,
                  const menu_t &parent = m_MainMenu);

  /** Add the main menu to the root window content. */
  void addMainMenu(void);

  /** Add the 'Connect' menu entry. */
  void addConnectMenu(void);

  /** Add the 'Scan' menu entry. */
  void addScanMenu(void);

  /** Add the 'Delete' menu entry. */
  void addDeleteMenu(void);

  /** Add 'GPS Data' page. */
  void addGPSMenu(const menu_t &parent);

  /** Add the 'Features' menu entry. */
  void addFeaturesMenu(const menu_t &parent);

  /** Add the 'Intervalometer' menu entry. */
  void addIntervalometerMenu(const menu_t &parent);

  /** Add spinner menu item entry. */
  lv_obj_t *addSpinItem(lv_obj_t *page, const char *item, Intervalometer::Spinner &spinner);

  /** Add the spinner page menu entry. */
  void addSpinnerPage(const menu_t &parent, const char *item, Intervalometer::Spinner &spinner);

  void addBacklightMenu(const menu_t &parent);

  void addThemeMenu(const menu_t &parent);

  void addTransmitPowerMenu(const menu_t &parent);

  void addAboutMenu(const menu_t &parent);

  /** Add the 'Settings' menu entry. */
  void addSettingsMenu(void);

  /** Add 'Connected' menu. */
  menu_t &addConnectedMenu(void);

  /** Update entries in connect page. */
  static void updateItems(const menu_t &menu);

  /** Stop GPS Data timer. */
  static void gpsDataStop(lv_event_t *e);

  /** Handle connection request. */
  static void doConnect(lv_event_t *e);

  /** Handle disconnection. */
  static void doDisconnect(void);

  /** Refresh deletion items. */
  static void refreshDelete(void);

  /** Connection timer handler. */
  static void connectTimerHandler(lv_timer_t *timer);

  /** Intervalometer timer handler. */
  static void intervalometer(lv_timer_t *timer);

  /** Handle shutter event. */
  static void handleShutter(lv_event_t *e);

  /** Handle focus event. */
  static void handleFocus(lv_event_t *e);

  /** Handle shutter lock event. */
  static void handleShutterLock(lv_event_t *e);
};
}  // namespace Furble

void vUITask(void *param);

#endif
