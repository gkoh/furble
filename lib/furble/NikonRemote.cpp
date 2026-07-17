#include <cstring>

#include <NimBLEDevice.h>
#include <NimBLERemoteCharacteristic.h>
#include <NimBLERemoteService.h>

#include "NikonRemote.h"

namespace Furble {

const NimBLEUUID NikonRemote::REMOTE_PAIR_CHR_UUID {0x00002087, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};

NikonRemote::RemotePairing::RemotePairing(const uint64_t timestamp,
                                          const NikonBase::Pairing::id_t &id)
    : NikonBase::Pairing(timestamp, id) {
  m_Stage[1].timestamp = 0x00;
  m_Stage[1].id = {0x00, 0x00};
  m_Stage[2].timestamp = 0x00;
  m_Stage[2].id = {0x00, 0x00};
  m_Stage[3].timestamp = 0x00;
  m_Stage[3].id = {0x00, 0x00};
  m_Stage[4].timestamp = 0x00;
  m_Stage[4].id = {0x00, 0x00};
}

const NikonBase::Pairing::msg_t *NikonRemote::RemotePairing::processMessage(const msg_t &msg) {
  switch (msg.stage) {
    case 0:
      m_Msg = &m_Stage[0];
      return m_Msg;
    case 2:
    {
      const msg_t *expected = &m_Stage[1];
      if (memcmp(&msg, expected, sizeof(msg)) == 0) {
        m_Msg = &m_Stage[2];
        return m_Msg;
      }
    } break;
    case 4:
    {
      if (msg.timestamp == m_Stage[3].timestamp) {
        char serial[sizeof(msg.serial) + 1] = {0x00};
        strncpy(serial, msg.serial, sizeof(serial) - 1);

        ESP_LOGI(LOG_TAG, "Serial: %s", serial);
        m_Msg = &m_Stage[4];
        return m_Msg;
      }
    } break;
  }

  return nullptr;
}

NikonRemote::NikonRemote(NimBLEClient *client,
                         QueueHandle_t queue,
                         NimBLERemoteCharacteristic *pairChr,
                         const NikonBase::Pairing::id_t &id,
                         std::atomic<uint8_t> *progress)
    : NikonBase(client, queue, pairChr, progress) {
  m_Pairing = std::make_unique<RemotePairing>(__builtin_bswap64(0x01), id);
}

bool NikonRemote::preSubscribe(NimBLERemoteService *pSvc) {
  ESP_LOGI(LOG_TAG, "Connecting as remote, subscribing to indication 1");
  auto *pChr = pSvc->getCharacteristic(REMOTE_IND1_CHR_UUID);
  if (!pChr->subscribe(
          false,
          [this](NimBLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData,
                 size_t length, bool isNotify) {
#if NIKON_DEBUG
            ESP_LOGI(LOG_TAG, "data(ind1) = %s",
                     NimBLEUtils::dataToHexString(pData, length).c_str());
#endif
          },
          true)) {
    return false;
  }
  ESP_LOGI(LOG_TAG, "Subscribed to indication 1!");
  return true;
}

bool NikonRemote::connectFinalise(void) {
  // nothing more needed for remote mode
  return true;
}

void NikonRemote::shutterPress(void) {
  std::array<uint8_t, 2> cmd = {MODE_SHUTTER, CMD_PRESS};
  m_Client->setValue(SERVICE_UUID, REMOTE_SHUTTER_CHR_UUID, {cmd.data(), cmd.size()}, true);
}

void NikonRemote::shutterRelease(void) {
  std::array<uint8_t, 2> cmd = {MODE_SHUTTER, CMD_RELEASE};
  m_Client->setValue(SERVICE_UUID, REMOTE_SHUTTER_CHR_UUID, {cmd.data(), cmd.size()}, true);
}

void NikonRemote::focusPress(void) {
  // do nothing
}

void NikonRemote::focusRelease(void) {
  // do nothing
}

void NikonRemote::updateGeoData(const Camera::gps_t &gps, const Camera::timesync_t &timesync) {
  // Remote variant does not support geotagging.
  (void)gps;
  (void)timesync;
}

}  // namespace Furble
