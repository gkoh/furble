#ifndef FURBLE_UI_H
#define FURBLE_UI_H

#include <initializer_list>
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
  /**
   * UI input control modes.
   *
   * Modifies button/navigation operation.
   */
  enum class ControlMode { MENU, SHUTTER, SLIDER, REVERT };

  UI(const interval_t &interval);

  void task(void);

  /** Set inactivity timeout in multiples of 30s. */
  void setInactivityTimeout(uint8_t timeout);

  /** Check and/or handle inactivity. */
  void processInactivity(void);

  /**
   * Display/hide navigation bar.
   */
  void displayNavigationBar(bool show);

  /** Configure input control mode. */
  void configureControl(ControlMode mode, bool set = true);

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
    int32_t column;
    int32_t row;
  } grid_position_t;

  typedef struct {
    lv_obj_t *main;
    lv_group_t *group;
    lv_obj_t *page;
    lv_obj_t *button;
    grid_position_t grid;
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
      STATE_WAIT,
      STATE_SHUTTER_OPEN,
      STATE_DELAY,
      STATE_FINISHED,
    } state_t;

    Intervalometer(const interval_t &interval);

    void save(void);

    state_t m_State;
    Spinner m_Count;
    Spinner m_Delay;
    Spinner m_Shutter;
    Spinner m_Wait;
  };

  typedef enum { MODE_SCAN, MODE_DELETE, MODE_CONNECT, MODE_MULTICONNECT } CameraListMode_t;

  typedef struct {
    UI *ui;
    lv_obj_t *messageBox;
    lv_obj_t *label;
    lv_obj_t *bar;
    lv_obj_t *cancel;
    const char *menuName;
  } ConnectContext_t;

  static std::mutex m_Mutex;

  static ConnectContext_t m_ConnectContext;

  // Click streak latency threshold in milliseconds
  static const uint8_t m_ClickThreshold = 20;

  static const uint32_t m_KeyLeft = LV_KEY_LEFT;
  static const uint32_t m_KeyEnter = LV_KEY_ENTER;
  static const uint32_t m_KeyRight = LV_KEY_RIGHT;

#if (FURBLE_TEST_VERSION + 0)
  static constexpr const char *m_Title = FURBLE_VERSION;
#else
  static constexpr const char *m_Title = FURBLE_STR;
#endif
  static const uint8_t m_BrightnessSteps = 16;

  // main menu
  static constexpr const char *m_ConnectStr = "Connect";
  static constexpr const char *m_ScanStr = "Scan";
  static constexpr const char *m_DeleteStr = "Delete";
  static constexpr const char *m_SettingsStr = "Settings";
  static constexpr const char *m_PowerOffStr = "Off";

  // connected
  static constexpr const char *m_ConnectedStr = "Connected";
  static constexpr const char *m_RemoteShutter = "Remote";
  static constexpr const char *m_RemoteInterval = "Interval";
  static constexpr const char *m_RemoteDisconnect = "Disconnect";
  // dodgy hack, add a space so map key is unique
  static constexpr const char *m_IntervalometerRunStr = "Intervalometer ";

  // settings
  static constexpr const char *m_BacklightStr = "Backlight";
  static constexpr const char *m_FeaturesStr = "Features";
  static constexpr const char *m_GPSStr = "GPS";
  static constexpr const char *m_IntervalometerStr = "Timer";
  static constexpr const char *m_ThemeStr = "Theme";
  static constexpr const char *m_TransmitPowerStr = "TX Power";
  static constexpr const char *m_AboutStr = "About";

  // settings->gps
  static constexpr const char *m_GPSDataStr = "GPS Data";

  // settings->intervalometer
  static constexpr const char *m_IntervalCountStr = "Count";
  static constexpr const char *m_IntervalDelayStr = "Delay";
  static constexpr const char *m_IntervalShutterStr = "Shutter";
  static constexpr const char *m_IntervalWaitStr = "Wait";

  static constexpr uint8_t BYTES_PER_PIXEL = (LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB565));
  static constexpr int32_t MAX_WIDTH = 320;
  static constexpr int32_t MAX_HEIGHT = 240;
  static constexpr size_t BUFFER_SIZE = (UI::MAX_WIDTH * (MAX_HEIGHT / 15) * UI::BYTES_PER_PIXEL);

#if defined(FURBLE_M5COREX)
  static constexpr int32_t ICON_MENU_SIZE = 48;
#else
  static constexpr int32_t ICON_MENU_SIZE = 24;
#endif

  static constexpr int32_t ICON_HEADER_SIZE = 24;

  static LV_ATTRIBUTE_MEM_ALIGN void *m_Buffer1;
  static LV_ATTRIBUTE_MEM_ALIGN void *m_Buffer2;

  static lv_timer_t *m_ConnectTimer;
  static lv_timer_t *m_IntervalTimer;
  static lv_obj_t *m_IntervalStateLabel;
  static lv_obj_t *m_IntervalCountLabel;
  static lv_obj_t *m_IntervalRemainingLabel;
  static lv_timer_t *m_IntervalPageRefresh;
  static uint32_t m_IntervalNext;

  const std::vector<int32_t> m_GridLayoutColDsc = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1),
                                                   LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
  const std::vector<int32_t> m_GridLayoutRowDsc = {LV_GRID_FR(1), LV_GRID_FR(1),
                                                   LV_GRID_TEMPLATE_LAST};

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
  lv_obj_t *m_NavBar = nullptr;

  lv_obj_t *m_Left;
  lv_obj_t *m_OK;
  lv_obj_t *m_Right;
  lv_obj_t *m_ShutterLockIcon;
  ControlMode m_ControlMode = ControlMode::MENU;

  lv_obj_t *m_IntervalStart = nullptr;
  Intervalometer m_Intervalometer;

  status_t m_Status;
  bool m_ShutterLock = false;
  uint32_t m_InactivityTimeout;

  static menu_t m_MainMenu;

  static std::unordered_map<const char *, menu_t> m_Menu;

  lv_obj_t *m_PowerOff = nullptr;

  static uint8_t m_PMICClickCount;
  bool m_PMICHack = false;
  uint32_t m_PMICClickTime = 0;

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
  lv_obj_t *addIcon(const lv_image_dsc_t *symbol);

  /** Set the icon symbol in the root window header. */
  void setIcon(lv_obj_t *icon, const lv_image_dsc_t *symbol);

  /** Add a menu item. */
  static lv_obj_t *addMenuItem(const menu_t &menu,
                               const lv_image_dsc_t *icon,
                               const char *text,
                               bool checkbox = false,
                               const int32_t col_pos = 0,
                               const int32_t row_pos = 0);

  /** Add a menu switch item. */
  void addSettingItem(lv_obj_t *page, const char *symbol, Settings::type_t setting);

  /** Add camera menu item. */
  static lv_obj_t *addCameraItem(Camera *camera, const menu_t &menu, const CameraListMode_t mode);

  /** Create a menu entry. */
  menu_t &addMenu(const char *entry,
                  const lv_image_dsc_t *symbol,
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

  /** Configure shutter control. */
  void configShutterControl(void);

  /** Configure menu control. */
  void configMenuControl(void);

  /** Configure slider control. */
  void configSliderControl(void);

  /** Check lock screen activity. */
  void handleLockScreen(void);
};
}  // namespace Furble

#endif
