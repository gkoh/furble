#include <NimBLEAddress.h>
#include <NimBLEAdvertisedDevice.h>
#include <NimBLEDevice.h>
#include <NimBLERemoteService.h>
#include <NimBLERemoteCharacteristic.h>

#include "Furble.h"

typedef struct _fujifilm_t
{
  char name[MAX_NAME];           /** Human readable device name. */
  uint64_t address;              /** Device MAC address. */
  uint8_t type;                  /** Address type. */
  uint8_t token[FUJIFILM_TOKEN_LEN]; /** Pairing token. */
} fujifilm_t;

// Camera
static const char AP_STATE_FORRESERVED = 3;
static const char AP_STATE_LAUNCHED = 1;
static const char AP_STATE_LAUNCHING = 2;
static const char AP_STATE_NOT_LAUNCHING = 0;
static const char *CH_AP_LAUNCH_REQUEST = "FB15C357-364F-49D3-B5C5-1E32C0DED038";
static const char *CH_AP_STATE = "A68E3F66-0FCC-4395-8D4C-AA980B5877FA";
static const char *CH_BLE_PROTOCOL_VERSION = "389363E4-712E-4CF2-A72E-BFCF7FB6ADC1";
static const char *CH_CARD_STATE = "34D8C8DE-E2A9-43FF-822C-7D945DD8D8E1";
static const char *CH_CONNECTED_DEVICEDISCONNECTEDREASON = "7EDE1988-B27E-43FC-80F4-6FEC994F0552";
static const char *CH_CONNECTED_DEVICENAME = "85B9163E-62D1-49FF-A6F5-054B4630D4A1";
static const char *CH_CONNECTED_DEVICERECEIVESTATE = "A80BE3F8-8BCB-4ADD-A725-170B7A53ADC9";
static const char *CH_CONNECTED_DIVICE_BLE_PROTOCOL_VERSION = "EB4166B0-9CCA-445E-A4E4-75B3817FD57A";
static const char *CH_CONNECTED_ERROR_STATE = "763514E8-C627-4B9E-B7E8-89074AD3CCC7";
static const char *CH_DATE_SYNC_STATE = "F9150137-5D40-4801-A8DC-F7FC5B01DA50";
static const char *CH_DATE_TIME = "B9BFD37F-CCAD-4D36-A1EE-018E792B3EDF";
static const char *CH_FIRMWAREVERSION = "417CF7FF-54E3-442F-B77F-76E228252211";
static const char *CH_FUNCTION_LAUNCH_REQUEST = "600655E6-3637-42F1-8FB2-44EFC5C63B13";
static const char *CH_FWUPDATE_REQUEST = "B1307521-7AC5-4199-AAEE-9D094781CE69";
static const char *CH_FWUPDATE_STATE = "049EC406-EF75-4205-A390-08FE209C51F0";
static const char *CH_LENSFIRMWAREVERSION = "621A98D8-6314-4C3E-A683-EEA1D6166606";
static const char *CH_LENSPRODUCTNAME = "68887A7A-10A7-4A4A-AADD-6F9DB1ABAA0E";
static const char *CH_LOCATIONANDSPEED = "0F36EC14-29E5-411A-A1B6-64EE8383F090";
static const char *CH_LOCATION_SYNC_STATE = "AD06C7B7-F41A-46F4-A29A-712055319122";
static const char *CH_MOVIE_BUTTON = "CAE8C738-4BB6-4605-98C1-58331C87F3B6";
static const char *CH_MOVIE_REC_REQUEST = "861442AB-B94E-4935-90D9-41E291D91374";
static const char *CH_ORG_BT_DEVICE_NAME = "00002A00-0000-1000-8000-00805F9B34FB";
static const char *CH_ORG_BT_FIRMWARE = "00002A26-0000-1000-8000-00805F9B34FB";
static const char *CH_ORG_BT_MANUFACTURE_NAME = "00002A29-0000-1000-8000-00805F9B34FB";
static const char *CH_ORG_BT_SERIALNUMBER = "00002A25-0000-1000-8000-00805F9B34FB";
static const char *CH_PAIRING_SMARTDEVICE_NUM = "8814441B-1D7B-4046-891D-D8F80864CC8E";
static const char *CH_PARINGKEY = "ABA356EB-9633-4E60-B73F-F52516DBD671";
static const char *CH_POWER_OFF_REQUEST = "43070F6C-51E0-4887-86A7-5F762BDA5791";
static const char *CH_PRODUCTNAME = "F398C909-9C5F-4865-B83A-7D0A35EACCBC";
static const char *CH_SERIALNUMBER = "D3F67AF3-161B-45EE-BD20-2AEA80DA92DE";
static const char *CH_SETTING_RESERVATION_AFTER_SHOOTING = "BD45F887-A6BE-4CB7-8565-390DF38BF5BF";
static const char *CH_SHOOTING_REQUEST = "7FCF49C6-4FF0-4777-A03D-1A79166AF7A8";
static const char *CH_SSID = "BF6DC9CF-3606-4EC9-A4C8-D77576E93EA4";
static const char *CH_STARTUP_VALUE = "4BF16F6D-7264-4E76-B0D2-52CDE16D2C7D";
static const char *CH_TRANSFER_STATE = "BD17BA04-B76B-4892-A545-B73BA1F74DAE";
static const char *CH_WAKEUPMODE = "9C72C205-5740-4F17-9949-0D3FADF2F67A";
static const char *CLIENT_CH_CONFIG = "00002902-0000-1000-8000-00805f9b34fb";
static const char MOVIE_REQUEST_START = 1;
static const char SHOOTING_REQUEST_S0 = 0;
static const char SHOOTING_REQUEST_S1 = 1;
static const char SHOOTING_REQUEST_S2 = 2;

