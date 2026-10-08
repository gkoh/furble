#ifndef FURBLE_X3_APP_H
#define FURBLE_X3_APP_H

#include <atomic>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "FurbleControl.h"
#include "FurbleIntervalometer.h"
#include "FurbleSettings.h"
#include "x3/FurbleX3View.h"

namespace Furble {

class X3App {
 public:
  enum class Key { UP, DOWN, LEFT, RIGHT, SELECT, BACK, POWER };
  static constexpr Key RIGHT_SIDE_KEY = Key::DOWN;
  struct KeyEvent {
    Key key;
    bool pressed;
  };
  struct Hooks {
    uint32_t (*tick)(void);
    bool (*poll)(KeyEvent &event);
    void (*draw)(const X3View &view);
    int16_t (*batteryPercent)(void);
    void (*powerOff)(void);
  };

  explicit X3App(Hooks hooks);
  /** Runs input and interval timing; lifecycle and display use separate tasks. */
  void task(void);

 private:
  static constexpr uint8_t MAX_CAMERAS = 16;
  static constexpr uint8_t MAX_TARGETS = 2;
  enum class Page {
    MAIN,
    CAMERAS,
    CAMERA_ACTION,
    CONNECTING,
    REMOTE,
    TIMER,
    EDIT,
    RUN,
    SETTINGS,
    ABOUT
  };
  enum class Operation { SCAN, LOAD, STOP_SCAN, CONNECT, DELETE, DISCONNECT, SAVE, POWER_OFF };
  struct Config {
    interval_t interval;
    bool autoconnect;
    bool reconnect;
    bool multiconnect;
    bool fauxny;
    uint8_t power;
    uint8_t inactivity;
  };
  struct Request {
    Operation operation;
    uint16_t selection;
    Config config;
    uint32_t id;
    uint32_t revision;
    uint32_t cancelEpoch;
  };
  struct Snapshot {
    char cameras[MAX_CAMERAS][64] = {};
    char connecting[64] = {};
    char message[80] = {};
    uint8_t count = 0;
    uint8_t progress = 0;
    int16_t batteryPercent = -1;
    uint16_t total = 0;
    bool scan = false;
    bool busy = false;
    bool commandFailed = false;
    bool connectFailed = false;
    uint32_t cancelEpoch = 0;
    bool powerFailed = false;
    uint32_t revision = 0;
    uint32_t completedRequest = 0;
    Control::state_t state = Control::STATE_IDLE;
  };

  Hooks m_Hooks;
  Config m_Config;
  Config m_InitialConfig;
  Snapshot m_Snapshot;
  Intervalometer m_Timer;
  QueueHandle_t m_Work = nullptr;
  QueueHandle_t m_Results = nullptr;
  QueueHandle_t m_Display = nullptr;
  std::atomic<bool> m_ScanDirty {false};
  std::atomic<bool> m_DisconnectRequested {false};
  std::atomic<bool> m_ShutdownRequested {false};
  std::atomic<uint32_t> m_CancelEpoch {0};
  uint32_t m_RequestId = 0;
  uint32_t m_ListPending = 0;
  bool m_CommandFailed = false;
  uint8_t m_ActionCamera = 0;
  Page m_Page = Page::MAIN;
  uint8_t m_Cursor = 0;
  uint8_t m_EditField = 0;
  uint16_t m_Selection = 0;
  uint32_t m_LastInput = 0;
  int16_t m_Battery = -1;
  bool m_FocusHeld = false;
  bool m_ShutterHeld = false;
  bool m_Bulb = false;
  bool m_SelectHeld = false;
  bool m_PowerRequested = false;
  char m_Message[80] = {};

  void applySnapshot(const Snapshot &snapshot);
  static bool currentSelection(const Request &request, const Snapshot &snapshot);
  void worker(void);
  void display(void);
  void input(const KeyEvent &event);
  void navigate(Page page);
  bool request(Operation operation, uint16_t selection = 0);
  bool command(Control::cmd_t command);
  void action(Intervalometer::Action action);
  void releaseControls(void);
  void save(void);
  void edit(int change);
  uint8_t rows(void) const;
  X3View view(uint32_t now) const;
};

}  // namespace Furble

#endif
