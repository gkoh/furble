#include <M5ez.h>

#include <Preferences.h>

#include <algorithm>
#include <string>

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   T H E M E
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ezTheme::begin() {
  if (!ez.themes.size()) {
    ezTheme defaultTheme;
    defaultTheme.add();
  }
  ez.theme = &ez.themes[0];
  Preferences prefs;
  prefs.begin("M5ez", true);  // read-only
  select(prefs.getString("theme", "").c_str());
  prefs.end();
}

void ezTheme::add() {
  ez.themes.push_back(*this);
}

bool ezTheme::select(std::string name) {
  for (uint8_t n = 0; n < ez.themes.size(); n++) {
    if (ez.themes[n].name == name) {
      ez.theme = &ez.themes[n];
      return true;
    }
  }

  return false;
}

void ezTheme::menu() {
  std::string orig_name = ez.theme->name;
  ezMenu thememenu("Theme chooser");
  thememenu.txtSmall();
  thememenu.buttons({"OK", "down"});
  thememenu.downOnLast("first");

  for (uint8_t n = 0; n < ez.themes.size(); n++) {
    thememenu.addItem(ez.themes[n].name);
  }
  thememenu.addItem("Back");
  while (thememenu.runOnce()) {
    if (thememenu.pickName() == "Back")
      break;
    if (thememenu.pick()) {
      ez.theme->select(thememenu.pickName());
    }
  }
  if (ez.theme->name != orig_name) {
    Preferences prefs;
    prefs.begin("M5ez");
    prefs.putString("theme", ez.theme->name.c_str());
    prefs.end();
  }
  return;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   S C R E E N
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint16_t ezScreen::_background;

void ezScreen::begin() {
  _background = ez.theme->background;
  ez.header.begin();
  ez.canvas.begin();
  ez.buttons.begin();
}

uint16_t ezScreen::background() {
  return _background;
}

void ezScreen::clear() {
  clear(ez.theme->background);
}

void ezScreen::clear(uint16_t color) {
  _background = color;
  ez.header.clear(false);
  ez.buttons.clear(false);
  ez.canvas.reset();
  M5.Lcd.fillRect(0, 0, TFT_W, TFT_H, color);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   H E A D E R
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::vector<header_widget_t> ezHeader::_widgets;
bool ezHeader::_shown;
std::string ezHeader::_title;

void ezHeader::begin() {
  _widgets.clear();
  insert(0, "title", 0, _drawTitle, true);
  _shown = false;
}

void ezHeader::_recalculate() {
  bool we_have_leftover = false;
  uint16_t x = 0;
  for (uint8_t n = 0; n < _widgets.size(); n++) {  // start from left, set x values
    _widgets[n].x = x;
    if (_widgets[n].leftover) {  // until "leftover" widget (usually "title")
      we_have_leftover = true;
      break;
    }
    x += _widgets[n].w;
  }
  if (we_have_leftover) {  // Then start from right setting x values
    x = TFT_W;
    for (int8_t n = _widgets.size() - 1; n >= 0; n--) {
      if (_widgets[n].leftover) {  // and set width of leftover widget to remainder
        _widgets[n].w = x - _widgets[n].x;
        break;
      }
      x -= _widgets[n].w;
      _widgets[n].x = x;
    }
  }
  if (_shown)
    show();
}

void ezHeader::insert(uint8_t position,
                      std::string name,
                      uint16_t width,
                      void (*function)(uint16_t x, uint16_t w),
                      bool leftover /* = false */) {
  for (uint8_t n = 0; n < _widgets.size(); n++) {
    if (_widgets[n].leftover)
      leftover = false;  // ignore leftover if there already is one
    if (_widgets[n].name == name)
      return;  // fail silently if trying to create two widgets with same name
  }
  if (position > _widgets.size())
    position = _widgets.size();  // interpret anything over current number of widgets as RIGHTMOST
  header_widget_t wdgt;
  wdgt.name = name;
  wdgt.leftover = leftover;
  wdgt.x = 0;
  wdgt.w = width;
  wdgt.function = function;
  _widgets.insert(_widgets.begin() + position, wdgt);
  _recalculate();
}

void ezHeader::remove(std::string name) {
  for (uint8_t n = 0; n < _widgets.size(); n++) {
    if (_widgets[n].name == name) {
      _widgets.erase(_widgets.begin() + n);
      _recalculate();
    }
  }
}

uint8_t ezHeader::position(std::string name) {
  for (uint8_t n = 0; n < _widgets.size(); n++) {
    if (_widgets[n].name == name)
      return n;
  }
  return 0;
}

void ezHeader::show(std::string t /* = "" */) {
  _shown = true;
  if (t != "")
    _title = t;  // only change title if provided
  M5.Lcd.fillRect(0, 0, TFT_W, ez.theme->header_height,
                  ez.theme->header_bgcolor);  // Clear header area
  for (uint8_t n = 0; n < _widgets.size(); n++) {
    (_widgets[n].function)(_widgets[n].x, _widgets[n].w);  // Tell all header widgets to draw
  }
  ez.canvas.top(ez.theme->header_height);
}

void ezHeader::draw(std::string name) {
  if (!_shown)
    return;
  for (uint8_t n = 0; n < _widgets.size(); n++) {
    if (_widgets[n].name == name) {
      (_widgets[n].function)(_widgets[n].x, _widgets[n].w);
    }
  }
}

void ezHeader::clear(bool wipe /* = true */) {
  if (wipe)
    M5.Lcd.fillRect(0, 0, TFT_W, ez.theme->header_height, ez.theme->background);
  _shown = false;
  ez.canvas.top(0);
}

bool ezHeader::shown() {
  return _shown;
}

std::string ezHeader::title() {
  return _title;
}

void ezHeader::title(std::string t) {
  _title = t;
  if (_shown)
    draw("title");
}

void ezHeader::_drawTitle(uint16_t x, uint16_t w) {
  M5.Lcd.fillRect(x, 0, w, ez.theme->header_height, ez.theme->header_bgcolor);
  M5.Lcd.setTextDatum(TL_DATUM);
  M5.Lcd.setTextColor(ez.theme->header_fgcolor);
  ez.setFont(ez.theme->header_font);
  M5.Lcd.drawString(ez.clipString(_title, w - ez.theme->header_hmargin).c_str(),
                    x + ez.theme->header_hmargin, ez.theme->header_tmargin);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   C A N V A S
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t ezCanvas::_y, ezCanvas::_top, ezCanvas::_bottom;
uint16_t ezCanvas::_x, ezCanvas::_left, ezCanvas::_right, ezCanvas::_lmargin;
const GFXfont *ezCanvas::_font;
uint16_t ezCanvas::_color;
bool ezCanvas::_wrap, ezCanvas::_scroll;
std::vector<print_t> ezCanvas::_printed;
uint32_t ezCanvas::_next_scroll;

void ezCanvas::begin() {
  _left = 0;
  _right = TFT_W - 1;
  _top = 0;
  _bottom = TFT_H - 1;
  ez.addEvent(ez.canvas.loop);
  reset();
}

void ezCanvas::reset() {
  _wrap = true;
  _scroll = false;
  _font = ez.theme->print_font;
  _color = ez.theme->print_color;
  _lmargin = 0;
  _next_scroll = 0;
  clear();
}

void ezCanvas::clear() {
  M5.Lcd.fillRect(left(), top(), width(), height(), ez.screen.background());
  _x = _lmargin;
  _y = 0;
  _printed.clear();
}

uint8_t ezCanvas::top() {
  return _top;
}

uint8_t ezCanvas::bottom() {
  return _bottom;
}

uint16_t ezCanvas::left() {
  return _left;
}

uint16_t ezCanvas::right() {
  return _right;
}

uint8_t ezCanvas::height() {
  return _bottom - _top + 1;
}

uint16_t ezCanvas::width() {
  return _right - _left + 1;
}

bool ezCanvas::scroll() {
  return _scroll;
}

void ezCanvas::scroll(bool s) {
  _scroll = s;
}

bool ezCanvas::wrap() {
  return _wrap;
}

void ezCanvas::wrap(bool w) {
  _wrap = w;
}

uint16_t ezCanvas::lmargin() {
  return _lmargin;
}

void ezCanvas::lmargin(uint16_t newmargin) {
  if (_x < newmargin)
    _x = newmargin;
  _lmargin = newmargin;
}

void ezCanvas::font(const GFXfont *font) {
  _font = font;
}

const GFXfont *ezCanvas::font() {
  return _font;
}

void ezCanvas::color(uint16_t color) {
  _color = color;
}

uint16_t ezCanvas::color() {
  return _color;
}

void ezCanvas::pos(uint16_t x, uint8_t y) {
  _x = x;
  _y = y;
}

uint16_t ezCanvas::x() {
  return _x;
}

void ezCanvas::x(uint16_t x) {
  _x = x;
}

uint8_t ezCanvas::y() {
  return _y;
}

void ezCanvas::y(uint8_t y) {
  _y = y;
}

void ezCanvas::top(uint8_t newtop) {
  if (_y < newtop)
    _y = newtop;
  _top = newtop;
}

void ezCanvas::bottom(uint8_t newbottom) {
  _bottom = newbottom;
}

size_t ezCanvas::write(uint8_t c) {
  _print(std::string(c, 1));
  return 1;
}

size_t ezCanvas::write(const char *str) {
  std::string tmp = std::string(str);
  _print(tmp);
  return tmp.size();
}

size_t ezCanvas::write(const uint8_t *buffer, size_t size) {
  std::string tmp = std::string((const char *)buffer, size);
  _print(tmp);
  return size;
}

uint16_t ezCanvas::loop(void *private_data) {
  if (_next_scroll && millis() >= _next_scroll) {
    ez.setFont(_font);
    uint8_t h = ez.fontHeight();
    uint8_t scroll_by = _y - _bottom;
    if (_x > _lmargin)
      scroll_by += h;
    const GFXfont *hold_font = _font;
    const uint16_t hold_color = _color;
    for (uint16_t n = 0; n < _printed.size(); n++) {
      _printed[n].y -= scroll_by;
    }
    M5.Lcd.fillRect(left(), top(), width(), height(), ez.screen.background());
    // M5.Lcd.fillRect(0, 0, 320, 240, ez.screen.background());
    for (uint16_t n = 0; n < _printed.size(); n++) {
      if (_printed[n].y >= _top) {
        if (_printed[n].font != _font)
          ez.setFont(_printed[n].font);
        if (_printed[n].color != _color)
          M5.Lcd.setTextColor(_printed[n].color);
        M5.Lcd.drawString(_printed[n].text.c_str(), _printed[n].x, _printed[n].y);
      }
    }
    _y -= scroll_by;
    _font = hold_font;
    _color = hold_color;
    _next_scroll = 0;

    std::vector<print_t> clean_copy;
    for (uint16_t n = 0; n < _printed.size(); n++) {
      if (_printed[n].y >= 0)
        clean_copy.push_back(_printed[n]);
    }
    _printed = clean_copy;
    Serial.println(ESP.getFreeHeap());
  }
  return 10;
}

void ezCanvas::_print(std::string text) {
  ez.setFont(_font);
  M5.Lcd.setTextDatum(TL_DATUM);
  M5.Lcd.setTextColor(_color, ez.theme->background);
  uint8_t h = ez.fontHeight();
  if (_y + h > _bottom) {
    if (!_scroll)
      return;
    if (!_next_scroll)
      _next_scroll = millis() + 200;
  }
  auto crlf = text.find("\r\n");
  std::string remainder = "";
  if (crlf != std::string::npos) {
    remainder = text.substr(crlf + 2);
    text = text.substr(0, crlf);
  }
  if (_x + M5.Lcd.textWidth(text.c_str()) <= _right) {
    if (text != "")
      _putString(text);
  } else {
    for (uint16_t n = 0; n < text.length(); n++) {
      if (_x + M5.Lcd.textWidth(text.substr(0, n + 1).c_str()) > _right) {
        if (n) {
          _putString(text.substr(0, n));
        }
        if (_wrap) {
          _x = _lmargin;
          _y += h;
          _print(text.substr(n));
        }
        break;
      }
    }
  }
  if (crlf != -1) {
    _x = _lmargin;
    _y += h;
    if (remainder != "")
      _print(remainder);
  }
}

void ezCanvas::_putString(std::string text) {
  ez.setFont(_font);
  uint8_t h = ez.fontHeight();
  if (_scroll) {
    print_t p;
    p.font = _font;
    p.color = _color;
    p.x = _x;
    p.y = _y;
    p.text = text;
    _printed.push_back(p);
  }
  if (_y + h > _bottom) {
    _x += M5.Lcd.textWidth(text.c_str());
  } else {
    _x += M5.Lcd.drawString(text.c_str(), _x, _y);
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   B U T T O N S
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::string ezButtons::_btn_a;
std::string ezButtons::_btn_b;
std::string ezButtons::_btn_c;
bool ezButtons::_key_release_wait;
bool ezButtons::_lower_button_row, ezButtons::_upper_button_row;

void ezButtons::begin() {
  clear(false);
}

void ezButtons::show(std::vector<std::string> buttons) {
  switch (buttons.size()) {
    case 1:
      _drawButtons("", buttons[0], "");
      break;
    case 2:
      _drawButtons(buttons[0], buttons[1], "");
      break;
    case 3:
      _drawButtons(buttons[0], buttons[1], buttons[2]);
      break;
  }
}

std::vector<std::string> ezButtons::get() {
  return {_btn_a, _btn_b, _btn_c};
}

void ezButtons::clear(bool wipe /* = true */) {
  if (wipe && (_lower_button_row || _upper_button_row)) {
    M5.Lcd.fillRect(0, ez.canvas.bottom() + 1, TFT_H - ez.canvas.bottom() - 1, TFT_W,
                    ez.screen.background());
  }
  _btn_a = _btn_b = _btn_c = "";
  _lower_button_row = false;
  _upper_button_row = false;
  ez.canvas.bottom(TFT_H - 1);
}

void ezButtons::_drawButtons(std::string btn_a, std::string btn_b, std::string btn_c) {
  int16_t btnwidth = int16_t((TFT_W - 4 * ez.theme->button_gap) / 3);

  // See if any buttons are used on the bottom row
  if (btn_a != "" || btn_b != "" || btn_c != "") {
    if (!_lower_button_row) {
      // If the lower button row wasn't there before, clear the area first
      M5.Lcd.fillRect(0, TFT_H - ez.theme->button_height - ez.theme->button_gap, TFT_W,
                      ez.theme->button_height + ez.theme->button_gap, ez.screen.background());
    }
    // Then draw the three buttons there. (drawButton erases single buttons if unused.)
    if (_btn_a != btn_a) {
      _drawButton(1, btn_a, ez.theme->button_gap, btnwidth);
      _btn_a = btn_a;
    }
    if (_btn_b != btn_b) {
      _drawButton(1, btn_b, btnwidth + 2 * ez.theme->button_gap, btnwidth);
      _btn_b = btn_b;
    }
    if (_btn_c != btn_c) {
      _drawButton(1, btn_c, 2 * btnwidth + 3 * ez.theme->button_gap, btnwidth);
      _btn_c = btn_c;
    }
    _lower_button_row = true;
  } else {
    if (_lower_button_row) {
      // If there was a lower button row before and it's now gone, clear the area
      M5.Lcd.fillRect(0, TFT_H - ez.theme->button_height - ez.theme->button_gap, TFT_W,
                      ez.theme->button_height + ez.theme->button_gap, ez.screen.background());
      _btn_a = _btn_b = _btn_c = "";
      _lower_button_row = false;
    }
  }

  uint8_t button_rows = _upper_button_row ? 2 : (_lower_button_row ? 1 : 0);
  ez.canvas.bottom(TFT_H - (button_rows * (ez.theme->button_height + ez.theme->button_gap)));
}

void ezButtons::_drawButton(int16_t row, std::string text, int16_t x, int16_t w) {
  // row = 1 for lower and 2 for upper row
  int16_t y, bg_color;
  if (row == 1) {
    y = TFT_H - ez.theme->button_height;
    bg_color = ez.theme->button_bgcolor_b;
  } else {
    y = TFT_H - 2 * ez.theme->button_height - ez.theme->button_gap;
    bg_color = ez.theme->button_bgcolor_t;
  }
  if (text != "") {
    ez.setFont(ez.theme->button_font);
    M5.Lcd.fillRoundRect(x, y, w, ez.theme->button_height, ez.theme->button_radius, bg_color);
    _drawButtonString(text, x + int16_t(w / 2), y + ez.theme->button_tmargin,
                      ez.theme->button_fgcolor, TC_DATUM);
  } else {
    M5.Lcd.fillRect(x, y, w, ez.theme->button_height, ez.screen.background());
  }
}

void ezButtons::_drawButtonString(std::string text,
                                  int16_t x,
                                  int16_t y,
                                  uint16_t color,
                                  int16_t datum) {
  if (text == "~")
    return;
  if (text == "up" || text == "down" || text == "left" || text == "right") {
    y += 2;
    int16_t w = M5.Lcd.textWidth("A") * 1.2;
    int16_t h = ez.fontHeight() * 0.6;
    if (datum == TR_DATUM)
      x = x - w;
    if (datum == TC_DATUM)
      x = x - w / 2;
    if (text == "up")
      M5.Lcd.fillTriangle(x, y + h, x + w, y + h, x + w / 2, y, color);
    if (text == "down")
      M5.Lcd.fillTriangle(x, y, x + w, y, x + w / 2, y + h, color);
    if (text == "right")
      M5.Lcd.fillTriangle(x, y, x, y + h, x + w, y + h / 2, color);
    if (text == "left")
      M5.Lcd.fillTriangle(x + w, y, x + w, y + h, x, y + h / 2, color);
  } else {
    M5.Lcd.setTextColor(color);
    M5.Lcd.setTextDatum(datum);
    M5.Lcd.drawString(text.c_str(), x, y + 1);
  }
}

void ezButtons::releaseWait() {
  _key_release_wait = true;
}

std::string ezButtons::poll() {
  std::string keystr = "";

  ez.yield();

  if (!_key_release_wait) {
    if (_btn_a != "" && M5.BtnA.wasReleased()) {
      keystr = _btn_a;
    }

    if (_btn_b != "" && M5.BtnB.wasReleased()) {
      keystr = _btn_b;
    }
  }

  if (M5.BtnA.isReleased() && M5.BtnB.isReleased()) {
    _key_release_wait = false;
  }

  if (keystr == "~")
    keystr = "";
#ifdef M5EZ_BACKLIGHT
  if (keystr != "")
    ez.backlight.activity();
#endif
  return keystr;
}

const std::string ezButtons::wait() {
  std::string keystr = "";
  while (keystr == "") {
    keystr = ez.buttons.poll();
  }
  return keystr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   S E T T I N G S
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ezMenu ezSettings::menuObj("Settings menu");

void ezSettings::begin() {
  menuObj.txtSmall();
  menuObj.buttons({"up", "select", "down"});
#ifdef M5EZ_BATTERY
  ez.battery.begin();
#endif
#ifdef M5EZ_CLOCK
  ez.clock.begin();
#endif
#ifdef M5EZ_BACKLIGHT
  ez.backlight.begin();
#endif
  if (ez.themes.size() > 1) {
    ez.settings.menuObj.addItem("Theme chooser", "", ez.theme->menu);
  }
  ez.settings.menuObj.addItem("Factory defaults", "", ez.settings.defaults);
}

void ezSettings::menu() {
  menuObj.run();
}

void ezSettings::defaults() {
  if (ez.msgBox("Reset to defaults?",
                {"Are you sure you want to reset all settings to factory defaults?"},
                {"yes", "", "no"})
      == "yes") {
    Preferences prefs;
    prefs.begin("M5ez");
    prefs.clear();
    prefs.end();
    ESP.restart();
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   B A C K L I G H T
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef M5EZ_BACKLIGHT
uint8_t ezBacklight::_brightness;
uint8_t ezBacklight::_inactivity;
uint32_t ezBacklight::_last_activity;
uint8_t ezBacklight::_MinimumBrightness;
const uint8_t ezBacklight::_MaxSteps;
bool ezBacklight::_backlight_off = false;

void ezBacklight::begin() {
  ez.addEvent(ez.backlight.loop);
  ez.settings.menuObj.addItem("Backlight settings", "", ez.backlight.menu);
  Preferences prefs;
  prefs.begin("M5ez", true);  // read-only
  _brightness = prefs.getUChar("brightness", 128);
  switch (M5.getBoard()) {
    case m5::board_t::board_M5StickCPlus2:
    case m5::board_t::board_M5StackCore2:
      _MinimumBrightness = 16;
      break;
    case m5::board_t::board_M5StickCPlus:
    case m5::board_t::board_M5StickC:
      _MinimumBrightness = 48;
      break;
    default:
      _MinimumBrightness = 64;
  }
  if (_brightness < _MinimumBrightness) {
    _brightness = 100;
  }
  _inactivity = prefs.getUChar("inactivity", 1);
  prefs.end();
  M5.Display.setBrightness(_brightness);
}

void ezBacklight::menu() {
  uint8_t start_brightness = _brightness;
  uint8_t start_inactivity = _inactivity;
  ezMenu blmenu("Backlight settings");
  blmenu.txtSmall();
  blmenu.buttons({"OK", "down"});
  blmenu.addItem("Backlight brightness");
  blmenu.addItem("Inactivity timeout");
  blmenu.addItem("Back");
  blmenu.downOnLast("first");
  uint8_t step = (_brightness - _MinimumBrightness) * _MaxSteps
                 / (256 - _MinimumBrightness);  // Calculate step from brightness
  while (true) {
    switch (blmenu.runOnce()) {
      case 1: {
        ezProgressBar bl("Backlight brightness", {"Set brightness"}, {"Adjust", "Back"});
        while (true) {
          std::string b = ez.buttons.poll();
          if (b == "Adjust") {
            if (_brightness >= _MinimumBrightness && step < _MaxSteps - 1) {
              step++;
              _brightness =
                  _MinimumBrightness + (step * (255 - _MinimumBrightness) / (_MaxSteps - 1));
            } else {
              step = 0;
              _brightness = _MinimumBrightness;
            }
          }
          float p = ((float)step / (_MaxSteps - 1)) * 100.0f;
          bl.value(p);
          M5.Display.setBrightness(_brightness);
          if (b == "Back")
            break;
        }
      } break;
      case 2: {
        const std::vector<std::string> text = {
            "Backlight will not turn off",
            "Backlight will turn off after 30 seconds of inactivity",
            "Backlight will turn off after a minute of inactivity",
        };
        while (true) {
          ez.msgBox("Inactivity timeout", {text[_inactivity]}, {"Adjust", "Back"}, false);
          std::string b = ez.buttons.wait();
          if (b == "Adjust")
            _inactivity++;
          if (_inactivity > 2)
            _inactivity = 0;
          if (b == "Back")
            break;
        }
      } break;
      case 0:
        if (_brightness != start_brightness || _inactivity != start_inactivity) {
          Preferences prefs;
          prefs.begin("M5ez");
          prefs.putUChar("brightness", _brightness);
          prefs.putUChar("inactivity", _inactivity);
          prefs.end();
        }
        return;
        //
    }
  }
}

void ezBacklight::inactivity(uint8_t half_minutes) {
  if (half_minutes == USER_SET) {
    Preferences prefs;
    prefs.begin("M5ez", true);
    _inactivity = prefs.getUShort("inactivity", 0);
    prefs.end();
  } else {
    _inactivity = half_minutes;
  }
}

void ezBacklight::activity() {
  _last_activity = millis();
}

void changeCpuPower(bool reduce) {
  switch (M5.getBoard()) {
    case m5::board_t::board_M5StickC:
    case m5::board_t::board_M5StickCPlus:
      // do nothing, lightSleep() works with backlight control
      break;
    default:
      // Backlight is PWM controlled, lightSleep() stops PWM
      if (reduce) {
        setCpuFrequencyMhz(10);
      } else {
        setCpuFrequencyMhz(80);
      }
  }
}

uint16_t ezBacklight::loop(void *private_data) {
  if (!_backlight_off && _inactivity) {
    if (millis() > _last_activity + 30000 * _inactivity) {
      _backlight_off = true;
      M5.Display.setBrightness(_MinimumBrightness);
      changeCpuPower(true);
      while (true) {
        if (M5.BtnA.wasClicked() || M5.BtnB.wasClicked())
          break;
        ez.yield();
        switch (M5.getBoard()) {
          case m5::board_t::board_M5StickC:
          case m5::board_t::board_M5StickCPlus:
            M5.Power.lightSleep(100000);
            break;
          default:
            // PWM inactive during lightSleep, thus display backlight not controlled
            delay(100);
        }
      }
      ez.buttons.releaseWait();  // Make sure the key pressed to wake display gets ignored
      changeCpuPower(false);
      M5.Display.setBrightness(_brightness);
      activity();
      _backlight_off = false;
    }
  }
  return 1000;
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   B A T T E R Y
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef M5EZ_BATTERY
bool ezBattery::_on = false;

void ezBattery::begin() {
  _on = true;
  if (_on) {
    _refresh();
  }
}

uint16_t ezBattery::loop(void *private_data) {
  if (!_on)
    return 0;
  ez.header.draw("battery");
  return 5000;
}

// Transform the M5Stack built in battery level into an internal format.
//  From [100, 75, 50, 25, 0] to [4, 3, 2, 1, 0]
uint8_t ezBattery::getTransformedBatteryLevel() {
  int32_t level = M5.Power.getBatteryLevel();

  return map(level, 0, 85, 0, 4);
}

// Return the theme based battery bar color according to its level
uint16_t ezBattery::getBatteryBarColor(uint8_t batteryLevel) {
  switch (batteryLevel) {
    case 0:
      return ez.theme->battery_0_fgcolor;
    case 1:
      return ez.theme->battery_25_fgcolor;
    case 2:
      return ez.theme->battery_50_fgcolor;
    case 3:
      return ez.theme->battery_75_fgcolor;
    case 4:
      return ez.theme->battery_100_fgcolor;
    default:
      return ez.theme->header_fgcolor;
  }
}

void ezBattery::_refresh() {
  if (_on) {
    ez.header.insert(RIGHTMOST, "battery",
                     ez.theme->battery_bar_width + 2 * ez.theme->header_hmargin,
                     ez.battery._drawWidget);
    ez.addEvent(ez.battery.loop);
  } else {
    ez.header.remove("battery");
    ez.removeEvent(ez.battery.loop);
  }
}

void ezBattery::_drawWidget(uint16_t x, uint16_t w) {
  uint8_t currentBatteryLevel = getTransformedBatteryLevel();
  uint16_t left_offset = x + ez.theme->header_hmargin;
  uint8_t top = (ez.theme->header_height / 10) + 1;
  uint8_t height = ez.theme->header_height * 0.8;
  M5.Lcd.fillRoundRect(left_offset, top, ez.theme->battery_bar_width, height,
                       ez.theme->battery_bar_gap, ez.theme->header_bgcolor);
  M5.Lcd.drawRoundRect(left_offset, top, ez.theme->battery_bar_width, height,
                       ez.theme->battery_bar_gap, ez.theme->header_fgcolor);
  uint8_t bar_width = (ez.theme->battery_bar_width - ez.theme->battery_bar_gap * 5) / 4.0;
  uint8_t bar_height = height - ez.theme->battery_bar_gap * 2;
  left_offset += ez.theme->battery_bar_gap;
  for (uint8_t n = 0; n < currentBatteryLevel; n++) {
    M5.Lcd.fillRect(left_offset + n * (bar_width + ez.theme->battery_bar_gap),
                    top + ez.theme->battery_bar_gap, bar_width, bar_height,
                    getBatteryBarColor(currentBatteryLevel));
  }
}

#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   E Z
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::vector<ezTheme> M5ez::themes;
ezTheme *M5ez::theme = NULL;
ezScreen M5ez::screen;
constexpr ezScreen &M5ez::s;
ezHeader M5ez::header;
constexpr ezHeader &M5ez::h;
ezCanvas M5ez::canvas;
constexpr ezCanvas &M5ez::c;
ezButtons M5ez::buttons;
constexpr ezButtons &M5ez::b;
ezSettings M5ez::settings;
#ifdef M5EZ_BATTERY
ezBattery M5ez::battery;
#endif
#ifdef M5EZ_BACKLIGHT
ezBacklight M5ez::backlight;
#endif
std::vector<event_t> M5ez::_events;

void M5ez::begin() {
  auto cfg = M5.config();
  cfg.internal_imu = false;
  cfg.internal_spk = false;
  cfg.internal_mic = false;
  M5.begin(cfg);
#if ARDUINO_M5STICK_C || ARDUINO_M5STICK_C_PLUS
  M5.Lcd.setRotation(3);
#endif
  ezTheme::begin();
  ez.screen.begin();
  ez.settings.begin();
}

void M5ez::yield() {
  ::yield();  // execute the Arduino yield in the root namespace
  M5.update();
  for (uint8_t n = 0; n < _events.size(); n++) {
    if (millis() > _events[n].when) {
      uint16_t r = (_events[n].function)(_events[n].private_data);
      if (r) {
        _events[n].when = millis() + r - 1;
      } else {
        _events.erase(_events.begin() + n);
        break;  // make sure we don't go beyond _events.size() after deletion
      }
    }
  }
#ifdef M5EZ_CLOCK
  events();  // TMP
#endif
}

void M5ez::addEvent(uint16_t (*function)(void *private_data),
                    void *private_data,
                    uint32_t when /* = 1 */) {
  event_t n;
  n.function = function;
  n.private_data = private_data;
  n.when = millis() + when - 1;
  _events.push_back(n);
}

void M5ez::removeEvent(uint16_t (*function)(void *private_data)) {
  uint8_t n = 0;
  while (n < _events.size()) {
    if (_events[n].function == function) {
      _events.erase(_events.begin() + n);
    }
    n++;
  }
}

bool M5ez::_redraw;

void M5ez::redraw() {
  _redraw = true;
}

// ez.msgBox

std::string M5ez::msgBox(std::string header,
                         std::vector<std::string> msg,
                         std::vector<std::string> buttons /* = "OK" */,
                         const bool blocking /* = true */,
                         const GFXfont *font /* = NULL */,
                         uint16_t color /* = NO_COLOR */,
                         const bool clear /* = true */) {
  if (ez.header.title() != header) {
    ez.screen.clear();
    if (header != "")
      ez.header.show(header);
  } else {
    if (clear) {
      ez.canvas.clear();
    }
  }
  if (!font)
    font = ez.theme->msg_font;
  if (color == NO_COLOR)
    color = ez.theme->msg_color;
  ez.buttons.show(buttons);
  std::vector<line_t> lines;
  M5.Lcd.setTextDatum(CC_DATUM);
  M5.Lcd.setTextColor(color);
  ez.setFont(font);
  _fitLines(msg, ez.canvas.width() - 2 * ez.theme->msg_hmargin, ez.canvas.width() / 3, lines);
  int16_t font_h = ez.fontHeight();
  for (int8_t n = 0; n < lines.size(); n++) {
    int16_t y =
        ez.canvas.top() + ez.canvas.height() / 2 - ((lines.size() - 1) * font_h / 2) + n * font_h;
    if (!clear) {
      M5.Lcd.fillRect(0, y - font_h / 2, TFT_W, font_h, ez.theme->background);
    }
    M5.Lcd.drawString(lines[n].line.c_str(), TFT_W / 2, y);
  }
  if (buttons.size() != 0 && blocking) {
    std::string ret = ez.buttons.wait();
    ez.screen.clear();
    return ret;
  } else {
    return "";
  }
}

std::size_t M5ez::_findBreak(std::string text, const std::size_t pos, uint16_t max_width) {
  uint16_t start = pos;
  std::size_t breakpoint = std::string::npos;
  while (true) {
    std::size_t testpoint = text.find(' ', start);
    if (testpoint == std::string::npos) {
      breakpoint = testpoint;
      break;
    }
    uint16_t width = M5.Lcd.textWidth(text.substr(pos, testpoint - pos).c_str());
    if (width > max_width) {
      break;
    }

    breakpoint = testpoint;
    start = testpoint + 1;
  }

  return breakpoint;
}

void M5ez::_fitLines(std::vector<std::string> text,
                     uint16_t max_width,
                     uint16_t min_width,
                     std::vector<line_t> &lines) {
  for (int16_t n = 0; n < text.size(); n++) {
    // break each line on spaces into text width
    uint16_t pos = 0;
    std::size_t breakpoint;
    do {
      breakpoint = _findBreak(text[n], pos, max_width - 10);
      line_t line = {n, text[n].substr(pos, breakpoint - pos)};
      lines.push_back(line);
      pos = breakpoint + 1;
    } while (breakpoint != std::string::npos);
  }
}

std::string M5ez::clipString(std::string input, int16_t cutoff, bool dots /* = true */) {
  if (M5.Lcd.textWidth(input.c_str()) <= cutoff) {
    return input;
  } else {
    for (int16_t n = input.length(); n >= 0; n--) {
      std::string toMeasure = input.substr(0, n);
      if (dots)
        toMeasure = toMeasure + "..";
      if (M5.Lcd.textWidth(toMeasure.c_str()) <= cutoff)
        return toMeasure;
    }
    return "";
  }
}

bool M5ez::isBackExitOrDone(std::string str) {
  if (str == "back" || str == "exit" || str == "done" || str == "Back" || str == "Exit"
      || str == "Done")
    return true;
  return false;
}

// Font related M5.Lcd wrappers

void M5ez::setFont(const GFXfont *font) {
  long ptrAsInt = (long)font;
  int16_t size = 1;
  if (ptrAsInt <= 16) {
    if (ptrAsInt > 8) {
      ptrAsInt -= 8;
      size++;
    }
    M5.Lcd.setTextFont(ptrAsInt);
  } else {
    M5.Lcd.setFont(font);
  }
  M5.Lcd.setTextSize(size);
}

int16_t M5ez::fontHeight() {
#if ARDUINO_M5STICK_C_PLUS || ARDUINO_M5STACK_CORE_ESP32
  return M5.Lcd.fontHeight(M5.Lcd.getFont());
#else
  return 11;
#endif
}

std::string M5ez::version() {
  return std::string(M5EZ_VERSION);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   M E N U
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ezMenu::ezMenu(std::string hdr /* = "" */) {
  _img_background = NO_COLOR;
  _offset = 0;
  _selected = -1;
  _header = hdr;
  _buttons = {};
  _font = NULL;
  _img_from_top = 0;
  _img_caption_location = TC_DATUM;
  _img_caption_font = &FreeSansBold12pt7b;
  _img_caption_color = TFT_RED;
  _img_caption_hmargin = 10;
  _img_caption_vmargin = 10;
  _sortFunction = NULL;
}

void ezMenu::txtBig() {
  _font = ez.theme->menu_big_font;
}

void ezMenu::txtSmall() {
  _font = ez.theme->menu_small_font;
}

void ezMenu::txtFont(const GFXfont *font) {
  _font = font;
}

bool ezMenu::addItem(std::string name,
                     std::string caption,
                     void (*simpleFunction)() /* = NULL */,
                     bool (*advancedFunction)(ezMenu *callingMenu) /* = NULL */,
                     void (*drawFunction)(ezMenu *callingMenu,
                                          int16_t x,
                                          int16_t y,
                                          int16_t w,
                                          int16_t h) /* = NULL */) {
  MenuItem_t new_item = {name, caption, simpleFunction, advancedFunction, drawFunction};
  if (_selected == -1)
    _selected = _items.size();
  _items.push_back(new_item);
  _sortItems();
  M5ez::_redraw = true;
  return true;
}

bool ezMenu::deleteItem(int16_t index) {
  if (index < 1 || index > _items.size())
    return false;
  index--;  // internally we work with zero-referenced items
  _items.erase(_items.begin() + index);
  if (_selected >= _items.size())
    _selected = _items.size() - 1;
  _fixOffset();
  M5ez::_redraw = true;
  return true;
}

bool ezMenu::deleteItem(std::string name) {
  return deleteItem(getItemNum(name));
}

bool ezMenu::setCaption(std::string name, std::string caption) {
  for (int n = 0; n < _items.size(); n++) {
    if (_items[n].name == name) {
      _items[n].caption = caption;
      M5ez::_redraw = true;
      return true;
    }
  }

  return false;
}

int16_t ezMenu::countItems(void) {
  return _items.size();
}

int16_t ezMenu::getItemNum(std::string name) {
  for (int16_t n = 0; n < _items.size(); n++) {
    if (_items[n].name == name)
      return n + 1;
  }
  return 0;
}

void ezMenu::setSortFunction(bool (*sortFunction)(const char *s1, const char *s2)) {
  _sortFunction = sortFunction;
  _sortItems();  // In case the menu is already populated
}

void ezMenu::buttons(std::vector<std::string> bttns) {
  _buttons = bttns;
}

void ezMenu::upOnFirst(std::string name) {
  _up_on_first = name;
}

void ezMenu::leftOnFirst(std::string name) {
  _up_on_first = name;
}

void ezMenu::downOnLast(std::string name) {
  _down_on_last = name;
}

void ezMenu::rightOnLast(std::string name) {
  _down_on_last = name;
}

void ezMenu::run() {
  while (runOnce()) {
  }
}

int16_t ezMenu::runOnce(bool dynamic) {
  if (_items.size() == 0)
    return 0;
  if (_selected == -1)
    _selected = 0;
  if (!_font)
    _font = ez.theme->menu_big_font;  // Cannot be in constructor: ez.theme not there yet

  int16_t r;

  do {
    r = _runTextOnce(dynamic);
  } while (r < 0);

  return r;
}

int16_t ezMenu::_runTextOnce(bool dynamic) {
  if (_buttons.size() == 0)
    _buttons = {"up", "select", "down"};
  ez.screen.clear();
  if (_header != "")
    ez.header.show(_header);
  ez.setFont(_font);
  _per_item_h = ez.fontHeight();
  ez.buttons.show(
      _buttons);  // we need to draw the buttons here to make sure ez.canvas.height() is correct
  _items_per_screen = (ez.canvas.height() - 5) / _per_item_h;
  _drawItems();
  while (true) {
    int16_t old_selected = _selected;
    int16_t old_offset = _offset;
    std::vector<std::string> tmp_buttons = _buttons;
    if (_selected <= 0)
      _replace(tmp_buttons, "up", _up_on_first);
    if (_selected >= _items.size() - 1)
      _replace(tmp_buttons, "down", _down_on_last);
    ez.buttons.show(tmp_buttons);
    std::string name = _items[_selected].name;
    std::string pressed;
    while (true) {
      pressed = ez.buttons.poll();
      if (M5ez::_redraw) {
        _drawItems();
        if (dynamic) {
          return -1;
        }
      }
      if (pressed != "")
        break;
    }
    if (pressed == "up") {
      _selected--;
      _fixOffset();
    } else if (pressed == "down") {
      _selected++;
      _fixOffset();
    } else if (pressed == "first") {
      _selected = 0;
      _offset = 0;
    } else if (pressed == "last") {
      _selected = _items.size() - 1;
      _offset = 0;
      _fixOffset();
    } else if ((ez.isBackExitOrDone(name) && !_items[_selected].advancedFunction)
               || ez.isBackExitOrDone(pressed)) {
      _pick_button = pressed;
      _selected = -1;
      ez.screen.clear();
      return 0;
    } else {
      // Some other key must have been pressed. We're done here!
      ez.screen.clear();
      _pick_button = pressed;
      if (_items[_selected].simpleFunction) {
        (_items[_selected].simpleFunction)();
      }
      if (_items[_selected].advancedFunction) {
        if (!(_items[_selected].advancedFunction)(this))
          return 0;
      }
      return _selected
             + 1;  // We return items starting at one, but work starting at zero internally
    }

    // Flicker prevention, only redraw whole menu if scrolled
    if (_offset == old_offset) {
      int16_t top_item_h =
          ez.canvas.top()
          + (ez.canvas.height() % _per_item_h)
                / 2;  // remainder of screen left over by last item not fitting split to center menu
      if (_items[old_selected].drawFunction) {
        ez.setFont(_font);
        (_items[old_selected].drawFunction)(
            this, ez.theme->menu_lmargin, top_item_h + (old_selected - _offset) * _per_item_h,
            TFT_W - ez.theme->menu_lmargin - ez.theme->menu_rmargin, _per_item_h);
      } else {
        MenuItem_t *item = &_items[old_selected];
        std::string text = item->caption != "" ? item->caption : item->name;
        _drawItem(old_selected - _offset, text, false);
      };
      if (_items[_selected].drawFunction) {
        ez.setFont(_font);
        (_items[_selected].drawFunction)(
            this, ez.theme->menu_lmargin, top_item_h + (_selected - _offset) * _per_item_h,
            TFT_W - ez.theme->menu_lmargin - ez.theme->menu_rmargin, _per_item_h);
      } else {
        MenuItem_t *item = &_items[_selected];
        std::string text = item->caption != "" ? item->caption : item->name;
        _drawItem(_selected - _offset, text, true);
      };
    } else {
      ez.canvas.clear();
      _drawItems();
    }
  }
}

void ezMenu::_drawItems() {
  for (int16_t n = 0; n < _items_per_screen; n++) {
    int16_t item_ref = _offset + n;
    if (item_ref < _items.size()) {
      if (_items[item_ref].drawFunction) {
        int16_t top_item_h =
            ez.canvas.top()
            + (ez.canvas.height() % _per_item_h) / 2;  // remainder of screen left over by last item
                                                       // not fitting split to center menu
        ez.setFont(_font);
        (_items[item_ref].drawFunction)(this, ez.theme->menu_lmargin, top_item_h + n * _per_item_h,
                                        TFT_W - ez.theme->menu_lmargin - ez.theme->menu_rmargin,
                                        _per_item_h);
      } else {
        MenuItem_t *item = &_items[item_ref];
        std::string text = item->caption != "" ? item->caption : item->name;
        _drawItem(n, text, (item_ref == _selected));
      }
    }
  }
  _Arrows();
  M5ez::_redraw = false;
}

void ezMenu::_drawItem(int16_t n, std::string text, bool selected) {
  uint16_t fill_color;
  ez.setFont(_font);
  int16_t top_item_h =
      ez.canvas.top()
      + (ez.canvas.height() % _per_item_h)
            / 2;  // remainder of screen left over by last item not fitting split to center menu
  int16_t menu_text_y =
      top_item_h + (n * _per_item_h) + (_per_item_h / 2) + (_per_item_h % 2 ? 1 : 0);
  M5.Lcd.setTextDatum(CL_DATUM);
  if (selected) {
    fill_color = ez.theme->menu_sel_bgcolor;
    M5.Lcd.setTextColor(ez.theme->menu_sel_fgcolor);
  } else {
    fill_color = ez.screen.background();
    M5.Lcd.setTextColor(ez.theme->menu_item_color);
  }
  text = ez.clipString(text, TFT_W - ez.theme->menu_lmargin - 2 * ez.theme->menu_item_hmargin
                                 - ez.theme->menu_rmargin);
  M5.Lcd.fillRoundRect(ez.theme->menu_lmargin, top_item_h + n * _per_item_h,
                       TFT_W - ez.theme->menu_lmargin - ez.theme->menu_rmargin, _per_item_h,
                       ez.theme->menu_item_radius, fill_color);
  auto tabpos = text.find("\t");
  M5.Lcd.drawString(text.substr(0, tabpos).c_str(),
                    ez.theme->menu_lmargin + ez.theme->menu_item_hmargin, menu_text_y);
  if (tabpos != std::string::npos) {
    M5.Lcd.setTextDatum(CR_DATUM);
    M5.Lcd.drawString(text.substr(tabpos, std::string::npos).c_str(),
                      TFT_W - ez.theme->menu_rmargin - ez.theme->menu_item_hmargin, menu_text_y);
  }
}

void ezMenu::imgBackground(uint16_t color) {
  _img_background = color;
}

void ezMenu::imgFromTop(int16_t offset) {
  _img_from_top = offset;
}

void ezMenu::imgCaptionFont(const GFXfont *font) {
  _img_caption_font = font;
}

void ezMenu::imgCaptionLocation(uint8_t datum) {
  _img_caption_location = datum;
}

void ezMenu::imgCaptionColor(uint16_t color) {
  _img_caption_color = color;
}

void ezMenu::imgCaptionMargins(int16_t hmargin, int16_t vmargin) {
  _img_caption_hmargin = hmargin;
  _img_caption_vmargin = vmargin;
}

void ezMenu::imgCaptionMargins(int16_t margin) {
  _img_caption_hmargin = margin;
  _img_caption_vmargin = margin;
}

void ezMenu::_drawCaption() {
  int16_t x, y;
  std::string caption = _items[_selected].caption;
  if (_img_caption_font == NULL || caption == "")
    return;
  ez.setFont(_img_caption_font);
  M5.Lcd.setTextColor(_img_caption_color);
  M5.Lcd.setTextDatum(_img_caption_location);
  // Set X and Y for printing caption seperately, less code duplication
  switch (_img_caption_location) {
    case TL_DATUM:
    case ML_DATUM:
    case BL_DATUM:
      x = ez.canvas.left() + _img_caption_hmargin;
      break;
    case TC_DATUM:
    case MC_DATUM:
    case BC_DATUM:
      x = ez.canvas.left() + ez.canvas.width() / 2;
      break;
    case TR_DATUM:
    case MR_DATUM:
    case BR_DATUM:
    default:
      x = ez.canvas.right() - _img_caption_hmargin;
      break;
      return;
      //
  }
  switch (_img_caption_location) {
    case TL_DATUM:
    case TC_DATUM:
    case TR_DATUM:
      y = ez.canvas.top() + _img_caption_vmargin;
      break;
    case ML_DATUM:
    case MC_DATUM:
    case MR_DATUM:
      y = ez.canvas.top() + ez.canvas.height() / 2;
      break;
    case BL_DATUM:
    case BC_DATUM:
    case BR_DATUM:
    default:
      y = ez.canvas.bottom() - _img_caption_vmargin;
      break;
      //
  }
  M5.Lcd.drawString(caption.c_str(), x, y);
}

void ezMenu::_fixOffset() {
  if (_selected != -1) {
    if (_selected >= _offset + _items_per_screen)
      _offset = _selected - _items_per_screen + 1;
    if (_selected < _offset)
      _offset = _selected;

    // If there's stuff above the current screen and screen is not full (can only happen after
    // deleteItem)
    if (_offset > 0 && _offset + _items_per_screen > _items.size()) {
      _offset = _items.size() - _items_per_screen;
    }

  } else {
    _offset = 0;
  }
}

int16_t ezMenu::pick() {
  return _selected + 1;
}

std::string ezMenu::pickName() {
  return _items[_selected].name;
}

void ezMenu::_Arrows() {
  uint16_t fill_color;

  int16_t top = ez.canvas.top();
  int16_t height = ez.canvas.height();

  // Up arrow
  if (_offset > 0) {
    fill_color = ez.theme->menu_item_color;
  } else {
    fill_color = ez.screen.background();
  }
  static uint8_t l = ez.theme->menu_arrows_lmargin;
  M5.Lcd.fillTriangle(l, top + 17, l + 6, top + 17, l + 3, top + 5, fill_color);

  // Down arrow
  if (_items.size() > _offset + _items_per_screen) {
    fill_color = ez.theme->menu_item_color;
  } else {
    fill_color = ez.screen.background();
  }
  M5.Lcd.fillTriangle(l, top + height - 17, l + 6, top + height - 17, l + 3, top + height - 5,
                      fill_color);
}

bool ezMenu::_sortWrapper(MenuItem_t &item1, MenuItem_t &item2) {
  // Be sure to set _sortFunction before calling _sortWrapper(), as _sortItems ensures.
  return _sortFunction(item1.name.c_str(), item2.name.c_str());
}

void ezMenu::_sortItems() {
  if (_sortFunction && _items.size() > 1) {
    using namespace std::placeholders;  // For _1, _2
    sort(_items.begin(), _items.end(), std::bind(&ezMenu::_sortWrapper, this, _1, _2));
    M5ez::_redraw = true;
  }
}

void ezMenu::_replace(std::vector<std::string> &list, std::string name, std::string replacement) {
  for (int i = 0; i < list.size(); i++) {
    if (list[i] == name) {
      list[i] = replacement;
      return;
    }
  }
}

// For sorting by Names as quickly as possible
bool ezMenu::sort_asc_name_cs(const char *s1, const char *s2) {
  return 0 > strcmp(s1, s2);
}
bool ezMenu::sort_asc_name_ci(const char *s1, const char *s2) {
  return 0 > strcasecmp(s1, s2);
}
bool ezMenu::sort_dsc_name_cs(const char *s1, const char *s2) {
  return 0 < strcmp(s1, s2);
}
bool ezMenu::sort_dsc_name_ci(const char *s1, const char *s2) {
  return 0 < strcasecmp(s1, s2);
}

// For sorting by Caption if there is one, falling back to sorting by Name if no Caption is provided
// (all purpose)
const char *captionSortHelper(const char *nameAndCaption) {
  char *sub = strchr(nameAndCaption, '|');  // Find the separator
  if (nullptr == sub)
    return nameAndCaption;  // If none, return the entire string
  sub++;                    // move past the separator
  while (isspace(sub[0]))
    sub++;  // trim whitespace
  return sub;
}
bool ezMenu::sort_asc_caption_cs(const char *s1, const char *s2) {
  return 0 > strcmp(captionSortHelper(s1), captionSortHelper(s2));
}
bool ezMenu::sort_asc_caption_ci(const char *s1, const char *s2) {
  return 0 > strcasecmp(captionSortHelper(s1), captionSortHelper(s2));
}
bool ezMenu::sort_dsc_caption_cs(const char *s1, const char *s2) {
  return 0 < strcmp(captionSortHelper(s1), captionSortHelper(s2));
}
bool ezMenu::sort_dsc_caption_ci(const char *s1, const char *s2) {
  return 0 < strcasecmp(captionSortHelper(s1), captionSortHelper(s2));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   P R O G R E S S B A R
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ezProgressBar::ezProgressBar(const std::string header /* = "" */,
                             std::vector<std::string> msg /* = "" */,
                             std::vector<std::string> buttons /* = "" */,
                             const GFXfont *font /* = NULL */,
                             uint16_t color /* = NO_COLOR */,
                             uint16_t bar_color /* = NO_COLOR */) {
  if (!font)
    font = ez.theme->msg_font;
  if (color == NO_COLOR)
    color = ez.theme->msg_color;
  if (bar_color == NO_COLOR)
    bar_color = ez.theme->progressbar_color;
  _bar_color = bar_color;
  ez.screen.clear();
  M5.Lcd.fillRect(0, 0, TFT_W, TFT_H, ez.screen.background());

  if (header != "")
    ez.header.show(header);
  ez.buttons.show(buttons);
  std::vector<line_t> lines;
  M5.Lcd.setTextDatum(CC_DATUM);
  M5.Lcd.setTextColor(color);
  ez.setFont(font);
  ez._fitLines(msg, ez.canvas.width() - 2 * ez.theme->msg_hmargin, ez.canvas.width() / 3, lines);
  uint8_t font_h = ez.fontHeight();
  uint8_t num_lines = lines.size() + 2;
  for (uint8_t n = 0; n < lines.size(); n++) {
    int16_t y =
        ez.canvas.top() + ez.canvas.height() / 2 - ((num_lines - 1) * font_h / 2) + n * font_h;
    M5.Lcd.drawString(lines[n].line.c_str(), TFT_W / 2, y);
  }
  _bar_y = ez.canvas.top() + ez.canvas.height() / 2 + ((num_lines - 1) * font_h / 2)
           - ez.theme->progressbar_width / 2;
  for (uint8_t n = 0; n < ez.theme->progressbar_line_width; n++) {
    M5.Lcd.drawRect(ez.canvas.left() + ez.theme->msg_hmargin + n, _bar_y + n,
                    ez.canvas.width() - 2 * ez.theme->msg_hmargin - 2 * n,
                    ez.theme->progressbar_width - 2 * n, bar_color);
  }
}

void ezProgressBar::value(float val) {
  uint16_t left = ez.canvas.left() + ez.theme->msg_hmargin + ez.theme->progressbar_line_width;
  uint16_t width = (int16_t)(ez.canvas.width() - 2 * ez.theme->msg_hmargin
                             - 2 * ez.theme->progressbar_line_width);
  M5.Lcd.fillRect(left, _bar_y + ez.theme->progressbar_line_width, width * val / 100,
                  ez.theme->progressbar_width - 2 * ez.theme->progressbar_line_width, _bar_color);
  M5.Lcd.fillRect(left + (width * val / 100), _bar_y + ez.theme->progressbar_line_width,
                  width - (width * val / 100),
                  ez.theme->progressbar_width - 2 * ez.theme->progressbar_line_width,
                  ez.screen.background());
}

M5ez ez;
