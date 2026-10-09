#include <algorithm>
#include <cstdio>
#include <cstring>

#include <esp_heap_caps.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "CameraList.h"
#include "FurbleFatal.h"
#include "Scan.h"
#include "x3/FurbleX3App.h"

namespace Furble {

namespace {
template <size_t N>
void copy(char (&destination)[N], const char *source) {
  snprintf(destination, N, "%s", source);
}

const char *units(SpinValue::unit_t unit) {
  switch (unit) {
    case SpinValue::UNIT_MS:
      return "ms";
    case SpinValue::UNIT_SEC:
      return "sec";
    case SpinValue::UNIT_MIN:
      return "min";
    case SpinValue::UNIT_INF:
      return "infinite";
    default:
      return "count";
  }
}

const char *phase(IntervalTimer::State state) {
  switch (state) {
    case IntervalTimer::State::WAIT:
      return "Initial wait";
    case IntervalTimer::State::SHUTTER_OPEN:
      return "Shutter open";
    case IntervalTimer::State::DELAY:
      return "Between captures";
    case IntervalTimer::State::FINISHED:
      return "Finished";
    default:
      return "Stopped";
  }
}
}  // namespace

X3App::X3App(Hooks hooks) : m_Hooks(hooks) {
  m_Config.interval = Settings::load<Settings::INTERVAL>();
  m_Config.autoconnect = Settings::load<Settings::AUTOCONNECT>();
  m_Config.reconnect = Settings::load<Settings::RECONNECT>();
  m_Config.multiconnect = Settings::load<Settings::MULTICONNECT>();
  m_Config.fauxny = Settings::load<Settings::FAUXNY>();
  m_Config.power = std::min<uint8_t>(Settings::load<Settings::TX_POWER>(), 2);
  m_Config.inactivity = Settings::load<Settings::INACTIVITY>();
  m_InitialConfig = m_Config;
  m_Work = xQueueCreate(8, sizeof(Request));
  m_Results = xQueueCreate(1, sizeof(Snapshot));
  m_Display = xQueueCreate(1, sizeof(X3View));
  if (!m_Work || !m_Results || !m_Display) {
    ESP_LOGE(LOG_TAG, "X3 application queue allocation failed");
    fatal();
  }
}

void X3App::task(void) {
  if (xTaskCreate([](void *app) { static_cast<X3App *>(app)->worker(); }, "x3-lifecycle", 8192,
                  this, 2, nullptr)
          != pdPASS
      || xTaskCreate([](void *app) { static_cast<X3App *>(app)->display(); }, "x3-display", 4096,
                     this, 1, nullptr)
             != pdPASS) {
    ESP_LOGE(LOG_TAG, "X3 application task allocation failed");
    fatal();
  }
  m_LastInput = m_Hooks.tick();
  X3View previous {};
  bool first = true;
  for (;;) {
    const uint32_t now = m_Hooks.tick();
    Snapshot snapshot;
    if (xQueueReceive(m_Results, &snapshot, 0) == pdTRUE) {
      applySnapshot(snapshot);
    }
    serviceReleases();

    KeyEvent event;
    for (uint8_t n = 0; n < 16 && m_Hooks.poll(event); ++n) {
      m_LastInput = now;
      input(event);
    }
    if (m_Timer.isRunning()) {
      action(m_Timer.update(now, Control::getInstance().getState() == Control::STATE_ACTIVE));
    }
    if (!m_PowerRequested && m_Config.inactivity != 0 && !m_Timer.isRunning() && !m_ShutterHeld
        && !m_FocusHeld && m_Snapshot.state == Control::STATE_IDLE
        && now - m_LastInput >= static_cast<uint32_t>(m_Config.inactivity) * 30000) {
      m_PowerRequested = request(Operation::POWER_OFF);
    }
    const X3View current = view(now);
    if (first || memcmp(&current, &previous, sizeof(current)) != 0) {
      xQueueOverwrite(m_Display, &current);
      previous = current;
      first = false;
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void X3App::applySnapshot(const Snapshot &snapshot) {
  const auto previousState = m_Snapshot.state;
  const bool newCommandFailure = snapshot.commandFailures != m_Snapshot.commandFailures;
  if (snapshot.revision != m_Snapshot.revision) {
    m_Selection = 0;
    m_Cursor = 0;
    if (m_Page == Page::CAMERA_ACTION)
      navigate(Page::CAMERAS);
  }
  m_Snapshot = snapshot;
  m_Battery = snapshot.batteryPercent;
  if (m_ListPending && snapshot.completedRequest >= m_ListPending) {
    m_ListPending = 0;
  }
  if (snapshot.powerFailed) {
    m_PowerRequested = false;
  }
  if (snapshot.cancelEpoch != m_CancelEpoch.load())
    return;
  if (newCommandFailure) {
    action(m_Timer.cancel());
    copy(m_Message, "Camera command dropped; try again");
  }
  if (snapshot.state == Control::STATE_ACTIVE && m_Page == Page::CONNECTING) {
    navigate(Page::REMOTE);
    m_Message[0] = '\0';
  } else if ((snapshot.state == Control::STATE_CONNECT_FAILED || snapshot.connectFailed)
             && m_Page == Page::CONNECTING) {
    copy(m_Message, "Connection failed; try again or rescan");
    request(Operation::DISCONNECT);
    navigate(Page::MAIN);
  } else if (snapshot.state != Control::STATE_ACTIVE && previousState == Control::STATE_ACTIVE
             && (m_Page == Page::REMOTE || m_Page == Page::TIMER || m_Page == Page::EDIT
                 || m_Page == Page::RUN)) {
    action(m_Timer.cancel());
    releaseControls();
    if (snapshot.state == Control::STATE_IDLE) {
      copy(m_Message, "Connection lost; select a camera to reconnect");
      navigate(Page::MAIN);
    } else {
      copy(m_Message, "Connection lost; reconnecting");
      navigate(Page::CONNECTING);
    }
  } else if (snapshot.state != Control::STATE_IDLE && m_Page == Page::MAIN && m_Config.autoconnect
             && m_CancelEpoch.load() == 0 && !m_PowerRequested) {
    navigate(Page::CONNECTING);
  }
}

bool X3App::currentSelection(const Request &request, const Snapshot &snapshot) {
  return request.revision == snapshot.revision;
}

void X3App::display(void) {
  X3View current;
  for (;;) {
    if (xQueueReceive(m_Display, &current, portMAX_DELAY) == pdTRUE) {
      m_Hooks.draw(current);
    }
  }
}

bool X3App::request(Operation operation, uint16_t selection) {
  if (operation == Operation::DISCONNECT || operation == Operation::POWER_OFF) {
    // Cancellation must remain available even when the lifecycle queue is full.
    m_CancelEpoch.fetch_add(1);
    m_DisconnectRequested.store(true);
    if (operation == Operation::POWER_OFF)
      m_ShutdownRequested.store(true);
    return true;
  }
  Request request {operation,       selection,           m_Config,
                   m_RequestId + 1, m_Snapshot.revision, m_CancelEpoch.load()};
  if (xQueueSend(m_Work, &request, 0) != pdTRUE) {
    copy(m_Message, "Busy; try again");
    return false;
  }
  m_RequestId = request.id;
  if (operation == Operation::SCAN || operation == Operation::LOAD
      || operation == Operation::DELETE) {
    m_ListPending = request.id;
  }
  return true;
}

bool X3App::command(Control::cmd_t command) {
  if (Control::getInstance().sendCommand(command) == pdTRUE)
    return true;
  if (Control::getInstance().getState() == Control::STATE_ACTIVE)
    copy(m_Message, "Camera command queue full; try again");
  return false;
}

void X3App::action(IntervalTimer::Action action) {
  if (action == IntervalTimer::Action::SHUTTER_PRESS) {
    serviceReleases();
    if (m_ShutterReleasePending || !command(Control::CMD_SHUTTER_PRESS)) {
      if (m_Timer.cancel() == IntervalTimer::Action::SHUTTER_RELEASE) {
        m_ShutterReleasePending = true;
        serviceReleases();
      }
    }
  } else if (action == IntervalTimer::Action::SHUTTER_RELEASE) {
    m_ShutterReleasePending = true;
    serviceReleases();
  }
}

void X3App::releaseControls(void) {
  m_ShutterReleasePending |= m_ShutterHeld || m_Bulb;
  m_FocusReleasePending |= m_FocusHeld;
  m_ShutterHeld = false;
  m_FocusHeld = false;
  m_Bulb = false;
  serviceReleases();
}

void X3App::serviceReleases(void) {
  auto &control = Control::getInstance();
  if (control.getState() == Control::STATE_IDLE) {
    m_ShutterReleasePending = false;
    m_FocusReleasePending = false;
    return;
  }
  if (m_ShutterReleasePending && command(Control::CMD_SHUTTER_RELEASE))
    m_ShutterReleasePending = false;
  if (m_FocusReleasePending && command(Control::CMD_FOCUS_RELEASE))
    m_FocusReleasePending = false;
}

void X3App::navigate(Page page) {
  m_Page = page;
  m_Cursor = 0;
}

void X3App::save(void) {
  request(Operation::SAVE);
}

uint8_t X3App::rows(void) const {
  switch (m_Page) {
    case Page::MAIN:
      return 5;
    case Page::CAMERAS:
      return m_Snapshot.count + 1;
    case Page::CAMERA_ACTION:
      return m_Snapshot.scan ? 2 : 3;
    case Page::REMOTE:
    case Page::TIMER:
      return 5;
    case Page::EDIT:
      return 2;
    case Page::SETTINGS:
      return 6;
    default:
      return 1;
  }
}

void X3App::edit(int change) {
  auto &interval = m_Config.interval;
  auto &value = m_EditField == 0   ? interval.count
                : m_EditField == 1 ? interval.wait
                : m_EditField == 2 ? interval.shutter
                                   : interval.delay;
  if (m_Cursor == 0) {
    const int minimum = m_EditField == 0 ? 1 : 0;
    value.value = std::max(minimum, std::min(999, static_cast<int>(value.value) + change));
  } else if (m_EditField == 0) {
    value.unit = value.unit == SpinValue::UNIT_INF ? SpinValue::UNIT_NIL : SpinValue::UNIT_INF;
    if (value.value == 0) {
      value.value = 1;
    }
  } else {
    const int current = value.unit >= SpinValue::UNIT_MS && value.unit <= SpinValue::UNIT_MIN
                            ? static_cast<int>(value.unit) - SpinValue::UNIT_MS
                            : 0;
    value.unit = static_cast<SpinValue::unit_t>(SpinValue::UNIT_MS + (current + change + 3) % 3);
  }
}

void X3App::input(const KeyEvent &event) {
  // The side shutter belongs to ADC bank 2; front Confirm focus is bank 1.
  // Releases are processed even after navigation or shutdown begins.
  if (event.key == RIGHT_SIDE_KEY && !event.pressed && m_ShutterHeld) {
    m_ShutterHeld = false;
    if (!m_Bulb) {
      m_ShutterReleasePending = true;
      serviceReleases();
    }
    return;
  }
  if (m_Page == Page::REMOTE && event.key == RIGHT_SIDE_KEY && event.pressed && !m_PowerRequested) {
    serviceReleases();
    if (!m_ShutterHeld && !m_ShutterReleasePending
        && Control::getInstance().getState() == Control::STATE_ACTIVE)
      m_ShutterHeld = m_Bulb || command(Control::CMD_SHUTTER_PRESS);
    return;
  }
  if (event.key == Key::SELECT && !event.pressed) {
    m_SelectHeld = false;
    if (m_FocusHeld) {
      m_FocusHeld = false;
      m_FocusReleasePending = true;
      serviceReleases();
    }
    return;
  }
  if (!event.pressed || m_PowerRequested) {
    return;
  }
  if (event.key == Key::SELECT) {
    if (m_SelectHeld) {
      return;
    }
    m_SelectHeld = true;
  }
  if (event.key == Key::POWER) {
    action(m_Timer.cancel());
    releaseControls();
    m_PowerRequested = request(Operation::POWER_OFF);
    return;
  }
  if (event.key == Key::BACK) {
    if (m_Page == Page::RUN) {
      action(m_Timer.cancel());
      navigate(Page::TIMER);
    } else if (m_Page == Page::TIMER) {
      navigate(Page::REMOTE);
    } else if (m_Page == Page::CAMERA_ACTION) {
      navigate(Page::CAMERAS);
      m_Cursor = m_ActionCamera;
    } else if (m_Page == Page::EDIT) {
      save();
      navigate(Page::TIMER);
    } else if (m_Page == Page::REMOTE || m_Page == Page::CONNECTING) {
      action(m_Timer.cancel());
      releaseControls();
      request(Operation::DISCONNECT);
      navigate(Page::MAIN);
    } else {
      request(Operation::STOP_SCAN);
      navigate(Page::MAIN);
    }
    return;
  }
  const int change = event.key == Key::LEFT ? -1 : 1;
  if (m_Page == Page::EDIT) {
    if (event.key == Key::SELECT)
      m_Cursor = (m_Cursor + 1) % 2;
    else if (event.key == Key::LEFT || event.key == Key::RIGHT)
      edit(change);
    return;
  }
  if (event.key == Key::LEFT || event.key == Key::RIGHT) {
    const uint8_t count = rows();
    m_Cursor = (m_Cursor + count + change) % count;
    return;
  }
  if (m_Page == Page::SETTINGS && event.key == Key::SELECT) {
    switch (m_Cursor) {
      case 0:
        m_Config.autoconnect = !m_Config.autoconnect;
        break;
      case 1:
        m_Config.reconnect = !m_Config.reconnect;
        break;
      case 2:
        m_Config.multiconnect = !m_Config.multiconnect;
        break;
      case 3:
        m_Config.fauxny = !m_Config.fauxny;
        break;
      case 4:
        m_Config.power = (m_Config.power + 1) % 3;
        break;
      case 5:
        m_Config.inactivity = (m_Config.inactivity + 1) % 21;
        break;
    }
    save();
    return;
  }
  if (m_Page == Page::CAMERAS && m_ListPending)
    return;
  if (m_Page == Page::CAMERAS && m_Cursor < m_Snapshot.count && event.key == Key::SELECT) {
    m_ActionCamera = m_Cursor;
    navigate(Page::CAMERA_ACTION);
    return;
  }
  if (event.key != Key::SELECT) {
    return;
  }
  switch (m_Page) {
    case Page::MAIN:
      m_Message[0] = '\0';
      if (m_Cursor < 2) {
        if (request(m_Cursor == 0 ? Operation::SCAN : Operation::LOAD)) {
          m_Snapshot.count = 0;
          m_Selection = 0;
          navigate(Page::CAMERAS);
        }
      } else if (m_Cursor == 2) {
        navigate(Page::SETTINGS);
      } else if (m_Cursor == 3) {
        navigate(Page::ABOUT);
      } else {
        m_PowerRequested = request(Operation::POWER_OFF);
      }
      break;
    case Page::CAMERA_ACTION:
    {
      if (m_ListPending || m_ActionCamera >= m_Snapshot.count)
        return;
      const uint16_t bit = 1U << m_ActionCamera;
      if (m_Cursor == 0) {
        if (request(Operation::CONNECT, bit))
          navigate(Page::CONNECTING);
      } else if (m_Cursor == 1) {
        const uint16_t selected = m_Selection ^ bit;
        if (__builtin_popcount(selected) > MAX_TARGETS)
          copy(m_Message, "Select at most two cameras");
        else {
          m_Selection = selected;
          navigate(Page::CAMERAS);
          m_Cursor = m_ActionCamera;
        }
      } else if (!m_Snapshot.scan && request(Operation::DELETE, bit)) {
        m_Selection = 0;
        navigate(Page::CAMERAS);
      }
      break;
    }
    case Page::CAMERAS:
      if (m_Selection == 0) {
        copy(m_Message, "Select a camera first");
      } else if (request(Operation::CONNECT, m_Selection)) {
        navigate(Page::CONNECTING);
      }
      break;
    case Page::REMOTE:
      if (Control::getInstance().getState() != Control::STATE_ACTIVE) {
        return;
      }
      if (m_Cursor == 1) {
        serviceReleases();
        if (!m_FocusReleasePending)
          m_FocusHeld = command(Control::CMD_FOCUS_PRESS);
      } else if (m_Cursor == 2) {
        if (m_Bulb) {
          if (!m_ShutterHeld) {
            m_ShutterReleasePending = true;
            serviceReleases();
          }
          m_Bulb = false;
        } else {
          serviceReleases();
          if (!m_ShutterReleasePending)
            m_Bulb = m_ShutterHeld || command(Control::CMD_SHUTTER_PRESS);
        }
      } else if (m_Cursor == 3) {
        releaseControls();
        navigate(Page::TIMER);
      } else if (m_Cursor == 4) {
        releaseControls();
        request(Operation::DISCONNECT);
        navigate(Page::MAIN);
      }
      break;
    case Page::TIMER:
      if (m_Cursor < 4) {
        m_EditField = m_Cursor;
        navigate(Page::EDIT);
      } else if (Control::getInstance().getState() == Control::STATE_ACTIVE
                 && m_Timer.start(m_Config.interval, m_Hooks.tick())) {
        navigate(Page::RUN);
      } else {
        copy(m_Message, "Connect a camera and check interval settings");
      }
      break;
    case Page::RUN:
      action(m_Timer.cancel());
      navigate(Page::TIMER);
      break;
    default:
      break;
  }
}

void X3App::worker(void) {
  auto &scan = Scan::getInstance();
  auto &control = Control::getInstance();
  Snapshot snapshot;
  Config config = m_InitialConfig;
  Camera *active[MAX_TARGETS] = {};
  uint8_t activeCount = 0;
  bool savePairing = false;
  bool scanning = false;
  auto refresh = [&] {
    auto names = CameraList::snapshotNames(MAX_CAMERAS);
    snapshot.count = names.size();
    snapshot.total = std::min<size_t>(UINT16_MAX, CameraList::size());
    memset(snapshot.cameras, 0, sizeof(snapshot.cameras));
    for (size_t n = 0; n < names.size(); ++n) {
      copy(snapshot.cameras[n], names[n].c_str());
    }
  };
  auto beginScan = [&] {
    scan.start([](void *app) { static_cast<X3App *>(app)->m_ScanDirty.store(true); }, this);
    scanning = true;
    snapshot.scan = true;
  };
  auto disconnect = [&] {
    scan.stop();
    scanning = false;
    control.disconnect();
    activeCount = 0;
    savePairing = false;
    snapshot.state = Control::STATE_IDLE;
    snapshot.cancelEpoch = m_CancelEpoch.load();
  };
  auto connect = [&](uint16_t selection) {
    snapshot.connectFailed = false;
    scan.stop();
    scanning = false;
    activeCount = 0;
    for (uint8_t n = 0; n < snapshot.count && activeCount < MAX_TARGETS; ++n) {
      if (selection & (1U << n)) {
        auto *camera = CameraList::get(n);
        if (!camera || !control.addActive(camera)) {
          disconnect();
          snapshot.connectFailed = true;
          copy(snapshot.message, "Unable to allocate camera target; try again");
          return;
        }
        camera->setActive(true);
        active[activeCount++] = camera;
      }
    }
    if (activeCount != 0) {
      savePairing = snapshot.scan;
      if (!control.connectAll(config.reconnect)) {
        snapshot.connectFailed = true;
        copy(snapshot.message, "Could not start camera connection");
        disconnect();
        return;
      }
      snapshot.state = Control::STATE_CONNECT;
    } else {
      snapshot.connectFailed = true;
      copy(snapshot.message, "No camera selected");
    }
  };

  CameraList::load();
  refresh();
  ++snapshot.revision;
  if (config.autoconnect && snapshot.count != 0 && m_CancelEpoch.load() == 0) {
    connect(1);
  }
  uint32_t lastRefresh = m_Hooks.tick();
  uint32_t lastBattery = lastRefresh;
  snapshot.batteryPercent = m_Hooks.batteryPercent();
  for (;;) {
    if (m_DisconnectRequested.exchange(false))
      disconnect();
    if (m_ShutdownRequested.exchange(false)) {
      disconnect();
      m_Hooks.powerOff();
      snapshot.powerFailed = true;
      copy(snapshot.message, "Power-off failed; try again");
    }
    Request request;
    if (xQueueReceive(m_Work, &request, pdMS_TO_TICKS(50)) == pdTRUE) {
      snapshot.busy = true;
      xQueueOverwrite(m_Results, &snapshot);
      snapshot.message[0] = '\0';
      snapshot.powerFailed = false;
      const bool cancelled =
          request.cancelEpoch != m_CancelEpoch.load() && request.operation != Operation::SAVE;
      if (cancelled) {
        snapshot.completedRequest = request.id;
        snapshot.busy = false;
        xQueueOverwrite(m_Results, &snapshot);
        continue;
      }
      switch (request.operation) {
        case Operation::SCAN:
        case Operation::LOAD:
          disconnect();
          config = request.config;
          ++snapshot.revision;
          CameraList::clear();
          snapshot.scan = request.operation == Operation::SCAN;
          if (snapshot.scan) {
            if (config.fauxny) {
              CameraList::addFauxNY();
            }
            beginScan();
          } else {
            CameraList::load();
          }
          refresh();
          break;
        case Operation::STOP_SCAN:
          scan.stop();
          scanning = false;
          break;
        case Operation::CONNECT:
          config = request.config;
          if (!currentSelection(request, snapshot)) {
            snapshot.connectFailed = true;
            copy(snapshot.message, "Camera list changed; select again");
          } else if (control.getState() == Control::STATE_IDLE) {
            connect(request.selection);
          }
          break;
        case Operation::DELETE:
          scan.stop();
          scanning = false;
          if (currentSelection(request, snapshot) && control.getState() == Control::STATE_IDLE
              && !snapshot.scan) {
            for (uint8_t n = 0; n < snapshot.count; ++n) {
              if (request.selection & (1U << n)) {
                CameraList::remove(CameraList::get(n));
              }
            }
            ++snapshot.revision;
            CameraList::clear();
            CameraList::load();
            refresh();
          }
          break;
        case Operation::SAVE:
          config = request.config;
          Settings::save<Settings::INTERVAL>(config.interval);
          Settings::save<Settings::AUTOCONNECT>(config.autoconnect);
          Settings::save<Settings::RECONNECT>(config.reconnect);
          Settings::save<Settings::MULTICONNECT>(config.multiconnect);
          Settings::save<Settings::FAUXNY>(config.fauxny);
          Settings::save<Settings::TX_POWER>(config.power);
          Settings::save<Settings::INACTIVITY>(config.inactivity);
          control.setPower(Settings::load<esp_power_level_t>(Settings::TX_POWER));
          break;
        default:
          // DISCONNECT and POWER_OFF use priority flags, never this queue.
          break;
      }
      snapshot.completedRequest = request.id;
      snapshot.busy = false;
    }
    const auto status = control.getConnectionStatus();
    snapshot.state = status.state;
    snapshot.commandFailures = status.commandFailures;
    snapshot.progress = status.progress;
    copy(snapshot.connecting, status.connectingName.c_str());
    if (savePairing && snapshot.state == Control::STATE_ACTIVE) {
      for (uint8_t n = 0; n < activeCount; ++n) {
        CameraList::save(active[n]);
      }
      savePairing = false;
      ESP_LOGI(LOG_TAG, "X3 connected: free heap=%lu minimum=%lu",
               static_cast<unsigned long>(esp_get_free_heap_size()),
               static_cast<unsigned long>(esp_get_minimum_free_heap_size()));
    }
    const uint32_t now = m_Hooks.tick();
    if (now - lastBattery >= 5000) {
      snapshot.batteryPercent = m_Hooks.batteryPercent();
      lastBattery = now;
    }
    if (scanning && now - lastRefresh >= 200 && m_ScanDirty.exchange(false)) {
      refresh();
      lastRefresh = now;
    }
    xQueueOverwrite(m_Results, &snapshot);
  }
}

X3View X3App::view(uint32_t now) const {
  X3View result;
  result.batteryPercent = m_Battery;
  result.selected = m_Cursor;
  copy(result.footer, "Prev/Next: choose  Confirm: open  Back: return");
  copy(result.status, m_Message[0] ? m_Message : m_Snapshot.message);
  if (m_Snapshot.busy || (m_Page == Page::CAMERAS && m_ListPending)) {
    copy(result.status, "Working...");
  }
  if (m_PowerRequested) {
    result.kind = X3View::Kind::INFO;
    copy(result.title, "Power off");
    copy(result.status, "Releasing controls and disconnecting...");
    return result;
  }
  switch (m_Page) {
    case Page::MAIN:
      copy(result.title, "Camera remote");
      result.lineCount = 5;
      copy(result.lines[0], "Scan for cameras");
      copy(result.lines[1], "Saved cameras");
      copy(result.lines[2], "Settings");
      copy(result.lines[3], "About");
      copy(result.lines[4], "Power off");
      break;
    case Page::CAMERAS:
    {
      copy(result.title, m_Snapshot.scan ? "Discover cameras" : "Saved cameras");
      const uint8_t offset = (m_Cursor / X3View::MAX_LINES) * X3View::MAX_LINES;
      result.lineCount = std::min<int>(X3View::MAX_LINES, rows() - offset);
      result.selected = m_Cursor - offset;
      for (uint8_t row = 0; row < result.lineCount; ++row) {
        const uint8_t n = offset + row;
        if (n == m_Snapshot.count) {
          copy(result.lines[row], "Connect selected cameras");
        } else {
          snprintf(result.lines[row], sizeof(result.lines[row]), "[%c] %.58s",
                   m_Selection & (1U << n) ? 'x' : ' ', m_Snapshot.cameras[n]);
        }
      }
      if (!result.status[0]) {
        snprintf(result.status, sizeof(result.status), "%u camera(s)%s", m_Snapshot.total,
                 m_Snapshot.total > MAX_CAMERAS ? "; first 16 shown" : "");
      }
      copy(result.footer, "Prev/Next: choose  Confirm: actions  Back: return");
      break;
    }
    case Page::CAMERA_ACTION:
      copy(result.title, "Camera actions");
      copy(result.status, m_Snapshot.cameras[m_ActionCamera]);
      result.lineCount = m_Snapshot.scan ? 2 : 3;
      copy(result.lines[0], "Connect this camera");
      copy(result.lines[1],
           m_Selection & (1U << m_ActionCamera) ? "Unmark camera" : "Mark camera (max 2)");
      copy(result.lines[2], "Delete saved camera");
      break;
    case Page::CONNECTING:
      result.kind = X3View::Kind::INFO;
      copy(result.title, "Connecting");
      result.selected = -1;
      result.lineCount = 3;
      copy(result.lines[0], m_Snapshot.connecting);
      snprintf(result.lines[1], sizeof(result.lines[1]), "Progress: %u%%", m_Snapshot.progress);
      copy(result.lines[2], "Back: cancel and disconnect");
      break;
    case Page::REMOTE:
      result.kind = X3View::Kind::REMOTE;
      copy(result.title, "Remote control");
      result.lineCount = 5;
      copy(result.lines[0], m_ShutterReleasePending     ? "Shutter release pending"
                            : (m_ShutterHeld || m_Bulb) ? "Shutter pressed"
                                                        : "Shutter");
      copy(result.lines[1], m_FocusReleasePending ? "Focus release pending"
                            : m_FocusHeld         ? "Focus pressed"
                                                  : "Hold Confirm: focus");
      copy(result.lines[2], m_Bulb ? "Bulb lock: ON (OK releases)" : "Bulb lock: off");
      copy(result.lines[3], "Intervalometer");
      copy(result.lines[4], "Disconnect");
      copy(result.footer, "Right side: shutter  Confirm: action  Back: disconnect");
      break;
    case Page::TIMER:
    {
      copy(result.title, "Intervalometer");
      result.lineCount = 5;
      const auto &interval = m_Config.interval;
      if (interval.count.unit == SpinValue::UNIT_INF) {
        copy(result.lines[0], "Count: infinite");
      } else {
        snprintf(result.lines[0], sizeof(result.lines[0]), "Count: %u", interval.count.value);
      }
      snprintf(result.lines[1], sizeof(result.lines[1]), "Initial wait: %u %s", interval.wait.value,
               units(interval.wait.unit));
      snprintf(result.lines[2], sizeof(result.lines[2]), "Shutter: %u %s", interval.shutter.value,
               units(interval.shutter.unit));
      snprintf(result.lines[3], sizeof(result.lines[3]), "Delay: %u %s", interval.delay.value,
               units(interval.delay.unit));
      copy(result.lines[4], "Run");
      break;
    }
    case Page::EDIT:
    {
      const char *names[] = {"Capture count", "Initial wait", "Shutter duration", "Delay"};
      copy(result.title, names[m_EditField]);
      const auto &interval = m_Config.interval;
      const auto &value = m_EditField == 0   ? interval.count
                          : m_EditField == 1 ? interval.wait
                          : m_EditField == 2 ? interval.shutter
                                             : interval.delay;
      result.lineCount = 2;
      snprintf(result.lines[0], sizeof(result.lines[0]), "Value: %u", value.value);
      snprintf(result.lines[1], sizeof(result.lines[1]), "%s: %s",
               m_EditField == 0 ? "Mode" : "Units", units(value.unit));
      copy(result.footer, "Prev/Next: change  Confirm: field  Back: save");
      break;
    }
    case Page::RUN:
      result.kind = X3View::Kind::RUN;
      copy(result.title, "Intervalometer running");
      result.lineCount = 4;
      result.selected = -1;
      copy(result.lines[0], phase(m_Timer.getState()));
      snprintf(result.lines[1], sizeof(result.lines[1]), "Completed: %lu",
               static_cast<unsigned long>(m_Timer.getCompletedCount()));
      snprintf(result.lines[2], sizeof(result.lines[2]), "Remaining: %lu sec",
               static_cast<unsigned long>((m_Timer.remaining(now) + 999) / 1000));
      copy(result.lines[3], "OK / Back: stop and release shutter");
      copy(result.footer, "Timing continues during display refresh");
      break;
    case Page::SETTINGS:
      copy(result.title, "Settings");
      result.lineCount = 6;
      snprintf(result.lines[0], sizeof(result.lines[0]), "Auto-connect: %s",
               m_Config.autoconnect ? "on" : "off");
      snprintf(result.lines[1], sizeof(result.lines[1]), "Infinite reconnect: %s",
               m_Config.reconnect ? "on" : "off");
      snprintf(result.lines[2], sizeof(result.lines[2]), "Multi-connect (max 2): %s",
               m_Config.multiconnect ? "on" : "off");
      snprintf(result.lines[3], sizeof(result.lines[3]), "FauxNY: %s",
               m_Config.fauxny ? "on" : "off");
      snprintf(result.lines[4], sizeof(result.lines[4]), "TX power: +%u dBm",
               (m_Config.power + 1) * 3);
      snprintf(result.lines[5], sizeof(result.lines[5]), "Idle power-off: %u sec (0=off)",
               m_Config.inactivity * 30);
      copy(result.footer, "Prev/Next: choose  Confirm: change  Back: return");
      break;
    case Page::ABOUT:
      result.kind = X3View::Kind::INFO;
      copy(result.title, "About furble");
      result.selected = -1;
      result.lineCount = 4;
      copy(result.lines[0], FURBLE_VERSION);
      copy(result.lines[1], "Xteink X3 / ESP32-C3");
      copy(result.lines[2], "Fujifilm, Canon, Nikon, Sony, Ricoh");
      copy(result.lines[3], "GPS unavailable on this X3 port");
      break;
  }
  return result;
}

}  // namespace Furble
