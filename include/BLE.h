#ifndef BLUETOOTH_H_
#define BLUETOOTH_H_

#include <BLEServer.h>
#include <BLESecurity.h>
#include "dw3000.h"
#include "NI_UWB.h"

#define NI_ACCESSORY_PROTOCOL_SPEC_MAJOR_VERSION 1
#define NI_ACCESSORY_PROTOCOL_SPEC_MINOR_VERSION 0

#define MAX_UWB_CONFIG_SIZE (64)
#define ACCESSORY_CONFIGURATION_DATA_FIX_LEN (16)

#ifndef SERVICE_UUID_NAMESPACE
#define SERVICE_UUID_NAMESPACE
namespace ServiceUUIDs {
    namespace QorvoNIService {
        constexpr char NIServiceUUID[] = "2E938FD0-6A61-11ED-A1EB-0242AC120002";
        constexpr char scCharacteristicUUID[] = "2E93941C-6A61-11ED-A1EB-0242AC120002";
        constexpr char rxCharacteristicUUID[] = "2E93998A-6A61-11ED-A1EB-0242AC120002";
        constexpr char txCharacteristicUUID[] = "2E939AF2-6A61-11ED-A1EB-0242AC120002";
    };
    namespace TransferService {
        constexpr char transferServiceUUID[] = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
        constexpr char rxCharacteristicUUID[] = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";
        constexpr char txCharacteristicUUID[] = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";
    };
    namespace NearbyInteractionService {
        constexpr char NIServiceUUID[] = "48fe3e40-0817-4bb2-8633-3073689c2dba";
        constexpr char NICharacteristicUUID[] = "95e8d9d5-d8ef-4721-9a4e-807375f53328";
    };
    namespace DeviceInformationUUID {
        constexpr char Service[] = "180a";
        constexpr char Manufacturer[] = "2a29";
        constexpr char Name[] = "2a24";
        constexpr char SerialNumber[] = "2a25";
    };
};
#endif // SERVICE_UUID_NAMESPACE

#ifndef BLE_MESSAGE_IDS
#define BLE_MESSAGE_IDS
namespace BLEMessageIds {
    enum MessageId {
        // Messages from the accessory to the phone
        accessoryConfigurationData = 0x01,
        accessoryUWBDidStart = 0x02,
        accessorryUWBDidStop = 0x03,

        // Messages from the phone to the accessory
        initiateUWB = 0xA,
        configureAndStart = 0xB,
        stopUWB = 0xC
    };
};
#endif // BLE_MESSAGE_IDS

///  Section 3.4.3.1 in Nearby Interaction Accessory Protocol Specification, Developer Preview.
enum PreferredUpdateRate {
    PreferredUpdateRate_Automatic = 0, // iOS will choose a value
    PreferredUpdateRate_Infrequent = 10, // ~1.3Hz
    PreferredUpdateRate_UserInteractive = 20, // ~5.5Hz
};


struct AccessoryConfigurationData {
    uint16_t            majorVersion; // NI Accessory Protocol major version
    uint16_t            minorVersion; // NI Accessory Protocol minor version
    uint8_t             preferredUpdateRate;
    uint8_t             rfu[10];
    uint8_t             uwbConfigDataLength;
    // ACCESSORY_CONFIGURATION_DATA_FIX_LEN
    uint8_t             uwbConfigData[MAX_UWB_CONFIG_SIZE];
} __attribute__((packed));

class BLE 
{
    protected:
        BLEServer *pServer;
        BLEService *NIService;
        BLEService *transferService;
        BLEService *deviceInfoService;
        bool connected = false;
        bool currentlyAdvertising = false;

        NI_UWB *niUWB;

    public:
        BLE();
        void initialSetup();
        void setupTransferService();
        void setupNI(const dwt_config_t *UWBconfig);
        void setupNearbyInteractionService();
        void setupAccessoryConfigurationCharacteristic(const dwt_config_t *UWBconfig);
        void startAdvertising();
        void informDisconnected();
        void informConnected();
        bool isConnected();
        bool isAdverstising();
        void handleRx(BLECharacteristic *pCharacteristic);
        void respondToPhone(BLEMessageIds::MessageId messageId);
        void respondToPhone(BLEMessageIds::MessageId messageId, AccessoryConfigurationData *data, size_t size);
        void handleInitiateUWB();
        void setUWB(NI_UWB *niUWB);
        void setupAccessoryConfigurationData(AccessoryConfigurationData *accessoryConfig, const dwt_config_t *UWBconfig);
};

class ServerCallbacks : public BLEServerCallbacks
{
    private:
        BLE *ble;

    public:
        ServerCallbacks(BLE *ble);
        void onConnect(BLEServer* pServer);
        void onDisconnect(BLEServer* pServer);
};

class RxCallbacks : public BLECharacteristicCallbacks
{
    private:
        BLE *ble;

    public:
        RxCallbacks(BLE *ble);
        void onWrite(BLECharacteristic *pCharacteristic);
};

class TxCallbacks : public BLECharacteristicCallbacks
{
    private:
        BLE *ble;

    public:
        TxCallbacks(BLE *ble);
        void onRead(BLECharacteristic *pCharacteristic);
};

#endif