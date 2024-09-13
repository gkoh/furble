#include <NimBLEAdvertisedDevice.h>
#include <Preferences.h>

#include "CanonEOSM6.h"
#include "CanonEOSRP.h"
#include "Fujifilm.h"
#include "MobileDevice.h"

#include "CameraList.h"

#define FURBLE_PREF_INDEX "index"

namespace Furble {

std::vector<std::unique_ptr<Furble::Camera>> CameraList::m_ConnectList;
static Preferences m_Prefs;

/**
 * Non-volatile storage index entry.
 *
 * The name is unique identifier along with type of device so we can deserialise
 * properly.
 */
typedef struct {
  char name[16];
  device_type_t type;
} index_entry_t;

static void save_index(std::vector<index_entry_t> &index) {
  if (index.size() > 0) {
    m_Prefs.putBytes(FURBLE_PREF_INDEX, index.data(), sizeof(index[0]) * index.size());
  } else {
    m_Prefs.remove(FURBLE_PREF_INDEX);
  }
}

static std::vector<index_entry_t> load_index(void) {
  std::vector<index_entry_t> index;

  size_t bytes = m_Prefs.getBytesLength(FURBLE_PREF_INDEX);
  if (bytes > 0 && (bytes % sizeof(index_entry_t) == 0)) {
    uint8_t buffer[bytes] = {0};
    size_t count = bytes / sizeof(index_entry_t);
    ESP_LOGI(LOG_TAG, "Index entries: %d", count);
    m_Prefs.getBytes(FURBLE_PREF_INDEX, buffer, bytes);
    index_entry_t *entry = (index_entry_t *)buffer;

    for (int i = 0; i < count; i++) {
      ESP_LOGI(LOG_TAG, "Loading index entry: %s", entry[i].name);
      index.push_back(entry[i]);
    }
  }

  return index;
}

static void add_index(std::vector<index_entry_t> &index, index_entry_t &entry) {
  bool exists = false;
  for (size_t i = 0; i < index.size(); i++) {
    ESP_LOGI(LOG_TAG, "[%d] %s : %s", i, index[i].name, entry.name);
    if (strcmp(index[i].name, entry.name) == 0) {
      ESP_LOGI(LOG_TAG, "Overwriting existing entry: %s", entry.name);
      index[i] = entry;
      exists = true;
      break;
    }
  }

  if (!exists) {
    ESP_LOGI(LOG_TAG, "Adding new entry: %s", entry.name);
    index.push_back(entry);
  }
}

void CameraList::save(Furble::Camera *camera) {
  m_Prefs.begin(FURBLE_STR, false);
  std::vector<index_entry_t> index = load_index();

  index_entry_t entry = {0};
  camera->fillSaveName(entry.name);
  entry.type = camera->getDeviceType();

  add_index(index, entry);

  size_t dbytes = camera->getSerialisedBytes();
  uint8_t dbuffer[dbytes] = {0};
  if (camera->serialise(dbuffer, dbytes)) {
    // Store the entry and the index if serialisation succeeds
    m_Prefs.putBytes(entry.name, dbuffer, dbytes);
    ESP_LOGI(LOG_TAG, "Saved %s", entry.name);
    save_index(index);
    ESP_LOGI(LOG_TAG, "Index entries: %d", index.size());
  }

  m_Prefs.end();
}

void CameraList::remove(Furble::Camera *camera) {
  m_Prefs.begin(FURBLE_STR, false);
  std::vector<index_entry_t> index = load_index();

  index_entry_t entry = {0};
  camera->fillSaveName(entry.name);

  size_t i = 0;
  for (i = 0; i < index.size(); i++) {
    if (strcmp(index[i].name, entry.name) == 0) {
      ESP_LOGI(LOG_TAG, "Deleting: %s", entry.name);
      break;
    }
  }

  index.erase(index.begin() + i);

  m_Prefs.remove(entry.name);
  save_index(index);

  m_Prefs.end();

  // delete bond if required
  if (NimBLEDevice::isBonded(camera->getAddress())) {
    NimBLEDevice::deleteBond(camera->getAddress());
  }
}

/**
 * Load the list of saved cameras.
 *
 * The Arduino-ESP32 NVS library does not expose an entry iterator even though
 * the underlying library supports it. We work around this by managing a simple
 * index with a known name and storing target devices in separate entries.
 */
void CameraList::load(void) {
  m_Prefs.begin(FURBLE_STR, true);
  m_ConnectList.clear();
  std::vector<index_entry_t> index = load_index();
  for (size_t i = 0; i < index.size(); i++) {
    size_t dbytes = m_Prefs.getBytesLength(index[i].name);
    if (dbytes == 0) {
      continue;
    }
    uint8_t dbuffer[dbytes] = {0};
    m_Prefs.getBytes(index[i].name, dbuffer, dbytes);

    switch (index[i].type) {
      case FURBLE_FUJIFILM:
        m_ConnectList.push_back(std::unique_ptr<Furble::Camera>(new Fujifilm(dbuffer, dbytes)));
        break;
      case FURBLE_CANON_EOS_M6:
        m_ConnectList.push_back(std::unique_ptr<Furble::Camera>(new CanonEOSM6(dbuffer, dbytes)));
        break;
      case FURBLE_CANON_EOS_RP:
        m_ConnectList.push_back(std::unique_ptr<Furble::Camera>(new CanonEOSRP(dbuffer, dbytes)));
        break;
      case FURBLE_MOBILE_DEVICE:
        m_ConnectList.push_back(std::unique_ptr<Furble::Camera>(new MobileDevice(dbuffer, dbytes)));
        break;
    }
  }
  m_Prefs.end();
}

size_t CameraList::getSaveCount(void) {
  m_Prefs.begin(FURBLE_STR, false);
  auto index = load_index();
  m_Prefs.end();

  return index.size();
}

size_t CameraList::size(void) {
  return m_ConnectList.size();
}

void CameraList::clear(void) {
  m_ConnectList.clear();
}

Furble::Camera *CameraList::get(size_t n) {
  return m_ConnectList[n].get();
}

Furble::Camera *CameraList::back(void) {
  return m_ConnectList.back().get();
}

bool CameraList::match(NimBLEAdvertisedDevice *pDevice) {
  if (Fujifilm::matches(pDevice)) {
    m_ConnectList.push_back(std::unique_ptr<Furble::Camera>(new Furble::Fujifilm(pDevice)));
    return true;
  } else if (CanonEOSM6::matches(pDevice)) {
    m_ConnectList.push_back(std::unique_ptr<Furble::Camera>(new Furble::CanonEOSM6(pDevice)));
    return true;
  } else if (CanonEOSRP::matches(pDevice)) {
    m_ConnectList.push_back(std::unique_ptr<Furble::Camera>(new Furble::CanonEOSRP(pDevice)));
    return true;
  }

  return false;
}

void CameraList::add(const NimBLEAddress &address, const std::string &name) {
  m_ConnectList.push_back(std::unique_ptr<Furble::Camera>(new Furble::MobileDevice(address, name)));
}

}  // namespace Furble