// Service

static const int AP_PORT_NUMBER = 55740;
static const char *BT_PARAM_SCANMODE = "ScanMode";
static const int CONNECT_GATT = 6;
static const int FUNCTION_CAMERA_VIEWER = 3;
static const int FUNCTION_FIRMWARE_UPDATE = 5;
static const int FUNCTION_GPSINFO_ASSIST = 2;
static const int FUNCTION_NOT_SET = 0;
static const int FUNCTION_PHOTO_RECEIVER = 1;
static const int FUNCTION_REMOTE_VIEW = 4;
static const char *KEY_AP_STATE = "APState";
static const char *KEY_AP_STATE_LOWER = "apState";
static const char *KEY_CAMERADATA = "CameraData";
static const char *KEY_CAMERA_SERIAL_NUMBER = "CameraSerialNumber";
static const char *KEY_LOCAL_NAME_LOWER = "cameraLocalName";
static const char *KEY_PAIRING_OBJ = "BLEPairingCameraObj";
static const char *KEY_SERIAL_NUMBER = "SerialNumber";
static const char *KEY_SERIAL_NUMBER_LOWER = "serialNumber";
static const char *KEY_TARGET_CAMERA = "BLECurrentCamera";
static const char *KEY_TARGET_SERIAL = "TargetSerial";
static const char *KEY_TRANSFER_STATE = "TransferState";
static const char *KEY_TRANSFER_STATE_LOWER = "transferState";
static const int MANUFACTURE_ID = 1240;
static const int NOTIFY_APSTATE = 19;
static const int NOTIFY_BT_TURN_OFF = 26;
static const int NOTIFY_BT_TURN_ON = 25;
static const int NOTIFY_CAMERADATA = 21;
static const int NOTIFY_CANT_TRANSFER = 18;
static const int NOTIFY_CHANGED_TARGET = 23;
static const int NOTIFY_CONNECT = 10;
static const int NOTIFY_DISCONNECT = 11;
static const int NOTIFY_FWUPDATE_STATE = 22;
static const int NOTIFY_HANDOVER_FAILED = 106;
static const int NOTIFY_HANDOVER_SUCCESS = 105;
static const int NOTIFY_PAIRING_COMPLETED = 9;
static const int NOTIFY_READIMAGE = 8;
static const int NOTIFY_RECEIVE_RESERVED_PHOTORECEIVE_STATE_OFF_FROM_OTHER_CAMERA = 28;
static const int NOTIFY_RECEIVE_RESERVED_PHOTORECEIVE_STATE_ON_FROM_OTHER_CAMERA = 27;
static const int NOTIFY_RECONNECT_COMPLETED = 12;
static const int NOTIFY_RESERVED_PHOTO = 13;
static const int NOTIFY_RESERVED_PHOTORECEIVER_CLOSE = 101;
static const int NOTIFY_RESERVED_PHOTORECEIVER_FINISH = 102;
static const int NOTIFY_RESERVED_PHOTORECEIVER_OBJECT_COUNT = 104;
static const int NOTIFY_RESERVED_PHOTORECEIVER_OPEN_FAILED = 103;
static const int NOTIFY_RESERVED_PHOTORECEIVER_START = 100;
static const int NOTIFY_SDP = 7;
static const int NOTIFY_SERVICE_STATUS = 20;
static const int NOTIFY_SHOOTING_S2_COMPLETE = 24;
static const int NOTIFY_TRANSFERABLE = 17;
static const int NOTIFY_WLAN_CONNECTABLE = 16;
static const int RECIEVE_SDP_CONNECTED_CAMERA = 15;
static const int RECIEVE_SDP_DETECTED_CAMERA = 5;
static const int RECIEVE_SDP_FIND = 3;
static const int RECIEVE_SDP_PAIRING_CAMERA = 14;
static const int RECIEVE_SDP_START = 2;
static const int RECIEVE_SDP_STOP = 4;
static const int SCANMODE_PAIRING = 0;
static const int SCANMODE_RECONNECT = 1;
static const char *SERVICE_DEVICE_INFORMATION = "0000180A-0000-1000-8000-00805F9B34FB";
static const char *SERVICE_DEVICE_INFORMATION_EMU = "F24E2B80-03B5-49CD-A2F7-F633B940CEA0";
static const char *SERVICE_FF_CAMERA_CONTROL = "6514EB81-4E8F-458D-AA2A-E691336CDFAC";
static const char *SERVICE_FF_CAMERA_INFORMATION = "117C4142-EDD4-4C77-8696-DD18EEBB770A";
static const char *SERVICE_FF_CAMERA_SETTING = "4E941240-D01D-46B9-A5EA-67636806830B";
static const char *SERVICE_FF_CAMERA_STATE = "4C0020FE-F3B6-40DE-ACC9-77D129067B14";
static const char *SERVICE_FF_CONNECTED_DEVICE_INFORMATION = "91F1DE68-DFF6-466E-8B65-FF13B0F16FB8";
static const char *SERVICE_FF_CURRENT_LOCATION = "3B46EC2B-48BA-41FD-B1B8-ED860B60D22B";
static const char *SERVICE_FF_CURRENT_TIME = "E872B11F-D526-4AE1-9BB4-89A99D48FA59";
static const char *SERVICE_FF_MOUNTED_LENS_INFORMATION = "15CA59FE-620C-464D-A987-223FAB660CDE";
static const char *SERVICE_GENERIC_ACCESS = "00001800-0000-1000-8000-00805f9b34fb";
static const char *SERVICE_GENERIC_ATTRIBUTE = "00001801-0000-1000-8000-00805f9b34fb";
static const int EVENTTYPE_CONNECT_FAILED = 102;
static const int EVENTTYPE_CONNECT_SUCCESS = 101;
static const int EVENTTYPE_START_RECEIVING = 200;

