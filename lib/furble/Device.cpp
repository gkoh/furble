#include <NimBLEAdvertisedDevice.h>
#include <Preferences.h>

#include "Furble.h"

#define FURBLE_PREF_INDEX "index"

namespace Furble {

static Preferences m_Prefs;

/**
 * Non-volatile storage index entry.
 *
 * The name is unique identifier along with type of device so we can deserialise
 * properly.
 */
typedef struct {
  char   name[16];
  device_type_t type;
} index_entry_t;

static void save_index(std::vector<index_entry_t> &index) {
  m_Prefs.putBytes(FURBLE_PREF_INDEX,
                   index.data(),
                   sizeof(index[0]) * index.size());
}

static std::vector<index_entry_t> load_index(void) {
  std::vector<index_entry_t> index;

  size_t bytes = m_Prefs.getBytesLength(FURBLE_PREF_INDEX);
  if (bytes > 0 && (bytes % sizeof(index_entry_t) == 0)) {
    uint8_t buffer[bytes] = { 0 };
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

static void add_index(std::vector<index_entry_t> &index,
                      index_entry_t &entry) {
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

const char *Device::getName(void) {
  return m_Name.c_str();
}

void Device::save(void) {
  m_Prefs.begin(FURBLE_STR, false);
  std::vector<index_entry_t> index = load_index();

  index_entry_t entry = { 0 };
  fillSaveName(entry.name);
  entry.type = getDeviceType();

  add_index(index, entry);

  size_t dbytes = getSerialisedBytes();
  uint8_t dbuffer[dbytes] = { 0 };
  serialise(dbuffer, dbytes);

  // Store the entry and the index
  m_Prefs.putBytes(entry.name, dbuffer, dbytes);
  Serial.println("Saved " + String(entry.name));
  save_index(index);
  Serial.print("Index entries: ");
  Serial.println(index.size());

  m_Prefs.end();
}

void Device::remove(void) {
  m_Prefs.begin(FURBLE_STR, false);
  std::vector<index_entry_t> index = load_index();

  index_entry_t entry = { 0 };
  fillSaveName(entry.name);

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
 * Load the list of saved devices.
 *
 * The Arduino-ESP32 NVS library does not expose an entry iterator even though
 * the underlying library supports it. We work around this by managing a simple
 * index with a known name and storing target devices in separate entries.
 */
void Device::loadDevices(std::vector<Furble::Device*>&device_list) {
  m_Prefs.begin(FURBLE_STR, true);
  std::vector<index_entry_t> index = load_index();
  for (size_t i = 0; i < index.size(); i++) {
    size_t dbytes = m_Prefs.getBytesLength(index[i].name);
    if (dbytes == 0) {
      continue;
    }
    uint8_t dbuffer[dbytes] = { 0 };
    m_Prefs.getBytes(index[i].name, dbuffer, dbytes);

    switch (index[i].type) {
      case FURBLE_FUJIFILM_XT30:
        device_list.push_back(new FujifilmXT30(dbuffer, dbytes));
        break;
      case FURBLE_CANON_EOS_M6:
        device_list.push_back(new CanonEOSM6(dbuffer, dbytes));
        break;
      case FURBLE_CANON_EOS_RP:
        device_list.push_back(new CanonEOSRP(dbuffer, dbytes));
        break;
    }
  }
  m_Prefs.end();
}

void Device::fillSaveName(char *name) {
  snprintf(name, 16, "%08llX", (uint64_t)m_Address);
}

void Device::match(NimBLEAdvertisedDevice *pDevice, std::vector<Furble::Device *> &list) {
  if (FujifilmXT30::matches(pDevice)) {
    list.push_back(new Furble::FujifilmXT30(pDevice));
  } else if (CanonEOSM6::matches(pDevice)) {
    list.push_back(new Furble::CanonEOSM6(pDevice));
  } else if (CanonEOSRP::matches(pDevice)) {
    list.push_back(new Furble::CanonEOSRP(pDevice));
  }
}

/**
 * Generate a 32-bit PRNG.
 */
static uint32_t xorshift(uint32_t x) {
  /* Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs" */
  x ^= x << 13;
  x ^= x << 17;
  x ^= x << 5;
  return x;
}

void Device::getUUID128(uuid128_t *uuid) {
  uint32_t chip_id = (uint32_t)ESP.getEfuseMac();
  for (size_t i = 0; i < UUID128_AS_32_LEN; i ++) {
    chip_id = xorshift(chip_id);
    uuid->uint32[i] = chip_id;
  }
}

}
