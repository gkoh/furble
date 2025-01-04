#include <algorithm>
#include <numeric>
#include <tuple>

#include <M5Unified.h>
#include <lvgl.h>
#include <src/themes/lv_theme_private.h>

#include <Device.h>
#include <Scan.h>

#include "FurbleControl.h"
#include "FurbleGPS.h"
#include "FurbleSettings.h"
#include "FurbleUI.h"
#include "interval.h"

void vUITask(void *param) {
  using namespace Furble;
  auto interval = Settings::load<interval_t>(Settings::INTERVAL);
  auto ui = UI(interval);

  ui.task();
}

namespace Furble {

std::mutex UI::m_Mutex;

constexpr const char *UI::m_ConnectStr;
constexpr const char *UI::m_ConnectedStr;
constexpr const char *UI::m_DeleteStr;
constexpr const char *UI::m_ScanStr;
constexpr const char *UI::m_SettingsStr;
constexpr const char *UI::m_IntervalometerStr;
constexpr const char *UI::m_RemoteShutter;
constexpr const char *UI::m_IntervalometerRunStr;

const uint32_t UI::m_KeyEnter;
const uint32_t UI::m_KeyLeft;
const uint32_t UI::m_KeyRight;

lv_obj_t *UI::m_NavBar;
lv_obj_t *UI::m_Left;
lv_obj_t *UI::m_OK;
lv_obj_t *UI::m_Right;

bool UI::m_PMICHack;
bool UI::m_PMICClicked;

lv_timer_t *UI::m_IntervalTimer;

UI::display_buffer_t UI::m_Buffer1;
UI::display_buffer_t UI::m_Buffer2;

lv_obj_t *UI::m_ConnectMessageBox;
lv_obj_t *UI::m_ConnectLabel;
lv_obj_t *UI::m_ConnectBar;
lv_obj_t *UI::m_ConnectCancel;
lv_timer_t *UI::m_ConnectTimer;

lv_obj_t *UI::m_IntervalStateLabel;
lv_obj_t *UI::m_IntervalCountLabel;
lv_obj_t *UI::m_IntervalRemainingLabel;
lv_timer_t *UI::m_IntervalPageRefresh;
uint32_t UI::m_IntervalNext;

UI::menu_t UI::m_MainMenu;

std::unordered_map<const char *, UI::menu_t> UI::m_Menu = {
    {m_ConnectStr,           {}},
    {m_ScanStr,              {}},
    {m_DeleteStr,            {}},
    {m_SettingsStr,          {}},
    {m_ConnectedStr,         {}},
    {m_FeaturesStr,          {}},
    {m_GPSStr,               {}},
    {m_GPSDataStr,           {}},
    {m_IntervalometerStr,    {}},
    {m_IntervalCountStr,     {}},
    {m_IntervalDelayStr,     {}},
    {m_IntervalShutterStr,   {}},
    {m_BacklightStr,         {}},
    {m_ThemeStr,             {}},
    {m_TransmitPowerStr,     {}},
    {m_AboutStr,             {}},
    {m_RemoteShutter,        {}},
    {m_RemoteInterval,       {}},
    {m_IntervalometerRunStr, {}},
};

UI::UI(const interval_t &interval) : m_GPS {GPS::getInstance()}, m_Intervalometer(interval) {
  lv_init();
  lv_tick_set_cb(tick);

  // set display resolution
  m_Width = M5.Display.width();
  m_Height = M5.Display.height();

  // set display brightness
  auto brightness = Settings::load<uint8_t>(Settings::BRIGHTNESS);
  M5.Display.setBrightness(brightness);
  setInactivityTimeout(Settings::load<uint8_t>(Settings::INACTIVITY));

  // set minimum, ensure this is a multiple of m_BrightnessSteps so the slider steps work
  switch (M5.getBoard()) {
    case m5::board_t::board_M5StickCPlus2:
    case m5::board_t::board_M5StackCore2:
    case m5::board_t::board_M5Stack:
      m_MinimumBrightness = 32;
      break;
    case m5::board_t::board_M5StickCPlus:
    case m5::board_t::board_M5StickC:
      m_MinimumBrightness = 48;
      break;
    default:
      m_MinimumBrightness = 32;
  }

  // start inactivity timer
  lv_timer_create(
      [](lv_timer_t *t) {
        auto *ui = static_cast<Furble::UI *>(lv_timer_get_user_data(t));
        ui->processInactivity();
      },
      1000, this);

  // configure display
  m_Display = lv_display_create(m_Width, m_Height);
  lv_display_set_default(m_Display);
  lv_display_set_flush_cb(m_Display, displayFlush);

  // configure display buffers
  lv_display_set_buffers(m_Display, m_Buffer1.data(), m_Buffer2.data(), m_Buffer1.size(),
                         LV_DISPLAY_RENDER_MODE_PARTIAL);

  initInputDevices();

  setTheme(Settings::load<std::string>(Settings::THEME));

  m_Screen = lv_screen_active();

  m_Root = lv_win_create(m_Screen);
  lv_obj_update_layout(m_Root);

  lv_win_add_title(m_Root, m_Title);
  m_Header = lv_win_get_header(m_Root);

  m_GPS.init();
  m_Status.gps = &m_GPS;
  m_Status.reconnectIcon = addIcon(LV_SYMBOL_REFRESH);
  m_Status.gpsIcon = addIcon(LV_SYMBOL_GPS);
#if FURBLE_BATTERY_DEBUG == 1
  m_Status.batteryIcon = lv_label_create(m_Header);
#else
  m_Status.batteryIcon = addIcon(LV_SYMBOL_BATTERY_3);
#endif
  m_Status.screenLocked = false;

  m_GPS.setIcon(m_Status.gpsIcon);

  // refresh icons every 250ms
  lv_timer_create(
      [](lv_timer_t *timer) {
        status_t *status = static_cast<status_t *>(lv_timer_get_user_data(timer));

#if FURBLE_BATTERY_DEBUG == 1
        int32_t current = M5.Power.getBatteryCurrent();
        static int32_t mean = current;

        // exponentially weighted moving average with alpha = 0.33
        mean = mean + (current - mean) / 3;

        lv_label_set_text_fmt(status->batteryIcon, "%d", mean);
#else
        const char *symbol = NULL;
        int32_t level = M5.Power.getBatteryLevel();
        if (level >= 95) {
          symbol = LV_SYMBOL_BATTERY_FULL;
        } else if (level >= 66) {
          symbol = LV_SYMBOL_BATTERY_3;
        } else if (level >= 33) {
          symbol = LV_SYMBOL_BATTERY_2;
        } else if (level >= 5) {
          symbol = LV_SYMBOL_BATTERY_1;
        } else {
          symbol = LV_SYMBOL_BATTERY_EMPTY;
        }
        lv_image_set_src(status->batteryIcon, symbol);
#endif

        if (status->gps->isEnabled()) {
          lv_obj_clear_flag(status->gpsIcon, LV_OBJ_FLAG_HIDDEN);
        } else {
          lv_obj_add_flag(status->gpsIcon, LV_OBJ_FLAG_HIDDEN);
        }

        static lv_obj_t *lockMsgBox = NULL;
        if (status->screenLocked && (lockMsgBox == NULL)) {
          lockMsgBox = lv_msgbox_create(NULL);
          lv_msgbox_add_title(lockMsgBox, "Screen Locked");
          lv_msgbox_add_text(lockMsgBox, "Double-click PWR button to unlock.");
        } else if (!status->screenLocked && (lockMsgBox != NULL)) {
          lv_msgbox_close_async(lockMsgBox);
          lockMsgBox = NULL;
        }
      },
      250, &m_Status);

  lv_obj_update_layout(m_Header);
  lv_obj_set_height(m_Header, 1.2f * lv_font_get_line_height(LV_FONT_DEFAULT));

  lv_obj_t *x = lv_win_get_content(m_Root);
  lv_obj_set_width(x, LV_PCT(100));
  lv_obj_set_layout(x, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(x, LV_FLEX_FLOW_COLUMN);
  m_Content = lv_obj_create(x);
  lv_obj_set_width(m_Content, LV_PCT(100));
  lv_obj_set_flex_grow(m_Content, 1);

  // zero the padding
  lv_obj_set_style_pad_top(m_Content, 0, LV_STATE_DEFAULT);
  lv_obj_set_style_pad_row(m_Content, 0, LV_STATE_DEFAULT);
  lv_obj_set_style_pad_bottom(m_Content, 0, LV_STATE_DEFAULT);
  lv_obj_set_style_pad_left(m_Content, 0, LV_STATE_DEFAULT);
  lv_obj_set_style_pad_right(m_Content, 0, LV_STATE_DEFAULT);

  lv_obj_set_style_pad_row(x, 0, LV_STATE_DEFAULT);
  lv_obj_set_style_pad_top(x, 0, LV_STATE_DEFAULT);
  lv_obj_set_style_pad_bottom(x, 0, LV_STATE_DEFAULT);
  lv_obj_set_style_pad_left(x, 0, LV_STATE_DEFAULT);
  lv_obj_set_style_pad_right(x, 0, LV_STATE_DEFAULT);

  // add navigation buttons
  if (!M5.Touch.isEnabled()) {
    auto height = lv_font_get_line_height(LV_FONT_DEFAULT);

    m_NavBar = lv_obj_create(x);
    lv_obj_set_width(m_NavBar, LV_PCT(100));
    lv_obj_set_height(m_NavBar, 1.2f * lv_font_get_line_height(LV_FONT_DEFAULT));
    lv_obj_set_layout(m_NavBar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(m_NavBar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(m_NavBar, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(m_NavBar, LV_OBJ_FLAG_SCROLLABLE);

    switch (M5.getBoard()) {
      case m5::board_t::board_M5StickC:
      case m5::board_t::board_M5StickCPlus:
      case m5::board_t::board_M5StickCPlus2:
        lv_obj_set_style_pad_left(m_Content, 0, LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(m_Content, 0, LV_STATE_DEFAULT);

        m_Left = lv_button_create(m_Screen);
        m_OK = lv_button_create(m_NavBar);
        m_Right = lv_button_create(m_Screen);

        lv_obj_add_flag(m_Left, LV_OBJ_FLAG_FLOATING);
        lv_obj_align(m_Left, LV_ALIGN_BOTTOM_LEFT, 0, 0);

        lv_obj_set_style_bg_image_src(m_Right, LV_SYMBOL_RIGHT, 0);
        lv_obj_add_flag(m_Right, LV_OBJ_FLAG_FLOATING);
        lv_obj_align(m_Right, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
        break;

      default:
        m_Left = lv_button_create(m_NavBar);
        m_OK = lv_button_create(m_NavBar);
        m_Right = lv_button_create(m_NavBar);
        lv_obj_set_style_bg_image_src(m_Right, LV_SYMBOL_RIGHT, 0);
        break;
    }

    // prepare shutter handling
    prepareShutterControl();

    // ensure buttons do not receive focus
    lv_group_remove_obj(m_Left);
    lv_group_remove_obj(m_OK);
    lv_group_remove_obj(m_Right);

    lv_obj_set_style_bg_image_src(m_Left, LV_SYMBOL_LEFT, 0);
    lv_obj_set_size(m_Left, height, height);
    lv_obj_set_style_bg_image_src(m_OK, LV_SYMBOL_OK, 0);
    lv_obj_set_size(m_OK, height, height);
    lv_obj_set_size(m_Right, height, height);
  }

  // create connection timer
  m_ConnectTimer = lv_timer_create(connectTimerHandler, 125, NULL);
  lv_timer_pause(m_ConnectTimer);

  // create intervalometer timer
  m_IntervalTimer = lv_timer_create(intervalometer, 100, &m_Intervalometer);
  lv_timer_pause(m_IntervalTimer);

  addMainMenu();

  m_GPS.startService();
}

void UI::buttonPWRRead(lv_indev_t *drv, lv_indev_data_t *data) {
  data->key = *(static_cast<uint32_t *>(lv_indev_get_user_data(drv)));
  if (M5.BtnPWR.isReleased()) {
    data->state = LV_INDEV_STATE_RELEASED;
  } else if (M5.BtnPWR.isPressed()) {
    data->state = LV_INDEV_STATE_PRESSED;
  }
}

// read power button for M5StickC and M5StickCPlus
void UI::buttonPEKRead(lv_indev_t *drv, lv_indev_data_t *data) {
  data->key = *(static_cast<uint32_t *>(lv_indev_get_user_data(drv)));
  if (m_PMICClicked) {
    data->state = LV_INDEV_STATE_PRESSED;
    m_PMICClicked = false;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

void UI::buttonARead(lv_indev_t *drv, lv_indev_data_t *data) {
  data->key = *(static_cast<uint32_t *>(lv_indev_get_user_data(drv)));
  if (M5.BtnA.isReleased()) {
    data->state = LV_INDEV_STATE_RELEASED;
  } else if (M5.BtnA.isPressed()) {
    data->state = LV_INDEV_STATE_PRESSED;
  }
}

void UI::buttonBRead(lv_indev_t *drv, lv_indev_data_t *data) {
  data->key = *(static_cast<uint32_t *>(lv_indev_get_user_data(drv)));
  if (M5.BtnB.isReleased()) {
    data->state = LV_INDEV_STATE_RELEASED;
  } else if (M5.BtnB.isPressed()) {
    data->state = LV_INDEV_STATE_PRESSED;
  }
}

void UI::buttonCRead(lv_indev_t *drv, lv_indev_data_t *data) {
  data->key = *(static_cast<uint32_t *>(lv_indev_get_user_data(drv)));
  if (M5.BtnC.isReleased()) {
    data->state = LV_INDEV_STATE_RELEASED;
  } else if (M5.BtnC.isPressed()) {
    data->state = LV_INDEV_STATE_PRESSED;
  }
}

void UI::touchRead(lv_indev_t *drv, lv_indev_data_t *data) {
  auto count = M5.Touch.getCount();
  if (count == 0) {
    data->state = LV_INDEV_STATE_RELEASED;
  } else {
    auto touch = M5.Touch.getDetail(0);
    data->state = LV_INDEV_STATE_PRESSED;
    data->point.x = touch.x;
    data->point.y = touch.y;
  }
}

void UI::displayFlush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  lv_draw_sw_rgb565_swap(px_map, w * h);
  M5.Display.pushImageDMA<uint16_t>(area->x1, area->y1, w, h, (uint16_t *)px_map);
  lv_disp_flush_ready(disp);
}

uint32_t UI::tick(void) {
  return (esp_timer_get_time() / 1000LL);
}

void UI::initInputDevices(void) {
  m_Group = lv_group_create();
  lv_group_set_default(m_Group);

  m_ButtonL = lv_indev_create();
  lv_indev_set_type(m_ButtonL, LV_INDEV_TYPE_ENCODER);
  lv_indev_set_user_data(m_ButtonL, const_cast<void *>(static_cast<const void *>(&m_KeyLeft)));
  lv_indev_set_group(m_ButtonL, m_Group);

  m_ButtonO = lv_indev_create();
  lv_indev_set_type(m_ButtonO, LV_INDEV_TYPE_ENCODER);
  lv_indev_set_user_data(m_ButtonO, const_cast<void *>(static_cast<const void *>(&m_KeyEnter)));
  lv_indev_set_group(m_ButtonO, m_Group);

  m_ButtonR = lv_indev_create();
  lv_indev_set_type(m_ButtonR, LV_INDEV_TYPE_ENCODER);
  lv_indev_set_user_data(m_ButtonR, const_cast<void *>(static_cast<const void *>(&m_KeyRight)));
  lv_indev_set_group(m_ButtonR, m_Group);

  switch (M5.getBoard()) {
    case m5::board_t::board_M5StickC:
    case m5::board_t::board_M5StickCPlus:
      m_PMICHack = true;
      lv_indev_set_read_cb(m_ButtonL, buttonPEKRead);
      lv_indev_set_read_cb(m_ButtonO, buttonARead);
      lv_indev_set_read_cb(m_ButtonR, buttonBRead);
      break;

    case m5::board_t::board_M5StickCPlus2:
      lv_indev_set_read_cb(m_ButtonL, buttonPWRRead);
      lv_indev_set_read_cb(m_ButtonO, buttonARead);
      lv_indev_set_read_cb(m_ButtonR, buttonBRead);
      break;

    case m5::board_t::board_M5StackCore2:
      m_Touch = lv_indev_create();
      lv_indev_set_type(m_Touch, LV_INDEV_TYPE_POINTER);
      lv_indev_set_read_cb(m_Touch, touchRead);
      __attribute__((fallthrough));

    case m5::board_t::board_M5Stack:
      lv_indev_set_read_cb(m_ButtonL, buttonARead);
      lv_indev_set_read_cb(m_ButtonO, buttonBRead);
      lv_indev_set_read_cb(m_ButtonR, buttonCRead);
      break;

    default:
      ESP_LOGE("ui", "Unknown hardware, not configuring input devices");
  }
}

void UI::setTheme(std::string name) {
  lv_display_t *display = lv_display_get_default();
  lv_color_t primary = lv_palette_main(LV_PALETTE_BLUE);
  lv_color_t secondary = lv_color_black();
  bool dark = false;
  static lv_theme_t theme;
  static lv_style_t style_bg;
  static lv_style_t style_button;

  lv_style_init(&style_bg);
  lv_style_init(&style_button);

  if (name == "Dark") {
    dark = true;
    lv_style_set_bg_color(&style_bg, lv_color_black());
    lv_style_set_outline_color(&style_button, LV_COLOR_MAKE(127, 255, 0));
  } else if (name == "Mono Furble") {
    dark = true;
    primary = lv_palette_main(LV_PALETTE_ORANGE);
    lv_style_set_bg_color(&style_bg, lv_color_black());
    lv_style_set_outline_color(&style_button, lv_color_white());
  } else {
    // Default
    dark = false;
    lv_style_set_outline_color(&style_button, lv_palette_main(LV_PALETTE_ORANGE));
  }

  lv_theme_t *theme_default =
      lv_theme_default_init(display, primary, secondary, dark, LV_FONT_DEFAULT);
  theme = *theme_default;
  lv_theme_set_parent(&theme, theme_default);
  lv_theme_set_apply_cb(&theme, [](lv_theme_t *th, lv_obj_t *obj) {
    if (lv_obj_check_type(obj, &lv_button_class) || lv_obj_check_type(obj, &lv_roller_class)
        || lv_obj_check_type(obj, &lv_slider_class) || lv_obj_check_type(obj, &lv_switch_class)) {
      lv_obj_add_style(obj, &style_button, LV_STATE_FOCUS_KEY);
    } else if (!lv_obj_check_type(obj, &lv_button_class)
               && !lv_obj_check_type(obj, &lv_msgbox_footer_button_class)) {
      lv_obj_add_style(obj, &style_bg, LV_STATE_DEFAULT);
    }
  });
  lv_display_set_theme(display, &theme);
}

void UI::shutterLock(Control &control) {
  if (!m_ShutterLock) {
    control.sendCommand(Control::CMD_SHUTTER_PRESS);
    m_ShutterLock = true;
    ESP_LOGI("ui", "SHUTTER LOCKED");

    if (M5.Touch.isEnabled()) {
      lv_obj_add_state(m_OK, LV_STATE_DISABLED);
      lv_obj_add_state(m_Right, LV_STATE_DISABLED);
    }
  }
}

void UI::shutterUnlock(Control &control) {
  if (m_ShutterLock) {
    control.sendCommand(Control::CMD_SHUTTER_RELEASE);
    m_ShutterLock = false;
    ESP_LOGI("ui", "SHUTTER UNLOCKED");

    if (M5.Touch.isEnabled()) {
      lv_obj_remove_state(m_OK, LV_STATE_DISABLED);
      lv_obj_remove_state(m_Right, LV_STATE_DISABLED);
    }
  }
}

bool UI::isShutterLocked(void) {
  return m_ShutterLock;
}

void UI::handleShutter(lv_event_t *e) {
  auto *ui = static_cast<UI *>(lv_event_get_user_data(e));
  auto &control = Control::getInstance();
  lv_event_code_t code = lv_event_get_code(e);
  switch (code) {
    case LV_EVENT_PRESSED:
      if (ui->m_FocusPressed) {
        ui->shutterLock(control);
      } else if (!ui->isShutterLocked()) {
        control.sendCommand(Control::CMD_SHUTTER_PRESS);
      }
      break;
    case LV_EVENT_RELEASED:
      if (ui->isShutterLocked()) {
        if (!ui->m_FocusPressed) {
          ui->shutterUnlock(control);
        }
      } else {
        control.sendCommand(Control::CMD_SHUTTER_RELEASE);
      }
      break;
    default:
      break;
  }
}

void UI::handleFocus(lv_event_t *e) {
  auto *ui = static_cast<UI *>(lv_event_get_user_data(e));
  auto &control = Control::getInstance();
  lv_event_code_t code = lv_event_get_code(e);
  switch (code) {
    case LV_EVENT_PRESSED:
      ui->m_FocusPressed = true;
      if (ui->isShutterLocked()) {
        ui->shutterUnlock(control);
      } else {
        control.sendCommand(Control::CMD_FOCUS_PRESS);
      }
      break;
    case LV_EVENT_RELEASED:
      ui->m_FocusPressed = false;
      if (!ui->isShutterLocked()) {
        control.sendCommand(Control::CMD_FOCUS_RELEASE);
      }
      break;
    default:
      break;
  }
}

void UI::handleShutterLock(lv_event_t *e) {
  auto *ui = static_cast<UI *>(lv_event_get_user_data(e));
  auto &control = Control::getInstance();
  lv_event_code_t code = lv_event_get_code(e);

  // Bug in LVGL?
  // Initial long pressed is triggered twice, fix with requiring release event
  static bool released = true;

  switch (code) {
    case LV_EVENT_LONG_PRESSED:
      if (released) {
        if (ui->isShutterLocked()) {
          ui->shutterUnlock(control);
        } else {
          ui->shutterLock(control);
        }
        released = false;
      }
      break;
    case LV_EVENT_RELEASED:
      released = true;
      break;
    default:
      break;
  }
}

void UI::prepareShutterControl(void) {
  lv_obj_add_event_cb(
      m_Left,
      [](lv_event_t *e) {
        auto *ui = static_cast<UI *>(lv_event_get_user_data(e));
        lv_obj_t *back = lv_menu_get_main_header_back_button(m_MainMenu.main);
        lv_obj_send_event(back, LV_EVENT_CLICKED, m_MainMenu.main);
        ui->configMenuControl();
      },
      LV_EVENT_CLICKED, this);

  lv_obj_add_event_cb(m_OK, handleShutter, LV_EVENT_ALL, this);

  lv_obj_add_event_cb(m_Right, handleFocus, LV_EVENT_ALL, this);
}

lv_obj_t *UI::addIcon(const char *symbol) {
  lv_obj_t *icon = lv_image_create(m_Header);

  setIcon(icon, symbol);

  return icon;
}

void UI::setIcon(lv_obj_t *icon, const char *symbol) {
  lv_image_set_src(icon, symbol);
}

lv_obj_t *UI::addMenuItem(const menu_t &menu, const char *symbol, const char *text, bool checkbox) {
  lv_obj_t *cont = lv_menu_cont_create(menu.page);
  lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

#if defined(FURBLE_M5STICKC)
  // screen is too small for icons
#else
  if (symbol) {
    lv_obj_t *icon = lv_image_create(cont);
    lv_image_set_src(icon, symbol);
  }
#endif

  if (checkbox) {
    lv_obj_t *check = lv_checkbox_create(cont);
    lv_checkbox_set_text(check, text);
    lv_obj_set_width(check, LV_PCT(100));
    lv_obj_add_flag(check, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_group_add_obj(menu.group, check);
    return check;
  } else {
    lv_obj_t *label = lv_label_create(cont);
    lv_label_set_text(label, text);
    lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(label, LV_PCT(100));
    lv_obj_add_flag(cont, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_group_add_obj(menu.group, cont);
  }

  return cont;
}

void UI::addSettingItem(lv_obj_t *page, const char *symbol, Settings::type_t setting) {
  lv_obj_t *obj = lv_menu_cont_create(page);
  lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_ROW_WRAP);

  if (symbol) {
    lv_obj_t *icon = lv_image_create(obj);
    lv_image_set_src(icon, symbol);
  }

  auto &s = Settings::get(setting);

  lv_obj_t *label = lv_label_create(obj);
  lv_label_set_text(label, s.name);
  lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
  lv_obj_set_flex_grow(label, 1);

  lv_obj_t *sw = lv_switch_create(obj);
  bool enable = Settings::load<bool>(setting);
  lv_obj_add_state(sw, enable ? LV_STATE_CHECKED : 0);
  lv_obj_add_event_cb(
      sw,
      [](lv_event_t *e) {
        const auto *setting = static_cast<Settings::setting_t *>(lv_event_get_user_data(e));
        lv_obj_t *sw = static_cast<lv_obj_t *>(lv_event_get_target(e));
        Settings::save<bool>(setting->type, lv_obj_has_state(sw, LV_STATE_CHECKED));
      },
      LV_EVENT_VALUE_CHANGED, const_cast<Settings::setting_t *>(&s));

  if (setting == Settings::GPS) {
    lv_obj_add_event_cb(
        sw,
        [](lv_event_t *e) {
          auto *status = static_cast<status_t *>(lv_event_get_user_data(e));
          status->gps->reloadSetting();
          if (status->gps->isEnabled()) {
            lv_obj_clear_flag(status->gpsData, LV_OBJ_FLAG_HIDDEN);
          } else {
            lv_obj_add_flag(status->gpsData, LV_OBJ_FLAG_HIDDEN);
          }
        },
        LV_EVENT_VALUE_CHANGED, &m_Status);
  }

  if (setting == Settings::RECONNECT) {
    if (!enable) {
      lv_obj_add_flag(m_Status.reconnectIcon, LV_OBJ_FLAG_HIDDEN);
    }

    lv_obj_add_event_cb(
        sw,
        [](lv_event_t *e) {
          auto *sw = static_cast<lv_obj_t *>(lv_event_get_target(e));
          auto *status = static_cast<status_t *>(lv_event_get_user_data(e));
          if (lv_obj_has_state(sw, LV_STATE_CHECKED)) {
            lv_obj_clear_flag(status->reconnectIcon, LV_OBJ_FLAG_HIDDEN);
          } else {
            lv_obj_add_flag(status->reconnectIcon, LV_OBJ_FLAG_HIDDEN);
          }
        },
        LV_EVENT_VALUE_CHANGED, &m_Status);
  }
}

lv_obj_t *UI::addCameraItem(Camera *camera, const menu_t &menu, const CameraListMode_t mode) {
  bool checkbox = (mode == MODE_MULTICONNECT);

  lv_obj_t *item = addMenuItem(menu, NULL, camera->getName().c_str(), checkbox);

  switch (mode) {
    case MODE_DELETE:
      lv_obj_add_event_cb(
          item,
          [](lv_event_t *e) {
            auto *camera = static_cast<Camera *>(lv_event_get_user_data(e));

            CameraList::remove(camera);
            refreshDelete();
          },
          LV_EVENT_CLICKED, camera);
      break;
    case MODE_SCAN:
    case MODE_CONNECT:
      lv_obj_add_event_cb(
          item,
          [](lv_event_t *e) {
            auto *camera = static_cast<Camera *>(lv_event_get_user_data(e));
            camera->setActive(true);

            doConnect(e);
          },
          LV_EVENT_CLICKED, camera);
      break;
    case MODE_MULTICONNECT:
      lv_obj_add_event_cb(
          item,
          [](lv_event_t *e) {
            auto *camera = static_cast<Camera *>(lv_event_get_user_data(e));
            auto *check = static_cast<lv_obj_t *>(lv_event_get_target(e));

            camera->setActive(lv_obj_has_state(check, LV_STATE_CHECKED));
          },
          LV_EVENT_VALUE_CHANGED, camera);
      break;
  }

  return item;
}

UI::menu_t &UI::addMenu(const char *name, const char *symbol, bool button, const menu_t &parent) {
  menu_t &menu = m_Menu.at(name);
  menu.main = m_MainMenu.main;
  menu.group = m_Group;
  menu.page = lv_menu_page_create(m_MainMenu.main, name);

  if (button) {
    menu.button = addMenuItem(parent, symbol, name);
  }

  return menu;
}

void UI::addMainMenu(void) {
  lv_obj_update_layout(m_Content);
  lv_obj_set_scrollbar_mode(m_Content, LV_SCROLLBAR_MODE_OFF);

  m_MainMenu.main = lv_menu_create(m_Content);
  if (M5.Touch.isEnabled()) {
    lv_menu_set_mode_header(m_MainMenu.main, LV_MENU_HEADER_BOTTOM_FIXED);
  } else {
    lv_menu_set_mode_header(m_MainMenu.main, LV_MENU_HEADER_TOP_FIXED);
  }

  lv_menu_set_mode_root_back_button(m_MainMenu.main, LV_MENU_ROOT_BACK_BUTTON_DISABLED);
#if defined(FURBLE_M5COREX)
  // M5StickC displays are too narrow
  lv_obj_t *back = lv_menu_get_main_header_back_button(m_MainMenu.main);
  lv_obj_t *back_label = lv_label_create(back);
  lv_label_set_text(back_label, "Back");
#endif

  lv_obj_set_size(m_MainMenu.main, LV_PCT(100), LV_PCT(100));
  lv_obj_center(m_MainMenu.main);

  m_MainMenu.page = lv_menu_page_create(m_MainMenu.main, NULL);
  m_MainMenu.group = m_Group;

  addConnectMenu();
  addScanMenu();
  addDeleteMenu();
  addSettingsMenu();
  addConnectedMenu();

  m_PowerOff = addMenuItem(m_MainMenu, LV_SYMBOL_POWER, "Power Off");
  lv_obj_add_event_cb(
      m_PowerOff, [](lv_event_t *e) { M5.Power.powerOff(); }, LV_EVENT_CLICKED, NULL);

  lv_obj_add_event_cb(
      m_MainMenu.main,
      [](lv_event_t *e) {
        auto *target = static_cast<lv_obj_t *>(lv_event_get_target(e));
        auto *page = lv_menu_get_cur_main_page(target);
        auto *back = lv_menu_get_main_header_back_button(m_MainMenu.main);
        auto &scan = Scan::getInstance();

        if (page == m_MainMenu.page) {
          // Hide connect & delete if there are zero saved
          CameraList::load();
          if (CameraList::size() == 0) {
            lv_obj_add_flag(m_Menu.at(m_ConnectStr).button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(m_Menu.at(m_DeleteStr).button, LV_OBJ_FLAG_HIDDEN);
          } else {
            lv_obj_clear_flag(m_Menu.at(m_ConnectStr).button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(m_Menu.at(m_DeleteStr).button, LV_OBJ_FLAG_HIDDEN);
          }

          // Ensure no active scans
          scan.stop();

          // Enable Back button
          if (lv_obj_has_state(back, LV_STATE_DISABLED)) {
            lv_obj_remove_state(back, LV_STATE_DISABLED);
          }
        } else if (page == m_Menu.at(m_DeleteStr).page) {
        } else if (page == m_Menu.at(m_ScanStr).page) {
          menu_t &menu = m_Menu.at(m_ScanStr);
          lv_obj_clean(menu.page);
          CameraList::clear();

          if (Settings::load<bool>(Settings::FAUXNY)) {
            CameraList::addFauxNY();
            updateItems(menu);
          }

          scan.clear();
          scan.start(
              [](void *param) {
                auto *menu = static_cast<menu_t *>(param);
                // Can be called asychronously from NimBLE scan thread,
                m_Mutex.lock();
                updateItems(*menu);
                m_Mutex.unlock();
              },
              &menu);

          lv_timer_set_user_data(m_ConnectTimer, (void *)(m_ScanStr));
        } else if (page == m_Menu.at(m_SettingsStr).page) {
        } else if (page == m_Menu.at(m_ConnectStr).page) {
        } else if (page == m_Menu.at(m_ConnectedStr).page) {
          // Ensure no active scans
          scan.stop();

          // hide and disable back button
          lv_obj_add_state(back, LV_STATE_DISABLED);
          lv_obj_add_flag(back, LV_OBJ_FLAG_HIDDEN);
        } else if (page == m_Menu.at(m_RemoteShutter).page) {
          if (M5.Touch.isEnabled()) {
            // if touch screen, enable back
            lv_obj_remove_state(back, LV_STATE_DISABLED);
          } else {
            // hide the back button
            lv_obj_add_flag(back, LV_OBJ_FLAG_HIDDEN);
          }
        } else if (page == m_Menu.at(m_IntervalometerStr).page) {
          // always display 'Back' in intervalometer
          lv_obj_remove_state(back, LV_STATE_DISABLED);
        } else if (page == m_Menu.at(m_IntervalometerRunStr).page) {
          // disable 'Back' when intervalometer is running
          lv_obj_remove_state(back, LV_STATE_DISABLED);
        }
      },
      LV_EVENT_VALUE_CHANGED, NULL);

  lv_obj_add_event_cb(
      m_MainMenu.main,
      [](lv_event_t *e) {
        auto *ui = static_cast<UI *>(lv_event_get_user_data(e));
        auto *target = static_cast<lv_obj_t *>(lv_event_get_current_target(e));
        auto *page = lv_menu_get_cur_main_page(target);

        if (page == m_MainMenu.page) {
        } else if (page == m_Menu.at(m_DeleteStr).page) {
        } else if (page == m_Menu.at(m_SettingsStr).page) {
        } else if (page == m_Menu.at(m_ConnectedStr).page) {
          lv_obj_t *back = lv_menu_get_main_header_back_button(m_MainMenu.main);
          if (lv_obj_has_state(back, LV_STATE_FOCUS_KEY)) {
            lv_group_focus_next(lv_group_get_default());
          }
          ui->shutterUnlock(Control::getInstance());
        }
      },
      LV_EVENT_CLICKED, this);

  lv_menu_set_page(m_MainMenu.main, m_MainMenu.page);
}

void UI::displayNavigationBar(bool show) {
  if (!M5.Touch.isEnabled()) {
    if (show) {
      lv_obj_clear_flag(m_NavBar, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(m_Left, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(m_OK, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(m_Right, LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_add_flag(m_NavBar, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(m_Left, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(m_OK, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(m_Right, LV_OBJ_FLAG_HIDDEN);
    }
  }
}

void UI::configShutterControl(void) {
  if (!M5.Touch.isEnabled()) {
    lv_obj_set_style_bg_image_src(m_Right, LV_SYMBOL_EYE_OPEN, 0);
    lv_obj_set_style_bg_image_src(m_OK, LV_SYMBOL_IMAGE, 0);

    lv_indev_set_type(m_ButtonL, LV_INDEV_TYPE_BUTTON);
    lv_indev_set_type(m_ButtonO, LV_INDEV_TYPE_BUTTON);
    lv_indev_set_type(m_ButtonR, LV_INDEV_TYPE_BUTTON);

    // map indev to button coords
    lv_area_t left;
    lv_area_t ok;
    lv_area_t right;

    lv_obj_get_coords(m_Left, &left);
    lv_obj_get_coords(m_OK, &ok);
    lv_obj_get_coords(m_Right, &right);

    static const lv_point_t leftPoint[] = {
        {(left.x1 + left.x2) / 2, (left.y1 + left.y2) / 2}
    };
    static const lv_point_t okPoint[] = {
        {(ok.x1 + ok.x2) / 2, (ok.y1 + ok.y2) / 2}
    };
    static const lv_point_t rightPoint[] = {
        {(right.x1 + right.x2) / 2, (right.y1 + right.y2) / 2}
    };

    lv_indev_set_button_points(m_ButtonL, leftPoint);
    lv_indev_set_button_points(m_ButtonO, okPoint);
    lv_indev_set_button_points(m_ButtonR, rightPoint);
  }
}

void UI::configMenuControl(void) {
  if (!M5.Touch.isEnabled()) {
    lv_obj_set_style_bg_image_src(m_Left, LV_SYMBOL_LEFT, 0);
    lv_obj_set_style_bg_image_src(m_OK, LV_SYMBOL_OK, 0);
    lv_obj_set_style_bg_image_src(m_Right, LV_SYMBOL_RIGHT, 0);

    lv_indev_set_type(m_ButtonL, LV_INDEV_TYPE_ENCODER);
    lv_indev_set_type(m_ButtonO, LV_INDEV_TYPE_ENCODER);
    lv_indev_set_type(m_ButtonR, LV_INDEV_TYPE_ENCODER);
  }
}

void UI::showShutterIntervalometer(bool show) {
  if (show) {
    lv_obj_clear_flag(m_IntervalStart, LV_OBJ_FLAG_HIDDEN);
    lv_group_focus_obj(m_IntervalStart);
  } else {
    lv_obj_add_flag(m_IntervalStart, LV_OBJ_FLAG_HIDDEN);
  }
}

void UI::connectTimerHandler(lv_timer_t *timer) {
  auto &control = Control::getInstance();
  Camera *camera = nullptr;
  auto state = control.getState();

  switch (state) {
    case Control::STATE_CONNECT:
      break;

    case Control::STATE_CONNECTING:
      camera = control.getConnectingCamera();

      if (lv_obj_has_flag(m_ConnectMessageBox, LV_OBJ_FLAG_HIDDEN)) {
        // hide menu, unhide message box
        lv_obj_add_flag(m_MainMenu.main, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(m_ConnectMessageBox, LV_OBJ_FLAG_HIDDEN);
        UI::displayNavigationBar(false);
        lv_group_focus_obj(m_ConnectCancel);
      }

      if (camera != nullptr) {
        lv_label_set_text(m_ConnectLabel, camera->getName().c_str());
        lv_bar_set_value(m_ConnectBar, camera->getConnectProgress(), LV_ANIM_ON);
      }
      break;

    case Control::STATE_CONNECT_FAILED:
      ESP_LOGE("ui", "Connection failed.");
      doDisconnect();
      break;

    case Control::STATE_ACTIVE:
      if (!lv_obj_has_flag(m_ConnectMessageBox, LV_OBJ_FLAG_HIDDEN)) {
        // if from scan, save the connection
        const char *from = static_cast<const char *>(lv_timer_get_user_data(timer));
        if (from == m_ScanStr) {
          for (const auto &target : control.getTargets()) {
            CameraList::save(target->getCamera());
          }
        }

        // everything connected, display menu, hide connection message box
        lv_obj_add_flag(m_ConnectMessageBox, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(m_MainMenu.main, LV_OBJ_FLAG_HIDDEN);
        UI::displayNavigationBar(true);
        lv_group_focus_next(lv_group_get_default());
      }
      break;

    case Control::STATE_IDLE:
      // nothing to do
      break;
  }
}

void UI::intervalometer(lv_timer_t *timer) {
  auto &control = Control::getInstance();
  auto *interval = static_cast<Intervalometer *>(lv_timer_get_user_data(timer));
  uint32_t next = 0;

  static uint32_t count = 0;

  if (interval->m_Count.m_SpinValue.m_Unit == SpinValue::UNIT_INF) {
    lv_label_set_text_fmt(m_IntervalCountLabel, "%09u", count);
  } else {
    lv_label_set_text_fmt(m_IntervalCountLabel, "%03u/%03u", count,
                          interval->m_Count.m_SpinValue.m_Value);
  }

  switch (interval->m_State) {
    case Intervalometer::STATE_IDLE:
      count = 0;
      lv_label_set_text(m_IntervalStateLabel, "IDLE");
      lv_timer_ready(timer);
      interval->m_State = Intervalometer::STATE_SHUTTER_OPEN;
      break;

    case Intervalometer::STATE_SHUTTER_OPEN:
      count++;
      lv_label_set_text(m_IntervalStateLabel, "SHUTTER");
      control.sendCommand(Control::CMD_SHUTTER_PRESS);
      next = interval->m_Shutter.m_SpinValue.toMilliseconds();
      interval->m_State = Intervalometer::STATE_WAIT;
      break;

    case Intervalometer::STATE_WAIT:
      lv_label_set_text(m_IntervalStateLabel, "DELAY");
      control.sendCommand(Control::CMD_SHUTTER_RELEASE);
      next = interval->m_Delay.m_SpinValue.toMilliseconds();
      if (count >= interval->m_Count.m_SpinValue.m_Value) {
        interval->m_State = Intervalometer::STATE_FINISHED;
      } else {
        interval->m_State = Intervalometer::STATE_SHUTTER_OPEN;
      }
      break;

    case Intervalometer::STATE_FINISHED:
      lv_label_set_text(m_IntervalStateLabel, "FINISHED");
      next = 0;
      lv_timer_pause(timer);
      break;
  }

  if (next > 0) {
    lv_timer_set_period(timer, next);
    m_IntervalNext = tick() + next;
  }
}

void UI::doConnect(lv_event_t *e) {
  auto &control = Control::getInstance();

  // activate selected cameras
  for (auto n = 0; n < CameraList::size(); n++) {
    auto *camera = CameraList::get(n);
    if (camera->isActive()) {
      control.addActive(camera);
    }
  }

  lv_obj_add_event_cb(
      m_ConnectCancel, [](lv_event_t *e) { doDisconnect(); }, LV_EVENT_CLICKED, NULL);

  control.connectAll(Settings::load<bool>(Settings::RECONNECT));
  lv_timer_reset(m_ConnectTimer);
  lv_timer_resume(m_ConnectTimer);

  menu_t &menu = m_Menu.at(m_ConnectedStr);
  lv_menu_set_page(m_MainMenu.main, menu.page);
}

void UI::doDisconnect(void) {
  lv_timer_pause(m_ConnectTimer);
  Control::getInstance().disconnect();

  lv_obj_add_flag(m_ConnectMessageBox, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(m_MainMenu.main, LV_OBJ_FLAG_HIDDEN);

  lv_menu_clear_history(m_MainMenu.main);
  lv_menu_set_page(m_MainMenu.main, m_MainMenu.page);
  menu_t &connect = m_Menu.at(m_ConnectStr);
  lv_group_focus_obj(connect.button);
}

UI::menu_t &UI::addConnectedMenu(void) {
  menu_t &menuConnected = addMenu(m_ConnectedStr, NULL, false);
  menu_t &menuShutter = addMenu(m_RemoteShutter, LV_SYMBOL_IMAGE, true, menuConnected);
  menu_t &menuInterval = addMenu(m_RemoteInterval, LV_SYMBOL_LOOP, true, menuConnected);
  lv_obj_t *disconnect = addMenuItem(menuConnected, LV_SYMBOL_CLOSE, "Disconnect");

  if (M5.Touch.isEnabled()) {
    // add remote shutter control for touch screens
    lv_obj_t *cont = lv_menu_cont_create(menuShutter.page);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
    lv_obj_center(cont);

    std::array<std::tuple<lv_obj_t *, lv_obj_t *, const char *, const char *>, 3> buttons = {
        {
         {nullptr, nullptr, "Shutter", LV_SYMBOL_IMAGE},
         {nullptr, nullptr, "Focus", LV_SYMBOL_EYE_OPEN},
         {nullptr, nullptr, "Shutter\nLock", LV_SYMBOL_NEXT},
         }
    };

    for (auto &i : buttons) {
      auto &buttonCont = std::get<0>(i);

      buttonCont = lv_obj_create(cont);
      lv_obj_set_layout(buttonCont, LV_LAYOUT_FLEX);
      lv_obj_set_flex_flow(buttonCont, LV_FLEX_FLOW_COLUMN);
      lv_obj_set_flex_align(buttonCont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                            LV_FLEX_ALIGN_CENTER);
      lv_obj_clear_flag(buttonCont, LV_OBJ_FLAG_SCROLLABLE);
      lv_obj_set_size(buttonCont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

      auto &button = std::get<1>(i);
      button = lv_button_create(buttonCont);
      lv_obj_set_style_bg_image_src(button, std::get<3>(i), 0);
      lv_obj_set_size(button, 64, 64);

      lv_obj_t *label = lv_label_create(buttonCont);
      lv_label_set_text(label, std::get<2>(i));
      lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    }

    m_OK = std::get<1>(buttons[0]);
    m_Right = std::get<1>(buttons[1]);
    auto lockButton = std::get<1>(buttons[2]);

    lv_obj_add_event_cb(m_OK, handleShutter, LV_EVENT_ALL, this);

    lv_obj_add_event_cb(m_Right, handleFocus, LV_EVENT_ALL, this);

    lv_obj_add_event_cb(lockButton, handleShutterLock, LV_EVENT_ALL, this);
  } else {
    // add remote shutter text for buttons
    lv_obj_t *txt = lv_label_create(menuShutter.page);
    lv_label_set_text(txt, LV_SYMBOL_IMAGE ": Shutter\n" LV_SYMBOL_EYE_OPEN
                                           ": Focus\n" LV_SYMBOL_EYE_OPEN " + " LV_SYMBOL_IMAGE
                                           ":\n Shutter Lock\n\n"

                      LV_SYMBOL_LEFT ": Back");
    lv_obj_set_size(txt, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_center(txt);
  }

  lv_obj_add_event_cb(
      menuShutter.button,
      [](lv_event_t *e) {
        auto *ui = static_cast<UI *>(lv_event_get_user_data(e));
        ui->configShutterControl();
      },
      LV_EVENT_CLICKED, this);

  // add intervalometer control
  menu_t &menuIntervalometer = m_Menu.at(m_IntervalometerStr);
  lv_menu_set_load_page_event(menuIntervalometer.main, menuInterval.button,
                              menuIntervalometer.page);

  lv_obj_add_event_cb(
      menuInterval.button,
      [](lv_event_t *e) {
        auto *ui = static_cast<UI *>(lv_event_get_user_data(e));
        ui->showShutterIntervalometer(true);
      },
      LV_EVENT_CLICKED, this);

  // add disconnect control
  lv_obj_add_event_cb(
      disconnect,
      [](lv_event_t *e) {
        auto *ui = static_cast<UI *>(lv_event_get_user_data(e));
        doDisconnect();
        ui->showShutterIntervalometer(false);
      },
      LV_EVENT_CLICKED, this);

  lv_menu_set_load_page_event(menuShutter.main, menuShutter.button, menuShutter.page);

  return menuConnected;
}

void UI::addConnectMenu(void) {
  menu_t &menu = addMenu(m_ConnectStr, LV_SYMBOL_WIFI);

  // refresh connection list every time
  lv_obj_add_event_cb(
      menu.button,
      [](lv_event_t *e) {
        auto &menu = m_Menu.at(m_ConnectStr);
        bool multiconnect = Settings::load<bool>(Settings::MULTICONNECT);

        lv_obj_clean(menu.page);

        CameraList::load();
        for (size_t n = 0; n < CameraList::size(); n++) {
          auto *camera = CameraList::get(n);
          addCameraItem(camera, menu, multiconnect ? MODE_MULTICONNECT : MODE_CONNECT);
        }

        if (multiconnect) {
          lv_obj_t *multibutton = lv_button_create(menu.page);
          lv_obj_t *label = lv_label_create(multibutton);
          lv_label_set_text(label, "Multi-Connect");
          lv_obj_center(label);
          lv_obj_add_event_cb(multibutton, doConnect, LV_EVENT_CLICKED, e);
        }

        lv_timer_set_user_data(m_ConnectTimer, NULL);
      },
      LV_EVENT_CLICKED, NULL);

  // create, but immediately hide the connect message box
  m_ConnectMessageBox = lv_msgbox_create(m_Screen);
  lv_msgbox_add_title(m_ConnectMessageBox, "Connecting");
  lv_obj_set_width(m_ConnectMessageBox, LV_PCT(100));
  lv_obj_update_layout(m_ConnectMessageBox);
  lv_obj_t *c = lv_msgbox_get_content(m_ConnectMessageBox);
  lv_obj_set_flex_align(c, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);
  m_ConnectLabel = lv_label_create(c);
  lv_label_set_long_mode(m_ConnectLabel, LV_LABEL_LONG_DOT);
  lv_obj_set_width(m_ConnectLabel, LV_PCT(80));

  m_ConnectBar = lv_bar_create(c);
  lv_obj_set_width(m_ConnectBar, LV_PCT(80));
  lv_bar_set_value(m_ConnectBar, 0, LV_ANIM_ON);

  m_ConnectCancel = lv_msgbox_add_footer_button(m_ConnectMessageBox, "Cancel");
  lv_obj_t *footer = lv_msgbox_get_footer(m_ConnectMessageBox);
  lv_obj_update_layout(footer);
  // @todo cancel button bottom is clipped, weird
  // lv_obj_set_height(m_ConnectCancel, LV_SIZE_CONTENT);
  // lv_obj_set_height(footer, lv_obj_get_height(m_ConnectCancel) * 1.2f);

  lv_obj_add_flag(m_ConnectMessageBox, LV_OBJ_FLAG_HIDDEN);

  lv_menu_set_load_page_event(menu.main, menu.button, menu.page);
}

void UI::addScanMenu(void) {
  menu_t &menu = addMenu(m_ScanStr, LV_SYMBOL_EYE_OPEN);

  lv_menu_set_load_page_event(menu.main, menu.button, menu.page);
}

void UI::refreshDelete(void) {
  auto &menu = m_Menu.at(m_DeleteStr);
  lv_obj_clean(menu.page);

  CameraList::load();
  for (size_t n = 0; n < CameraList::size(); n++) {
    auto *camera = CameraList::get(n);
    addCameraItem(camera, menu, MODE_DELETE);
  }
}

void UI::addDeleteMenu(void) {
  menu_t &menu = addMenu(m_DeleteStr, LV_SYMBOL_TRASH);

  // refresh connection list every time
  lv_obj_add_event_cb(menu.button, [](lv_event_t *e) { refreshDelete(); }, LV_EVENT_CLICKED, NULL);

  lv_menu_set_load_page_event(menu.main, menu.button, menu.page);
}

void UI::addGPSMenu(const menu_t &parent) {
  menu_t &menu = addMenu(m_GPSStr, NULL, true, parent);
  addSettingItem(menu.page, NULL, Settings::GPS);
  lv_menu_set_load_page_event(menu.main, menu.button, menu.page);

  menu_t &gpsData = addMenu(m_GPSDataStr, NULL, true, menu);
  m_Status.gpsData = gpsData.button;
  if (!m_Status.gps->isEnabled()) {
    lv_obj_add_flag(m_Status.gpsData, LV_OBJ_FLAG_HIDDEN);
  }

  static lv_timer_t *timer = lv_timer_create(
      [](lv_timer_t *t) {
        auto *gpsData = static_cast<menu_t *>(lv_timer_get_user_data(t));
        auto &gps = GPS::getInstance().get();
        static lv_obj_t *valid = lv_label_create(gpsData->page);
        lv_label_set_text_fmt(valid, "%s (%u)", gps.location.isValid() ? "Valid" : "Invalid",
                              gps.satellites.value());

        static lv_obj_t *lat = lv_label_create(gpsData->page);
        lv_label_set_text_fmt(lat, "%.2f°", gps.location.lat());

        static lv_obj_t *lon = lv_label_create(gpsData->page);
        lv_label_set_text_fmt(lon, "%.2f°", gps.location.lng());

        static lv_obj_t *alt = lv_label_create(gpsData->page);
        lv_label_set_text_fmt(alt, "%.2f m", gps.altitude.meters());

        static lv_obj_t *date = lv_label_create(gpsData->page);
        lv_label_set_text_fmt(date, "%4u-%02u-%02u", gps.date.year(), gps.date.month(),
                              gps.date.day());
        static lv_obj_t *time = lv_label_create(gpsData->page);
        lv_label_set_text_fmt(time, "%02u:%02u:%02u", gps.time.hour(), gps.time.minute(),
                              gps.time.second());
      },
      1000, &gpsData);
  lv_timer_pause(timer);

  // start the update timer on 'GPS Data' button press
  lv_obj_add_event_cb(
      gpsData.button,
      [](lv_event_t *e) {
        auto *timer = static_cast<lv_timer_t *>(lv_event_get_user_data(e));
        lv_timer_resume(timer);
        menu_t *menu = static_cast<menu_t *>(lv_timer_get_user_data(timer));
        lv_obj_add_event_cb(menu->main, gpsDataStop, LV_EVENT_CLICKED, timer);
      },
      LV_EVENT_CLICKED, timer);

  lv_menu_set_load_page_event(gpsData.main, gpsData.button, gpsData.page);
}

void UI::gpsDataStop(lv_event_t *e) {
  auto *timer = static_cast<lv_timer_t *>(lv_event_get_user_data(e));
  auto *target = static_cast<lv_obj_t *>(lv_event_get_target(e));
  lv_timer_pause(timer);
  lv_obj_remove_event_cb(target, gpsDataStop);
}

void UI::addFeaturesMenu(const menu_t &parent) {
  menu_t &menu = addMenu(m_FeaturesStr, NULL, true, parent);

  addSettingItem(menu.page, NULL, Settings::FAUXNY);
  addSettingItem(menu.page, NULL, Settings::RECONNECT);
  addSettingItem(menu.page, NULL, Settings::MULTICONNECT);

  lv_menu_set_load_page_event(menu.main, menu.button, menu.page);
}

lv_obj_t *UI::addSpinItem(lv_obj_t *page, const char *item, Intervalometer::Spinner &spinner) {
  spinner.m_Button = lv_menu_cont_create(page);
  lv_obj_set_flex_flow(spinner.m_Button, LV_FLEX_FLOW_ROW_WRAP);

  spinner.m_Label = lv_label_create(spinner.m_Button);
  lv_label_set_text(spinner.m_Label, item);
#if defined(FURBLE_M5COREX)
  lv_obj_set_flex_grow(spinner.m_Label, 1);
#endif

  spinner.m_Value = lv_label_create(spinner.m_Button);
  lv_label_set_long_mode(spinner.m_Value, LV_LABEL_LONG_SCROLL_CIRCULAR);
#if !defined(FURBLE_M5COREX)
  lv_obj_set_flex_grow(spinner.m_Value, 1);
#endif

  lv_obj_add_event_cb(
      spinner.m_Value,
      [](lv_event_t *e) {
        auto *spinner = static_cast<Intervalometer::Spinner *>(lv_event_get_user_data(e));
        spinner->update();
      },
      LV_EVENT_REFRESH, &spinner);

  return spinner.m_Button;
}

void UI::addSpinnerPage(const menu_t &parent, const char *item, Intervalometer::Spinner &spinner) {
  menu_t &menu = addMenu(item, NULL, false, parent);
  menu.button = addSpinItem(parent.page, item, spinner);

  lv_group_add_obj(m_Group, menu.button);

  lv_obj_set_flex_flow(menu.page, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_scrollbar_mode(menu.page, LV_SCROLLBAR_MODE_OFF);
  spinner.m_RowInfinite = lv_obj_create(menu.page);
  lv_obj_set_size(spinner.m_RowInfinite, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_set_layout(spinner.m_RowInfinite, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(spinner.m_RowInfinite, LV_FLEX_FLOW_ROW);
  lv_obj_t *label = lv_label_create(spinner.m_RowInfinite);
  lv_label_set_text(label, "Infinite");
  lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
  lv_obj_set_flex_grow(label, 1);
  spinner.m_SwitchInfinite = lv_switch_create(spinner.m_RowInfinite);
  if (spinner.m_SpinValue.m_Unit == SpinValue::UNIT_INF) {
    lv_obj_add_state(spinner.m_SwitchInfinite, LV_STATE_CHECKED);
  } else {
    lv_obj_remove_state(spinner.m_SwitchInfinite, LV_STATE_CHECKED);
  }

  if (!spinner.m_Infinite) {
    lv_obj_add_flag(spinner.m_RowInfinite, LV_OBJ_FLAG_HIDDEN);
  }

  lv_obj_add_event_cb(
      spinner.m_SwitchInfinite,
      [](lv_event_t *e) {
        auto *spinner = static_cast<Intervalometer::Spinner *>(lv_event_get_user_data(e));
        spinner->update();
      },
      LV_EVENT_VALUE_CHANGED, &spinner);

  spinner.m_RowSpinners = lv_obj_create(menu.page);
  lv_obj_set_size(spinner.m_RowSpinners, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_set_layout(spinner.m_RowSpinners, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(spinner.m_RowSpinners, LV_FLEX_FLOW_ROW);

  switch (M5.getBoard()) {
    case m5::board_t::board_M5StickC:
    case m5::board_t::board_M5StickCPlus:
    case m5::board_t::board_M5StickCPlus2:
      lv_obj_set_flex_align(spinner.m_RowSpinners, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                            LV_FLEX_ALIGN_CENTER);
      break;

    default:
      lv_obj_set_flex_align(spinner.m_RowSpinners, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                            LV_FLEX_ALIGN_CENTER);
      break;
  }

  for (auto &r : spinner.m_Roller) {
    r = lv_roller_create(spinner.m_RowSpinners);
    lv_obj_add_flag(r, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_roller_set_options(r, Intervalometer::Spinner::m_SpinDigitRoller, LV_ROLLER_MODE_INFINITE);

    lv_roller_set_visible_row_count(r, 2);
    lv_group_add_obj(m_Group, r);
    lv_obj_add_event_cb(
        r,
        [](lv_event_t *e) {
          auto *spinner = static_cast<Intervalometer::Spinner *>(lv_event_get_user_data(e));
          lv_obj_send_event(spinner->m_Value, LV_EVENT_REFRESH, spinner);
        },
        LV_EVENT_VALUE_CHANGED, &spinner);
  }

  if ((spinner.m_SpinValue.m_Unit != SpinValue::UNIT_NIL)
      && (spinner.m_SpinValue.m_Unit != SpinValue::UNIT_INF)) {
    spinner.m_RollerUnit = lv_roller_create(spinner.m_RowSpinners);
    lv_obj_add_flag(spinner.m_RollerUnit, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_roller_set_options(spinner.m_RollerUnit, Intervalometer::Spinner::m_SpinUnitsRoller,
                          LV_ROLLER_MODE_INFINITE);

    lv_obj_add_event_cb(
        spinner.m_RollerUnit,
        [](lv_event_t *e) {
          auto *spinner = static_cast<Intervalometer::Spinner *>(lv_event_get_user_data(e));
          lv_obj_send_event(spinner->m_Value, LV_EVENT_REFRESH, spinner);
        },
        LV_EVENT_VALUE_CHANGED, &spinner);

    lv_group_add_obj(m_Group, spinner.m_RollerUnit);
  }

  // squeeze width for smaller displays
  switch (M5.getBoard()) {
    case m5::board_t::board_M5StickC:
      lv_obj_set_flex_align(spinner.m_RowSpinners, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                            LV_FLEX_ALIGN_CENTER);
      lv_obj_add_flag(spinner.m_RowSpinners, LV_OBJ_FLAG_SCROLLABLE);
      lv_obj_set_scrollbar_mode(spinner.m_RowSpinners, LV_SCROLLBAR_MODE_OFF);
      lv_obj_set_scroll_dir(spinner.m_RowSpinners, LV_DIR_HOR);
      __attribute__((fallthrough));
    case m5::board_t::board_M5StickCPlus:
    case m5::board_t::board_M5StickCPlus2:
      for (auto &r : spinner.m_Roller) {
        lv_obj_set_style_pad_left(r, 2, LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(r, 2, LV_STATE_DEFAULT);
      }

      if (spinner.m_SpinValue.m_Unit != SpinValue::UNIT_NIL) {
        lv_obj_set_style_pad_left(spinner.m_RollerUnit, 2, LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(spinner.m_RollerUnit, 2, LV_STATE_DEFAULT);
      }
      break;
    default:
      break;
  }

  spinner.updateLabels();

  lv_menu_set_load_page_event(menu.main, menu.button, menu.page);
}

void UI::addIntervalometerMenu(const menu_t &parent) {
  menu_t &menu = addMenu(m_IntervalometerStr, NULL, true, parent);
  menu_t &menuIntervalRun = addMenu(m_IntervalometerRunStr, NULL, false, menu);

  m_IntervalStart = addMenuItem(menu, NULL, "Start");
  addSpinnerPage(menu, m_IntervalCountStr, m_Intervalometer.m_Count);
  addSpinnerPage(menu, m_IntervalDelayStr, m_Intervalometer.m_Delay);
  addSpinnerPage(menu, m_IntervalShutterStr, m_Intervalometer.m_Shutter);

  m_Intervalometer.m_Count.update();

  lv_obj_add_flag(m_IntervalStart, LV_OBJ_FLAG_HIDDEN);

  lv_obj_add_event_cb(
      m_IntervalStart,
      [](lv_event_t *e) {
        auto *timer = static_cast<lv_timer_t *>(lv_event_get_user_data(e));
        auto *interval = static_cast<Intervalometer *>(lv_timer_get_user_data(timer));

        interval->m_State = Intervalometer::STATE_IDLE;
        lv_timer_resume(timer);

        lv_timer_resume(m_IntervalPageRefresh);
        lv_timer_ready(m_IntervalPageRefresh);
      },
      LV_EVENT_CLICKED, m_IntervalTimer);

  lv_obj_t *cont = lv_menu_cont_create(menuIntervalRun.page);
  lv_obj_set_height(cont, LV_PCT(100));
  lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);

  m_IntervalStateLabel = lv_label_create(cont);
  m_IntervalCountLabel = lv_label_create(cont);
  m_IntervalRemainingLabel = lv_label_create(cont);

  lv_obj_t *stop = lv_button_create(cont);
  lv_obj_t *stopLabel = lv_label_create(stop);
  lv_label_set_text(stopLabel, "Stop");
  lv_obj_add_event_cb(
      stop,
      [](lv_event_t *e) {
        auto &control = Control::getInstance();

        // pause all interval timers
        lv_timer_pause(m_IntervalTimer);
        lv_timer_pause(m_IntervalPageRefresh);

        // release shutter and exit
        control.sendCommand(Control::CMD_SHUTTER_RELEASE);
        lv_obj_t *back = lv_menu_get_main_header_back_button(m_MainMenu.main);
        lv_obj_send_event(back, LV_EVENT_CLICKED, m_MainMenu.main);
      },
      LV_EVENT_CLICKED, NULL);

  m_IntervalPageRefresh = lv_timer_create(
      [](lv_timer_t *timer) {
        uint32_t now = tick();
        uint32_t remaining = m_IntervalNext > now ? m_IntervalNext - now : 0;
        SpinValue::hms_t hms = SpinValue::toHMS(remaining);
        lv_label_set_text_fmt(m_IntervalRemainingLabel, "%02u:%02u:%02u", hms.hours, hms.minutes,
                              hms.seconds);
      },
      500, NULL);
  lv_timer_pause(m_IntervalPageRefresh);

  lv_menu_set_load_page_event(menuIntervalRun.main, m_IntervalStart, menuIntervalRun.page);

  lv_menu_set_load_page_event(menu.main, menu.button, menu.page);
}

void UI::addBacklightMenu(const menu_t &parent) {
  menu_t &menu = addMenu(m_BacklightStr, NULL, true, parent);
  lv_obj_t *cont = lv_menu_cont_create(menu.page);
  lv_obj_set_height(cont, LV_PCT(100));
  lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);

  // Add brightness control
  lv_obj_t *label = lv_label_create(cont);
  lv_label_set_text(label, "Brightness");
  lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
  lv_obj_set_width(label, LV_PCT(100));

  lv_obj_t *slider = lv_slider_create(cont);
  lv_obj_set_width(slider, LV_PCT(90));

  // m_BrightnessSteps * 16 == 256 == black screen :(
  // limit maximum to m_BrightnessSteps * (16 - 1)
  lv_slider_set_range(slider, m_MinimumBrightness / m_BrightnessSteps, m_BrightnessSteps - 1);

  uint8_t brightness = Settings::load<uint8_t>(Settings::BRIGHTNESS);
  lv_slider_set_value(slider, brightness / m_BrightnessSteps, LV_ANIM_ON);

  lv_obj_add_event_cb(
      slider,
      [](lv_event_t *e) {
        auto *slider = static_cast<lv_obj_t *>(lv_event_get_target(e));
        auto brightness = lv_slider_get_value(slider) * m_BrightnessSteps;
        M5.Display.setBrightness(brightness);
      },
      LV_EVENT_VALUE_CHANGED, NULL);

  lv_obj_add_event_cb(
      slider,
      [](lv_event_t *e) {
        auto *slider = static_cast<lv_obj_t *>(lv_event_get_target(e));
        auto brightness = lv_slider_get_value(slider) * m_BrightnessSteps;
        Settings::save<uint8_t>(Settings::BRIGHTNESS, brightness);
      },
      LV_EVENT_RELEASED, NULL);

  // Add inactivity timeout control
  label = lv_label_create(cont);
  lv_label_set_text(label, "Inactivity timeout");
  lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
  lv_obj_set_width(label, LV_PCT(100));

  lv_obj_t *roller = lv_roller_create(cont);
  lv_obj_set_width(roller, LV_PCT(90));
  lv_roller_set_options(roller, "Never\n30 secs\n60 secs", LV_ROLLER_MODE_INFINITE);
  lv_roller_set_visible_row_count(roller, 2);
  uint8_t inactivity = Settings::load<uint8_t>(Settings::INACTIVITY);
  lv_roller_set_selected(roller, inactivity, LV_ANIM_ON);

  lv_obj_add_event_cb(
      roller,
      [](lv_event_t *e) {
        auto *ui = static_cast<UI *>(lv_event_get_user_data(e));
        auto *roller = static_cast<lv_obj_t *>(lv_event_get_target(e));
        auto inactivity = lv_roller_get_selected(roller);
        Settings::save<uint8_t>(Settings::INACTIVITY, inactivity);
        ui->setInactivityTimeout(inactivity);
      },
      LV_EVENT_VALUE_CHANGED, this);

  lv_menu_set_load_page_event(menu.main, menu.button, menu.page);
}

void UI::addThemeMenu(const menu_t &parent) {
  menu_t &menu = addMenu(m_ThemeStr, NULL, true, parent);

  static std::array<std::string, 3> themes = {"Dark", "Default", "Mono Furble"};

  lv_obj_t *cont = lv_menu_cont_create(menu.page);
  lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
  lv_obj_set_layout(cont, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_t *roller = lv_roller_create(cont);
#if !defined(FURBLE_M5COREX)
  lv_obj_set_width(roller, LV_PCT(90));
#endif

  std::string options =
      std::accumulate(std::next(themes.begin()), themes.end(), themes[0],
                      [](const std::string &a, const std::string &b) { return a + "\n" + b; });

  lv_roller_set_options(roller, options.c_str(), LV_ROLLER_MODE_INFINITE);
  lv_roller_set_visible_row_count(roller, 2);

  std::string current = Settings::load<std::string>(Settings::THEME);
  uint32_t index =
      std::distance(themes.data(), std::find(std::begin(themes), std::end(themes), current));
  lv_roller_set_selected(roller, index, LV_ANIM_OFF);

  lv_obj_t *restart = lv_button_create(cont);
  lv_obj_t *label = lv_label_create(restart);
  lv_label_set_text(label, "Restart");

  lv_obj_add_event_cb(
      restart,
      [](lv_event_t *e) {
        auto *roller = static_cast<lv_obj_t *>(lv_event_get_user_data(e));
        auto index = lv_roller_get_selected(roller);
        Settings::save<std::string>(Settings::THEME, themes[index]);
        esp_restart();
      },
      LV_EVENT_CLICKED, roller);

  lv_menu_set_load_page_event(menu.main, menu.button, menu.page);
}

void UI::addTransmitPowerMenu(const menu_t &parent) {
  menu_t &menu = addMenu(m_TransmitPowerStr, NULL, true, parent);
  lv_obj_t *cont = lv_menu_cont_create(menu.page);
  lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
  lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_t *slider = lv_slider_create(cont);
  lv_obj_set_width(slider, LV_PCT(80));
  lv_slider_set_range(slider, 0, 2);

  uint8_t power = Settings::load<uint8_t>(Settings::TX_POWER);
  lv_slider_set_value(slider, power, LV_ANIM_ON);

  lv_obj_add_event_cb(
      slider,
      [](lv_event_t *e) {
        auto *slider = static_cast<lv_obj_t *>(lv_event_get_target(e));
        auto power = lv_slider_get_value(slider);
        auto &control = Control::getInstance();
        Settings::save<uint8_t>(Settings::TX_POWER, power);
        control.setPower(Settings::load<esp_power_level_t>(Settings::TX_POWER));
      },
      LV_EVENT_RELEASED, NULL);

  lv_menu_set_load_page_event(menu.main, menu.button, menu.page);
}

void UI::addAboutMenu(const menu_t &parent) {
  menu_t &menu = addMenu(m_AboutStr, NULL, true, parent);
  lv_obj_t *cont = lv_menu_cont_create(menu.page);
  lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
  lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  lv_obj_t *version = lv_label_create(cont);
  lv_obj_set_width(version, LV_PCT(100));
  lv_label_set_long_mode(version, LV_LABEL_LONG_WRAP);
  lv_label_set_text_fmt(version, "Version:\n%s", FURBLE_VERSION);

  lv_obj_t *id = lv_label_create(cont);
  lv_obj_set_width(id, LV_PCT(100));
  lv_label_set_long_mode(id, LV_LABEL_LONG_WRAP);
  lv_label_set_text_fmt(id, "ID:\n%s", Device::getStringID().c_str());

  lv_menu_set_load_page_event(menu.main, menu.button, menu.page);
}

void UI::addSettingsMenu(void) {
  menu_t &menu = addMenu(m_SettingsStr, LV_SYMBOL_SETTINGS);

  addBacklightMenu(menu);
  addFeaturesMenu(menu);
  addGPSMenu(menu);
  addIntervalometerMenu(menu);
  addThemeMenu(menu);
  addTransmitPowerMenu(menu);
  addAboutMenu(menu);

  lv_menu_set_load_page_event(menu.main, menu.button, menu.page);
}

void UI::updateItems(const menu_t &menu) {
  auto *camera = CameraList::last();

  addCameraItem(camera, menu, MODE_SCAN);
  if (camera->getType() == Camera::Type::MOBILE_DEVICE) {
    // if we get here, mobile devices are already bonded
    CameraList::save(camera);
  }
}

void UI::setInactivityTimeout(uint8_t timeout) {
  m_InactivityTimeout = timeout * 30000;
}

void UI::processInactivity(void) {
  static bool inactive = false;

  if (m_InactivityTimeout > 0) {
    if (lv_disp_get_inactive_time(m_Display) > m_InactivityTimeout) {
      if (!inactive) {
        M5.Display.setBrightness(m_MinimumBrightness);
        inactive = true;
      }
    } else {
      if (inactive) {
        // restore brightness
        auto brightness = Settings::load<uint8_t>(Settings::BRIGHTNESS);
        M5.Display.setBrightness(brightness);
        inactive = false;
      }
    }
  }
}

void UI::task(void) {
  while (true) {
    M5.update();
    if (m_PMICHack && M5.BtnPWR.wasClicked()) {
      // fake PMIC button as actual button
      m_PMICClicked = true;
    }

    // toggle screen lock on power button double click for touch screens
    if (M5.Touch.isEnabled()) {
      if (M5.BtnPWR.wasDoubleClicked()) {
        m_Status.screenLocked = !m_Status.screenLocked;
      }
    }

    m_Mutex.lock();
    lv_task_handler();
    m_Mutex.unlock();

    vTaskDelay(pdMS_TO_TICKS(5));
  }
}
}  // namespace Furble
