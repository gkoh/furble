#include <cstring>

#include <NimBLEDevice.h>
#include <NimBLERemoteCharacteristic.h>
#include <NimBLERemoteService.h>

#include "NikonBase.h"

namespace Furble {

const NimBLEUUID NikonBase::SERVICE_UUID {0x0000de00, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};

NikonBase::Pairing::Pairing(const uint64_t timestamp, const id_t id) {
  m_Stage[0].stage = 0x01;
  m_Stage[0].timestamp = timestamp;
  m_Stage[0].id = id;
  m_Stage[1].stage = 0x02;
  m_Stage[2].stage = 0x03;
  m_Stage[3].stage = 0x04;
  m_Stage[4].stage = 0x05;

  m_Msg = &m_Stage[0];
}

const NikonBase::Pairing::msg_t *NikonBase::Pairing::getMessage(void) const {
  return m_Msg;
}

NikonBase::NikonBase(NimBLEClient *client,
                     QueueHandle_t queue,
                     NimBLERemoteCharacteristic *pairChr,
                     std::atomic<uint8_t> *progress)
    : m_Client(client), m_Queue(queue), m_PairChr(pairChr), m_Progress(progress) {}

bool NikonBase::subscribePair(void) {
  ESP_LOGI(LOG_TAG, "Subscribing to pairing indication");
  if (!m_PairChr->subscribe(
          false,
          [this](NimBLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData,
                 size_t length, bool isNotify) {
#if NIKON_DEBUG
            ESP_LOGI(LOG_TAG, "data(stage) = %s",
                     NimBLEUtils::dataToHexString(pData, length).c_str());
#endif
            NikonBase::Pairing::msg_t msg;
            memcpy(&msg, pData, sizeof(msg));
            auto pMsg = this->m_Pairing->processMessage(msg);
            bool rc = (pMsg != nullptr);
            if (!rc) {
              ESP_LOGI(LOG_TAG, "Stage response mismatch");
            }
            xQueueSend(m_Queue, &rc, 0);
          },
          true)) {
    return false;
  }
  ESP_LOGI(LOG_TAG, "Subscribed to pairing indication!");
  return true;
}

bool NikonBase::handshake4Stage(void) {
  bool success = true;
  for (uint8_t stage = 0; stage < 4 && success; stage += 2) {
    const auto *msg = m_Pairing->getMessage();
    if (!m_PairChr->writeValue((const uint8_t *)msg, sizeof(*msg), true)) {
      return false;
    }
#if NIKON_DEBUG
    ESP_LOGI(LOG_TAG, "sent = %s",
             NimBLEUtils::dataToHexString((const uint8_t *)msg, sizeof(*msg)).c_str());
#endif

    // wait 10s for a notification
    BaseType_t timeout = xQueueReceive(m_Queue, &success, pdMS_TO_TICKS(10000));
    if (timeout == pdFALSE) {
      success = false;
      ESP_LOGI(LOG_TAG, "Timeout waiting for stage %u response.", stage);
      break;
    }

    *m_Progress += 5;
  }
  return success;
}

bool NikonBase::connect(NimBLERemoteService *pSvc) {
  if (!preSubscribe(pSvc)) {
    return false;
  }
  if (!subscribePair()) {
    return false;
  }

  if (!handshake4Stage()) {
    return false;
  }

  *m_Progress += 10;

  if (!connectFinalise()) {
    return false;
  }

  *m_Progress = 100;
  ESP_LOGI(LOG_TAG, "Done!");
  return true;
}

}  // namespace Furble
