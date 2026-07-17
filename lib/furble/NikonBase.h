#ifndef NIKON_BASE_H
#define NIKON_BASE_H

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include <NimBLEClient.h>
#include <NimBLERemoteCharacteristic.h>
#include <NimBLERemoteService.h>
#include <NimBLEUUID.h>

#include "Camera.h"

#define NIKON_DEBUG (1)

namespace Furble {

/**
 * Abstraction base shared by the Nikon Remote and Smart variants.
 *
 * NikonBase provides the middle layer logic handling allowing Remote and
 * Smart protocol implementations to be self-contained.
 */
class NikonBase {
 public:
  /** Pairing protocol interface shared by both variants. */
  class Pairing {
   public:
    /** Identifier. */
    typedef struct __attribute__((packed)) _id_t {
      uint32_t device;  // sent in manufacturer data in reconnect
      uint32_t nonce;
    } id_t;

    /** Pairing message. */
    typedef struct __attribute__((packed)) _msg_t {
      uint8_t stage;
      union {
        struct __attribute__((packed)) {
          uint32_t timestampH;
          uint32_t timestampL;
        };
        uint64_t timestamp;
      };
      union {
        id_t id;
        char serial[8];
      };
    } msg_t;

    virtual ~Pairing() = default;

    virtual const msg_t *processMessage(const msg_t &msg) = 0;
    const msg_t *getMessage(void) const;

   protected:
    Pairing(const uint64_t timestamp, const id_t id);

    msg_t *m_Msg = nullptr;
    std::array<msg_t, 5> m_Stage;

   private:
  };

  virtual ~NikonBase() = default;

  /**
   * Connect to Nikon camera using the abstracted entry points.
   *
   * pre-subscribe, subscribe, 4-stage loop, then finalise.
   *
   * @return true iff connected successfully.
   */
  bool connect(NimBLERemoteService *pSvc);

  virtual void shutterPress(void) = 0;
  virtual void shutterRelease(void) = 0;
  virtual void focusPress(void) = 0;
  virtual void focusRelease(void) = 0;
  virtual void updateGeoData(const Camera::gps_t &gps, const Camera::timesync_t &timesync) = 0;

  /** Service UUID from advertisement data (shared). */
  static const NimBLEUUID SERVICE_UUID;

 protected:
  NikonBase(NimBLEClient *client,
            QueueHandle_t queue,
            NimBLERemoteCharacteristic *pairChr,
            std::atomic<uint8_t> *progress);

  NimBLEClient *m_Client;
  QueueHandle_t m_Queue;
  NimBLERemoteCharacteristic *m_PairChr;
  std::atomic<uint8_t> *m_Progress;
  std::unique_ptr<Pairing> m_Pairing;

  /** Pre-stage subscription (e.g. NOT1 / REMOTE_IND1). */
  virtual bool preSubscribe(NimBLERemoteService *pSvc) = 0;

  /** Post-stage finalisation. */
  virtual bool connectFinalise(void) = 0;

 private:
  /** Subscribe to the pair characteristic's stage indications. */
  bool subscribePair(void);

  /** Drive the 4-stage handshake. */
  bool handshake4Stage(void);
};

}  // namespace Furble
#endif
