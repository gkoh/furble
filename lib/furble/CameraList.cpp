#include <NimBLEAdvertisedDevice.h>
#include <Preferences.h>

#include "CanonEOSM6.h"
#include "CanonEOSR.h"
#include "FauxNY.h"
#include "FujifilmBasic.h"
#include "FujifilmSecure.h"
#include "MobileDevice.h"
#include "Nikon.h"
#include "Sony.h"

#include "CameraList.h"

#define FURBLE_PREF_INDEX "index"

namespace Furble {

std::vector<std::unique_ptr<Furble::Camera>> CameraList::m_ConnectList;
Preferences CameraList::m_Prefs;

/**
 * Non-volatile storage index entry.
 *
 * The name is unique identifier along with type of device so we can deserialise
 * properly.
 */
typedef struct {
  char name[16];
  Camera::Type type;
} index_entry_t;

void CameraList::fillSaveEntry(index_entry_t &entry, const Camera *camera) {
  snprintf(entry.name, 16, "%08llX", (uint64_t)camera->getAddress());
  entry.type = camera->getType();
}

void CameraList::save_index(std::vector<CameraList::index_entry_t> &index) {
  if (index.size() > 0) {
    m_Prefs.putBytes(FURBLE_PREF_INDEX, index.data(), sizeof(index[0]) * index.size());
  } else {
    m_Prefs.remove(FURBLE_PREF_INDEX);
  }
}

std::vector<CameraList::index_entry_t> CameraList::load_index(void) {
  std::vector<index_entry_t> index;

  if (m_Prefs.isKey(FURBLE_PREF_INDEX)) {
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
  }

  return index;
}

void CameraList::add_index(std::vector<CameraList::index_entry_t> &index, index_entry_t &entry) {
  bool exists = false;
  for (auto &i : index) {
    ESP_LOGD(LOG_TAG, "%s : %s", i.name, entry.name);
    if (strcmp(i.name, entry.name) == 0) {
      ESP_LOGI(LOG_TAG, "Overwriting existing entry: %s", entry.name);
      i = entry;
      exists = true;
      break;
    }
  }

  if (!exists) {
    ESP_LOGI(LOG_TAG, "Adding new entry: %s", entry.name);
    index.push_back(entry);
  }
}

void CameraList::save(const Furble::Camera *camera) {
  m_Prefs.begin(FURBLE_STR, false);
  std::vector<index_entry_t> index = load_index();

  index_entry_t entry;
  fillSaveEntry(entry, camera);

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

  index_entry_t entry;
  fillSaveEntry(entry, camera);

  size_t i = 0;
  for (i = 0; i < index.size(); i++) {
    if (strcmp(index[i].name, entry.name) == 0) {
      ESP_LOGI(LOG_TAG, "Deleting: %s", entry.name);
      index.erase(index.begin() + i);
      break;
    }
  }

  m_Prefs.remove(entry.name);
  save_index(index);

  m_Prefs.end();

  // delete bond whether needed or not
  NimBLEDevice::deleteBond(camera->getAddress());
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
  for (const auto &i : index) {
    size_t dbytes = m_Prefs.getBytesLength(i.name);
    if (dbytes == 0) {
      continue;
    }
    uint8_t dbuffer[dbytes] = {0};
    m_Prefs.getBytes(i.name, dbuffer, dbytes);

    switch (i.type) {
      case Camera::Type::FUJIFILM_BASIC:
        m_ConnectList.push_back(
            std::unique_ptr<Furble::Camera>(new FujifilmBasic(dbuffer, dbytes)));
        break;
      case Camera::Type::CANON_EOS_M6:
        m_ConnectList.push_back(std::unique_ptr<Furble::Camera>(new CanonEOSM6(dbuffer, dbytes)));
        break;
      case Camera::Type::CANON_EOS_R:
        m_ConnectList.push_back(std::unique_ptr<Furble::Camera>(new CanonEOSR(dbuffer, dbytes)));
        break;
      case Camera::Type::MOBILE_DEVICE:
        m_ConnectList.push_back(std::unique_ptr<Furble::Camera>(new MobileDevice(dbuffer, dbytes)));
        break;
      case Camera::Type::FAUXNY:
        m_ConnectList.push_back(std::unique_ptr<Furble::Camera>(new FauxNY(dbuffer, dbytes)));
        break;
      case Camera::Type::NIKON:
        m_ConnectList.push_back(std::unique_ptr<Furble::Camera>(new Nikon(dbuffer, dbytes)));
        break;
      case Camera::Type::SONY:
        m_ConnectList.push_back(std::unique_ptr<Furble::Camera>(new Sony(dbuffer, dbytes)));
        break;
      case Camera::Type::FUJIFILM_SECURE:
        m_ConnectList.push_back(
            std::unique_ptr<Furble::Camera>(new FujifilmSecure(dbuffer, dbytes)));
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

Furble::Camera *CameraList::last(void) {
  return m_ConnectList.back().get();
}

Furble::Camera *CameraList::get(size_t n) {
  return m_ConnectList[n].get();
}

bool CameraList::match(const NimBLEAdvertisedDevice *pDevice) {
  if (FujifilmBasic::matches(pDevice)) {
    m_ConnectList.push_back(std::unique_ptr<Furble::Camera>(new Furble::FujifilmBasic(pDevice)));
    return true;
  } else if (CanonEOSM6::matches(pDevice)) {
    m_ConnectList.push_back(std::unique_ptr<Furble::Camera>(new Furble::CanonEOSM6(pDevice)));
    return true;
  } else if (CanonEOSR::matches(pDevice)) {
    m_ConnectList.push_back(std::unique_ptr<Furble::Camera>(new Furble::CanonEOSR(pDevice)));
    return true;
  } else if (Nikon::matches(pDevice)) {
    m_ConnectList.push_back(std::unique_ptr<Furble::Camera>(new Furble::Nikon(pDevice)));
    return true;
  } else if (Sony::matches(pDevice)) {
    m_ConnectList.push_back(std::unique_ptr<Furble::Camera>(new Furble::Sony(pDevice)));
    return true;
  } else if (FujifilmSecure::matches(pDevice)) {
    m_ConnectList.push_back(std::unique_ptr<Furble::Camera>(new Furble::FujifilmSecure(pDevice)));
    return true;
  }

  return false;
}

void CameraList::add(const NimBLEAddress &address, const std::string &name) {
  m_ConnectList.push_back(std::unique_ptr<Furble::Camera>(new Furble::MobileDevice(address, name)));
}

void CameraList::addFauxNY(void) {
  m_ConnectList.push_back(std::unique_ptr<Furble::Camera>(new Furble::FauxNY()));
}

}  // namespace Furble