static const uint8_t FUJI_SHUTTER_CMD[2] = {0x01, 0x00};
static const uint8_t FUJI_SHUTTER_PRESS[2] = {0x02, 0x00};
static const uint8_t FUJI_SHUTTER_RELEASE[2] = {0x00, 0x00};

namespace Furble
{

  static void print_token(const uint8_t *token)
  {
    Serial.println("Token = " +
                   String(token[0], HEX) +
                   String(token[1], HEX) +
                   String(token[2], HEX) +
                   String(token[3], HEX));
  }

  Fujifilm::Fujifilm(const void *data, size_t len)
  {
    if (len != sizeof(fujifilm_t))
      throw;

    const fujifilmfilm_t *fujifilm = (fujifilm_t *)data;
    m_Name = std::string(fujifilm->name);
    m_Address = NimBLEAddress(fujifilm->address, fujifilm->type);
    memcpy(m_Token, fujifilm->token, FUJIFILM_TOKEN_LEN);
  }

  Fujifilm::Fujifilm(NimBLEAdvertisedDevice *pDevice)
  {
    const char *data = pDevice->getManufacturerData().data();
    m_Name = pDevice->getName();
    m_Address = pDevice->getAddress();
    m_Token[0] = data[3];
    m_Token[1] = data[4];
    m_Token[2] = data[5];
    m_Token[3] = data[6];
    Serial.println("Name = " + String(m_Name.c_str()));
    Serial.println("Address = " + String(m_Address.toString().c_str()));
    print_token(m_Token);
  }

  Fujifilm::~Fujifilm(void)
  {
    NimBLEDevice::deleteClient(m_Client);
    m_Client = nullptr;
  }

  const size_t FUJI_ADV_TOKEN_LEN = 7;
  const uint8_t FUJI_ID_0 = 0xd8;
  const uint8_t FUJI_ID_1 = 0x04;
  const uint8_t FUJI_TYPE_TOKEN = 0x02;

  /**
 * Determine if the advertised BLE device is a Fujifilm X-T30.
 */
  bool Fujifilm::matches(NimBLEAdvertisedDevice *pDevice)
  {
    if (pDevice->haveManufacturerData() &&
        pDevice->getManufacturerData().length() == FUJI_ADV_TOKEN_LEN)
    {
      const char *data = pDevice->getManufacturerData().data();
      if (data[0] == FUJI_ID_0 &&
          data[1] == FUJI_ID_1 &&
          data[2] == FUJI_TYPE_TOKEN)
      {
        return true;
      }
    }
    return false;
  }

