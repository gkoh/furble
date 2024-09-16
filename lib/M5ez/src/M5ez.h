#ifndef _M5EZ_H_
#define _M5EZ_H_

#define M5EZ_VERSION "2.1.2"

// Turn this off if you don't have a battery attached
#define M5EZ_BATTERY

// Determines whether the backlight is settable
#define M5EZ_BACKLIGHT

#include <FS.h>
#include <M5Unified.h>

// Special fake font pointers to access the older non FreeFonts in a unified way.
// Only valid if passed to ez.setFont
// (Note that these need to be specified without the & in front, unlike the FreeFonts)
#define hzk16 (GFXfont *)1
#define sans16 (GFXfont *)2
#define sans26 (GFXfont *)4
#define numonly48 (GFXfont *)6
#define numonly7seg48 (GFXfont *)7
#define numonly75 (GFXfont *)8
// The following fonts are just scaled up from previous ones (textSize 2)
// But they might still be useful.
#define mono12x16 (GFXfont *)9
#define sans32 (GFXfont *)10
#define sans52 (GFXfont *)12
#define numonly96 (GFXfont *)14
#define numonly7seg96 (GFXfont *)15
#define numonly150 (GFXfont *)16

#define NO_COLOR TFT_TRANSPARENT

struct line_t {
  int16_t position;
  std::string line;
};

