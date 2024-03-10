#include <NimBLEAdvertisedDevice.h>
#include <Preferences.h>

#include "CanonEOSM6.h"
#include "CanonEOSRP.h"
#include "Fujifilm.h"

#include "CameraList.h"

#define FURBLE_PREF_INDEX "index"

namespace Furble {

std::vector<Furble::Camera *> CameraList::m_ConnectList;
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
  m_Prefs.putBytes(FURBLE_PREF_INDEX, index.data(), sizeof(index[0]) * index.size());
}

static std::vector<index_entry_t> load_index(void) {
  std::vector<index_entry_t> index;

  size_t bytes = m_Prefs.getBytesLength(FURBLE_PREF_INDEX);
  if (bytes > 0 && (bytes % sizeof(index_entry_t) == 0)) {
    uint8_t buffer[bytes] = {0};
    size_t count = bytes / sizeof(index_entry_t);
    Serial.println("Index entries: " + String(count));
    m_Prefs.getBytes(FURBLE_PREF_INDEX, buffer, bytes);
    index_entry_t *entry = (index_entry_t *)buffer;

    for (int i = 0; i < count; i++) {
      Serial.println("Loading index entry: " + String(entry[i].name));
      index.push_back(entry[i]);
    }
  }

  return index;
}

static void add_index(std::vector<index_entry_t> &index, index_entry_t &entry) {
  bool exists = false;
  for (size_t i = 0; i < index.size(); i++) {
    Serial.println("[" + String(i) + "] " + String(index[i].name) + " : " + String(entry.name));
    if (strcmp(index[i].name, entry.name) == 0) {
      Serial.println("Overwriting existing entry");
      index[i] = entry;
      exists = true;
      break;
    }
  }

  if (!exists) {
    Serial.println("Adding new entry");
    index.push_back(entry);
  }
}

void CameraList::save(Camera *pCamera) {
  m_Prefs.begin(FURBLE_STR, false);
  std::vector<index_entry_t> index = load_index();

  index_entry_t entry = {0};
  pCamera->fillSaveName(entry.name);
  entry.type = pCamera->getDeviceType();

  add_index(index, entry);

  size_t dbytes = pCamera->getSerialisedBytes();
  uint8_t dbuffer[dbytes] = {0};
  pCamera->serialise(dbuffer, dbytes);

  // Store the entry and the index
  m_Prefs.putBytes(entry.name, dbuffer, dbytes);
  Serial.println("Saved " + String(entry.name));
  save_index(index);
  Serial.print("Index entries: ");
  Serial.println(index.size());

  m_Prefs.end();
}

void CameraList::remove(Camera *pCamera) {
  m_Prefs.begin(FURBLE_STR, false);
  std::vector<index_entry_t> index = load_index();

  index_entry_t entry = {0};
  pCamera->fillSaveName(entry.name);

  size_t i = 0;
  for (i = 0; i < index.size(); i++) {
    if (strcmp(index[i].name, entry.name) == 0) {
      Serial.print("Deleting: ");
      Serial.println(entry.name);
      break;
    }
  }

  index.erase(index.begin() + i);

  m_Prefs.remove(entry.name);
  save_index(index);

  m_Prefs.end();
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
        m_ConnectList.push_back(new Fujifilm(dbuffer, dbytes));
        break;
      case FURBLE_CANON_EOS_M6:
        m_ConnectList.push_back(new CanonEOSM6(dbuffer, dbytes));
        break;
      case FURBLE_CANON_EOS_RP:
        m_ConnectList.push_back(new CanonEOSRP(dbuffer, dbytes));
        break;
    }
  }
  m_Prefs.end();
}

void CameraList::match(NimBLEAdvertisedDevice *pDevice) {
  if (Fujifilm::matches(pDevice)) {
    m_ConnectList.push_back(new Furble::Fujifilm(pDevice));
  } else if (CanonEOSM6::matches(pDevice)) {
    m_ConnectList.push_back(new Furble::CanonEOSM6(pDevice));
  } else if (CanonEOSRP::matches(pDevice)) {
    m_ConnectList.push_back(new Furble::CanonEOSRP(pDevice));
  }
}

}  // namespace Furble