  /**
 * Connect to a Fujifilm X-T30.
 *
 * During initial pairing the X-T30 advertisement includes a pairing token, this
 * token is what we use to identify ourselves upfront and during subsequent
 * re-pairing.
 */
  bool Fujifilm::connect(NimBLEClient *pClient,
                         ezProgressBar &progress_bar)
  {
    m_Client = pClient;

    progress_bar.value(10.0f);

    NimBLERemoteService *pSvc = nullptr;
    NimBLERemoteCharacteristic *pChr = nullptr;

    Serial.println("Connecting");
    if (!m_Client->connect(m_Address))
      return false;

    Serial.println("Connected");
    progress_bar.value(20.0f);
    pSvc = m_Client->getService(SERVICE_FF_CONNECTED_DEVICE_INFORMATION);
    if (pSvc == nullptr)
      return false;

    Serial.println("Pairing");
    pChr = pSvc->getCharacteristic(CH_PARINGKEY);
    if (pChr == nullptr)
      return false;

    if (!pChr->canWrite())
      return false;
    print_token(m_Token);
    if (!pChr->writeValue(m_Token, FUJIFILM_TOKEN_LEN))
      return false;
    Serial.println("Paired!");
    progress_bar.value(30.0f);

    Serial.println("Identifying");
    pChr = pSvc->getCharacteristic(CH_CONNECTED_DEVICENAME);
    if (!pChr->canWrite())
      return false;
    if (!pChr->writeValue(FURBLE_STR))
      return false;
    Serial.println("Identified!");
    progress_bar.value(40.0f);

    Serial.println("Configuring");
    pSvc = m_Client->getService(SERVICE_FF_CAMERA_STATE);
    // indications
    pSvc->getCharacteristic(CH_AP_STATE)->subscribe(false);
    progress_bar.value(50.0f);
    pSvc->getCharacteristic(CH_TRANSFER_STATE)->subscribe(false);
    progress_bar.value(60.0f);
    // notifications
    pSvc->getCharacteristic(CH_DATE_SYNC_STATE)->subscribe(true);
    progress_bar.value(70.0f);
    pSvc->getCharacteristic(CH_LOCATION_SYNC_STATE)->subscribe(true);
    progress_bar.value(80.0f);
    pSvc->getCharacteristic(CH_FWUPDATE_STATE)->subscribe(true);
    progress_bar.value(90.0f);

    Serial.println("Configured");

    progress_bar.value(100.0f);

    return true;
  }

  void Fujifilm::shutterPress(void)
  {
    NimBLERemoteService *pSvc = m_Client->getService(SERVICE_FF_CAMERA_CONTROL);
    NimBLERemoteCharacteristic *pChr = pSvc->getCharacteristic(CH_SHOOTING_REQUEST);
    pChr->writeValue(&FUJI_SHUTTER_CMD[0], sizeof(FUJI_SHUTTER_CMD), true);
    pChr->writeValue(&FUJI_SHUTTER_PRESS[0], sizeof(FUJI_SHUTTER_PRESS), true);
  }

  void Fujifilm::shutterRelease(void)
  {
    NimBLERemoteService *pSvc = m_Client->getService(SERVICE_FF_CAMERA_CONTROL);
    NimBLERemoteCharacteristic *pChr = pSvc->getCharacteristic(CH_SHOOTING_REQUEST);
    pChr->writeValue(&FUJI_SHUTTER_CMD[0], sizeof(FUJI_SHUTTER_CMD), true);
    pChr->writeValue(&FUJI_SHUTTER_RELEASE[0], sizeof(FUJI_SHUTTER_RELEASE), true);
  }

  void Fujifilm::print(void)
  {
    Serial.print("Name: ");
    Serial.println(m_Name.c_str());
    Serial.print("Address: ");
    Serial.println(m_Address.toString().c_str());
    Serial.print("Type: ");
    Serial.println(m_Address.getType());
  }

  void Fujifilm::disconnect(void)
  {
    m_Client->disconnect();
  }

  device_type_t Fujifilm::getDeviceType(void)
  {
    return FURBLE_FUJIFILM;
  }

  size_t Fujifilm::getSerialisedBytes(void)
  {
    return sizeof(fujifilm_t);
  }

  bool Fujifilm::serialise(void *buffer, size_t bytes)
  {
    if (bytes != sizeof(fujifilm_t))
    {
      return false;
    }
    fujifilm_t *x = (fujifilm_t *)buffer;
    strncpy(x->name, m_Name.c_str(), 64);
    x->address = (uint64_t)m_Address;
    x->type = m_Address.getType();
    memcpy(x->token, m_Token, FUJIFILM_TOKEN_LEN);

    return true;
  }

}