extern const char *M5EZ_TAG;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   T H E M E
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ezTheme {
 public:
  static void begin();
  void add();
  static bool select(std::string name);
  static void menu();

  std::string name = "Default";  // Change this when making theme
  uint16_t background = 0xEF7D;
  uint16_t foreground = TFT_BLACK;
  uint8_t header_height;
  const GFXfont *header_font;
  uint8_t header_hmargin = 5;
  uint8_t header_tmargin = 3;
  uint16_t header_bgcolor = TFT_BLUE;
  uint16_t header_fgcolor = TFT_WHITE;

  const GFXfont *print_font;
  uint16_t print_color = foreground;

  uint8_t button_height;
  const GFXfont *button_font;
  uint8_t button_tmargin = 1;
  uint8_t button_hmargin = 5;
  uint8_t button_gap = 3;
  uint8_t button_radius;
  uint16_t button_bgcolor_b = TFT_BLUE;
  uint16_t button_bgcolor_t = TFT_PURPLE;
  uint16_t button_fgcolor = TFT_WHITE;
  uint16_t button_longcolor = TFT_CYAN;

  uint8_t menu_lmargin = 15;
  uint8_t menu_rmargin = 10;
  uint8_t menu_arrows_lmargin = 4;
  uint16_t menu_item_color = foreground;
  uint16_t menu_sel_bgcolor = foreground;
  uint16_t menu_sel_fgcolor = background;
  const GFXfont *menu_big_font;
  const GFXfont *menu_small_font;
  uint8_t menu_item_hmargin = 10;
  uint8_t menu_item_radius;

  const GFXfont *msg_font;
  uint16_t msg_color = foreground;
  uint8_t msg_hmargin = 20;

  uint8_t progressbar_line_width = 4;
  uint8_t progressbar_width = 25;
  uint16_t progressbar_color = foreground;

  uint16_t signal_interval = 2000;
  uint8_t signal_bar_width = 4;
  uint8_t signal_bar_gap = 2;

  uint8_t battery_bar_width = 26;
  uint8_t battery_bar_gap = 2;
  uint16_t battery_0_fgcolor = TFT_RED;
  uint16_t battery_25_fgcolor = TFT_ORANGE;
  uint16_t battery_50_fgcolor = TFT_YELLOW;
  uint16_t battery_75_fgcolor = TFT_GREENYELLOW;
  uint16_t battery_100_fgcolor = TFT_GREEN;

 private:
  static void update(void);
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   S C R E E N
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ezScreen {
 public:
  static void begin();
  static void clear();
  static void clear(uint16_t color);
  static uint16_t background();

 private:
  static uint16_t _background;
  //
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   H E A D E R
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct header_widget_t {
  std::string name;
  bool leftover;  // widget gets all leftover width. Used by title, only one widget can have this
                  // property
  uint16_t x;
  uint16_t w;
  void (*function)(uint16_t x, uint16_t w);
};
#define LEFTMOST 0
#define RIGHTMOST 255

class ezHeader {
 private:
  static std::vector<header_widget_t> _widgets;
  static bool _shown;
  static std::string _title;
  static void _drawTitle(uint16_t x, uint16_t w);
  static void _recalculate();

 public:
  static void begin();
  static void show(std::string t = "");
  static bool shown();
  static void clear(bool wipe = true);
  static void insert(uint8_t position,
                     std::string name,
                     uint16_t width,
                     void (*function)(uint16_t x, uint16_t w),
                     bool leftover = false);
  static void remove(std::string name);
  static uint8_t position(std::string name);
  static void draw(std::string name = "");
  static std::string title();
  static void title(std::string title);
  //
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   C A N V A S
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct print_t {
  const GFXfont *font;
  uint16_t color;
  uint16_t x;
  int16_t y;
  std::string text;
};

class ezTFT {
 public:
  int32_t width;
  int32_t height;
};

class ezCanvas: public Print {
  friend class ezHeader;
  friend class ezButtons;
  friend class ezProgressBar;
  friend class ezMenu;
  friend class ezScreen;
  friend class M5ez;

 public:
  static void begin();
  static void reset();
  static void clear();
  static uint8_t top();
  static uint8_t bottom();
  static uint16_t left();
  static uint16_t right();
  static uint8_t height();
  static uint16_t width();
  static bool scroll();
  static void scroll(bool s);
  static bool wrap();
  static void wrap(bool w);
  static uint16_t lmargin();
  static void lmargin(uint16_t newmargin);
  static void font(const GFXfont *font);
  static const GFXfont *font();
  static void color(uint16_t color);
  static uint16_t color();
  static void pos(uint16_t x, uint8_t y);
  static uint16_t x();
  static void x(uint16_t newx);
  static uint8_t y();
  static void y(uint8_t newy);
  virtual size_t write(
      uint8_t c);  // These three are used to inherit print and println from Print class
  virtual size_t write(const char *str);
  virtual size_t write(const uint8_t *buffer, size_t size);
  static uint16_t loop(void *context);

 private:
  static ezTFT tft;
  static std::vector<print_t> _printed;
  static uint32_t _next_scroll;
  static void top(uint8_t newtop);
  static void bottom(uint8_t newbottom);
  static uint8_t _y, _top, _bottom;
  static uint16_t _x, _left, _right, _lmargin;
  static bool _wrap, _scroll;
  static const GFXfont *_font;
  static uint16_t _color;
  static void _print(std::string text);
  static void _putString(std::string text);
  //
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   B U T T O N S
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ezButtons {
 public:
  static void begin();
  static void show(std::vector<std::string> buttons);
  static std::vector<std::string> get();
  static void clear(bool wipe = true);
  static void releaseWait();
  static std::string poll();
  static const std::string wait();

 private:
  static std::string _btn_a;
  static std::string _btn_b;
  static std::string _btn_c;
  static bool _key_release_wait;
  static bool _lower_button_row, _upper_button_row;
  static void _drawButtons(std::string btn_a, std::string btn_b, std::string btn_c);
  static void _drawButton(int16_t row, std::string text, int16_t x, int16_t w);
  static void _drawButtonString(std::string text,
                                int16_t x,
                                int16_t y,
                                uint16_t color,
                                int16_t datum);
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   M E N U
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ezMenu {
 public:
  ezMenu(std::string hdr = "");
  bool addItem(
      std::string name,
      std::string caption = "",
      void (*simpleFunction)() = NULL,
      void *context = nullptr,
      bool (*advancedFunction)(ezMenu *callingMenu, void *context) = NULL,
      void (*drawFunction)(ezMenu *callingMenu, int16_t x, int16_t y, int16_t w, int16_t h) = NULL);
  bool deleteItem(int16_t index);
  bool deleteItem(std::string name);
  bool setCaption(std::string name, std::string caption);
  int16_t countItems(void);
  void setSortFunction(bool (*sortFunction)(const char *s1, const char *s2));
  void buttons(std::vector<std::string> bttns);
  void upOnFirst(std::string name);
  void leftOnFirst(std::string name);
  void downOnLast(std::string name);
  void rightOnLast(std::string name);
  int16_t getItemNum(std::string name);
  int16_t pick();
  std::string pickName();
  void run();
  int16_t runOnce(bool dynamic = false);
  void txtBig();
  void txtSmall();
  void txtFont(const GFXfont *font);
  void imgBackground(uint16_t color);
  void imgFromTop(int16_t offset);
  void imgCaptionFont(const GFXfont *font);
  void imgCaptionLocation(uint8_t datum);
  void imgCaptionColor(uint16_t color);
  void imgCaptionMargins(int16_t hmargin, int16_t vmargin);
  void imgCaptionMargins(int16_t margin);

  static bool sort_asc_name_cs(const char *s1, const char *s2);
  static bool sort_asc_name_ci(const char *s1, const char *s2);
  static bool sort_dsc_name_cs(const char *s1, const char *s2);
  static bool sort_dsc_name_ci(const char *s1, const char *s2);
  static bool sort_asc_caption_cs(const char *s1, const char *s2);
  static bool sort_asc_caption_ci(const char *s1, const char *s2);
  static bool sort_dsc_caption_cs(const char *s1, const char *s2);
  static bool sort_dsc_caption_ci(const char *s1, const char *s2);

 private:
  struct MenuItem_t {
    std::string name;
    std::string caption;
    void *context;
    void (*simpleFunction)();
    bool (*advancedFunction)(ezMenu *callingMenu, void *context);
    void (*drawFunction)(ezMenu *callingMenu, int16_t x, int16_t y, int16_t w, int16_t h);
  };
  std::vector<MenuItem_t> _items;
  int16_t _selected, _offset;
  bool _redraw;
  std::string _header, _pick_button;
  std::vector<std::string> _buttons;
  std::string _up_on_first, _down_on_last;
  int16_t _per_item_h, _vmargin;
  int16_t _items_per_screen;
  uint16_t _old_background;
  void _drawCaption();
  const GFXfont *_font;
  int16_t _runTextOnce(bool dynamic = false);
  void _fixOffset();
  void _drawItems();
  void _drawItem(int16_t n, std::string text, bool selected);
  void _Arrows();
  int16_t _img_from_top;
  uint8_t _img_caption_location;
  uint16_t _img_caption_color;
  uint16_t _img_background;
  const GFXfont *_img_caption_font;
  int16_t _img_caption_hmargin, _img_caption_vmargin;
  bool (*_sortFunction)(const char *s1, const char *s2);
  void _sortItems();
  bool _sortWrapper(MenuItem_t &item1, MenuItem_t &item2);
  void _replace(std::vector<std::string> &list, std::string name, std::string replacement);
  //
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   P R O G R E S S B A R
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ezProgressBar {
 public:
  ezProgressBar(const std::string header = "",
                std::vector<std::string> msg = {""},
                std::vector<std::string> buttons = {""},
                const GFXfont *font = NULL,
                uint16_t color = NO_COLOR,
                uint16_t bar_color = NO_COLOR);
  void value(float val);

 private:
  int16_t _bar_y;
  uint16_t _bar_color;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   S E T T I N G S
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ezSettings {
 public:
  static void begin();
  static void menu();
  static void defaults();
  static ezMenu menuObj;
  //
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   B A C K L I G H T
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef M5EZ_BACKLIGHT
#define NEVER 0
#define USER_SET 255
class ezBacklight {
 public:
  static void begin();
  static void menu();
  static void inactivity(uint8_t half_minutes);
  static void activity();
  static uint16_t loop(void *context);

 private:
  static uint8_t _brightness;
  static uint8_t _inactivity;
  static uint32_t _last_activity;
  static uint8_t _MinimumBrightness;
  static const uint8_t _MaxSteps = 8;
  static bool _backlight_off;
  //
};
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   B A T T E R Y
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef M5EZ_BATTERY

class ezBattery {
 public:
  static void begin();
  static uint16_t loop(void *context);
  static uint8_t getTransformedBatteryLevel();
  static uint16_t getBatteryBarColor(uint8_t batteryLevel);

 private:
  static bool _on;
  static void _refresh();
  static void _drawWidget(uint16_t x, uint16_t w);
};

#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   E Z
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct event_t {
  uint16_t (*function)(void *);
  void *context;
  uint32_t when;
};

class M5ez {
  friend class ezProgressBar;
  friend class ezHeader;  // TMP?
  friend class ezMenu;    // For _redraw

 public:
  static std::vector<ezTheme> themes;
  static ezTheme *theme;
  static ezScreen screen;
  static constexpr ezScreen &s = screen;
  static ezHeader header;
  static constexpr ezHeader &h = header;
  static ezCanvas canvas;
  static constexpr ezCanvas &c = canvas;
  static ezButtons buttons;
  static constexpr ezButtons &b = buttons;
  static ezSettings settings;
#ifdef M5EZ_BATTERY
  static ezBattery battery;
#endif
#ifdef M5EZ_BACKLIGHT
  static ezBacklight backlight;
#endif

  static void begin();

  static void yield();

  static void addEvent(uint16_t (*function)(void *), void *context = nullptr, uint32_t when = 1);
  static void removeEvent(uint16_t (*function)(void *context));
  static void redraw();

  // ez.msgBox
  static std::string msgBox(std::string header,
                            std::vector<std::string> msg,
                            std::vector<std::string> buttons = {"OK"},
                            const bool blocking = true,
                            const GFXfont *font = NULL,
                            uint16_t color = NO_COLOR,
                            const bool clear = true);

  static std::string clipString(std::string input, int16_t cutoff, bool dots = true);
  static bool isBackExitOrDone(std::string str);

  // m5.lcd wrappers that make fonts easier
  static void setFont(const GFXfont *font);
  static int16_t fontHeight();

  static std::string version();

 private:
  static std::vector<event_t> _events;
  static bool _redraw;

  static void _fitLines(std::vector<std::string> text,
                        uint16_t max_width,
                        uint16_t min_width,
                        std::vector<line_t> &lines);

  static std::size_t _findBreak(std::string text, std::size_t pos, uint16_t max_width);
};

extern M5ez ez;

#endif  //_M5EZ_H_
