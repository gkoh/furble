#ifndef NIKON_REMOTE_H
#define NIKON_REMOTE_H

#include <NimBLERemoteCharacteristic.h>
#include <NimBLERemoteService.h>
#include <NimBLEUUID.h>

#include "NikonBase.h"

namespace Furble {

/**
 * Nikon remote protocol (based on ML-L7).
 */
class NikonRemote: public NikonBase {
 public:
  NikonRemote(NimBLEClient *client,
              QueueHandle_t queue,
              NimBLERemoteCharacteristic *pairChr,
              const NikonBase::Pairing::id_t &id,
              std::atomic<uint8_t> *progress);

  void shutterPress(void) override final;
  void shutterRelease(void) override final;
  void focusPress(void) override final;
  void focusRelease(void) override final;
  void updateGeoData(const Camera::gps_t &gps, const Camera::timesync_t &timesync) override final;

  /** Pair characteristic UUID. */
  static const NimBLEUUID REMOTE_PAIR_CHR_UUID;

 private:
  class RemotePairing: public NikonBase::Pairing {
   public:
    RemotePairing(const uint64_t timestamp, const NikonBase::Pairing::id_t &id);
    const msg_t *processMessage(const msg_t &msg) override final;
  };

  // Remote characteristic UUIDs (instance).
  const NimBLEUUID REMOTE_IND1_CHR_UUID {0x00002084, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};
  const NimBLEUUID REMOTE_SHUTTER_CHR_UUID {0x00002083, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};
  // Carried forward (currently unused).
  const NimBLEUUID REMOTE_R1_CHR_UUID {0x00002080, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};
  const NimBLEUUID REMOTE_W1_CHR_UUID {0x00002082, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};
  const NimBLEUUID REMOTE_R2_CHR_UUID {0x00002086, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};

  // Control modes.
  static constexpr uint8_t MODE_SHUTTER = 0x02;
  static constexpr uint8_t MODE_VIDEO = 0x03;
  static constexpr uint8_t MODE_MENU = 0x04;
  static constexpr uint8_t MODE_PLAYBACK = 0x05;

  // Control commands.
  static constexpr uint8_t CMD_PRESS = 0x02;
  static constexpr uint8_t CMD_RELEASE = 0x00;

  bool preSubscribe(NimBLERemoteService *pSvc) override final;
  bool connectFinalise(void) override final;
};

}  // namespace Furble
#endif
