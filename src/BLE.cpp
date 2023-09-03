#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#include "BLE.h"

// BLE setup

// Nearby Interaction Service
#ifndef BLE_SERVICE
#define BLE_SERVICE
#define NI_SERVICE_UUID "48fe3e40-0817-4bb2-8633-3073689c2dba"
// Accessory Configuration Data
#define NI_CHARACTERISTIC_UUID "95e8d9d5-d8ef-4721-9a4e-807375f53328"

// Fake service UUIDs
// #define NI_SERVICE_UUID        "4fdfd65f-6312-48e4-9fe9-64940e48589d"
// // Accessory Configuration Data
// #define NI_CHARACTERISTIC_UUID "193ec1cb-fc35-422d-997b-b1377b8418c9"
#endif // BLE_SERVICE

BLE::BLE()
{
    Serial.println("BLE constructor");
};

void BLE::initialSetup()
{
    Serial.println("BLE initialSetup");
    BLEDevice::init("ESP32");
    BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);
    // BLEDevice::setSecurityCallbacks(new MySecurity());
    pServer = BLEDevice::createServer();
    // BLESecurity *pSecurity = new BLESecurity();
    // pSecurity->setStaticPIN(123456);
    pServer->setCallbacks(new ServerCallbacks(this));

    // deviceInfoService = pServer->createService(ServiceUUIDs::DeviceInformationUUID::Service);
    // BLECharacteristic *characteristic = deviceInfoService->createCharacteristic
    // (
    //     ServiceUUIDs::DeviceInformationUUID::Manufacturer,
    //     BLECharacteristic::PROPERTY_READ
    // );
    // characteristic->setValue("ESP32");
    // characteristic = deviceInfoService->createCharacteristic
    // (
    //     ServiceUUIDs::DeviceInformationUUID::Name,
    //     BLECharacteristic::PROPERTY_READ
    // );
    // characteristic->setValue("ESP32");
    // characteristic = deviceInfoService->createCharacteristic
    // (
    //     ServiceUUIDs::DeviceInformationUUID::SerialNumber,
    //     BLECharacteristic::PROPERTY_READ
    // );
    // String chipId = String((uint32_t)(ESP.getEfuseMac() >> 24), HEX);
    // characteristic->setValue(chipId.c_str());

    // BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    // pAdvertising->addServiceUUID(deviceInfoService->getUUID());

    // pServer->startAdvertising();
    // BLEDevice::startAdvertising();
    Serial.println("BLE initialSetup end");
};

void BLE::setupTransferService()
{
    Serial.println("BLE setupTransferService");

    transferService = pServer->createService(ServiceUUIDs::QorvoNIService::NIServiceUUID);
    BLECharacteristic *pRxCharacteristic = transferService->createCharacteristic(
        ServiceUUIDs::QorvoNIService::rxCharacteristicUUID,
        BLECharacteristic::PROPERTY_WRITE);
    pRxCharacteristic->setAccessPermissions(ESP_GATT_PERM_WRITE_ENCRYPTED);
    pRxCharacteristic->setCallbacks(new RxCallbacks(this));

    BLECharacteristic *pTxCharacteristic = transferService->createCharacteristic(
        ServiceUUIDs::QorvoNIService::txCharacteristicUUID,
        BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ);
    pTxCharacteristic->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED);
    pTxCharacteristic->setCallbacks(new TxCallbacks(this));
    BLE2902 *p2902TxDescriptor = new BLE2902();
    p2902TxDescriptor->setNotifications(true);
    // p2902TxDescriptor->setAccessPermissions(ESP_GATT_PERM_WRITE_ENCRYPTED);
    pTxCharacteristic->addDescriptor(p2902TxDescriptor);

    BLECharacteristic *pscCharacteristic = transferService->createCharacteristic(
        ServiceUUIDs::QorvoNIService::scCharacteristicUUID,
        BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ);
    pscCharacteristic->setCallbacks(new TxCallbacks(this));

    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(transferService->getUUID());

    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06); // functions that help with iPhone connections issue
    pAdvertising->setMinPreferred(0x12);
    transferService->start();
    Serial.println("BLE setupTransferService end");
};

void BLE::setupNI(const dwt_config_t *UWBconfig)
{
    Serial.println("BLE setupNI");
    setupNearbyInteractionService();
    setupAccessoryConfigurationCharacteristic(UWBconfig);

    NIService->start();

    // pServer->startAdvertising();
};

void BLE::setupNearbyInteractionService()
{
    Serial.println("BLE setupNearbyInteractionService");
    NIService = pServer->createService(ServiceUUIDs::NearbyInteractionService::NIServiceUUID);
};

void BLE::setUWB(NI_UWB *uwb)
{
    this->niUWB = uwb;
};

void BLE::setupAccessoryConfigurationCharacteristic(const dwt_config_t *UWBconfig){
    // Serial.println("BLE setupAccessoryConfigurationCharacteristic");
    // AccessoryConfigurationData configData;

    // BLECharacteristic *pCharacteristic = NIService->createCharacteristic
    // (
    //     ServiceUUIDs::NearbyInteractionService::NICharacteristicUUID,
    //     BLECharacteristic::PROPERTY_READ
    // );
    // pCharacteristic->setValue("Hello World!");
    // pCharacteristic->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED);
    // // TODO: Check I can use this to enable notifications
    // // BLE2902* p2902Descriptor = new BLE2902();
    // // p2902Descriptor->setNotifications(true);
    // // p2902Descriptor->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED);
    // // pCharacteristic->addDescriptor(p2902Descriptor);
    // setupAccessoryConfigurationData(&configData, UWBconfig);
    // pCharacteristic->setValue((uint8_t*)&configData, sizeof(configData));

    // BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    // pAdvertising->addServiceUUID(NIService->getUUID());
};

void BLE::setupAccessoryConfigurationData(AccessoryConfigurationData *configData, const dwt_config_t *UWBconfig)
{
    // UNUSED CURRENTLY
    Serial.println("BLE setupAccessoryConfigurationData");

    configData->majorVersion = NI_ACCESSORY_PROTOCOL_SPEC_MAJOR_VERSION;
    configData->minorVersion = NI_ACCESSORY_PROTOCOL_SPEC_MINOR_VERSION;
    configData->preferredUpdateRate = PreferredUpdateRate_Automatic;
    memset(configData->uwbConfigData, 0, MAX_UWB_CONFIG_SIZE);
    memcpy(&configData->uwbConfigData, UWBconfig, sizeof(dwt_config_t));
    configData->uwbConfigDataLength = MAX_UWB_CONFIG_SIZE;
};

void BLE::setupAccessoryConfigurationData(AccessoryConfigurationData *configData, uint8_t *UWBconfig)
{
    Serial.println("BLE setupAccessoryConfigurationData");

    configData->majorVersion = NI_ACCESSORY_PROTOCOL_SPEC_MAJOR_VERSION;
    configData->minorVersion = NI_ACCESSORY_PROTOCOL_SPEC_MINOR_VERSION;
    configData->preferredUpdateRate = PreferredUpdateRate_Automatic;
    memset(configData->uwbConfigData, 0, MAX_UWB_CONFIG_SIZE);
    memcpy(&configData->uwbConfigData, UWBconfig, 21);
    configData->uwbConfigDataLength = 21;
};

void BLE::informConnected()
{
    Serial.println("BLE informConnected");
    connected = true;
};

void BLE::informDisconnected()
{
    Serial.println("BLE informDisconnected");
    connected = false;
    currentlyAdvertising = false;
};

bool BLE::isConnected()
{
    return connected;
};

void BLE::startAdvertising()
{
    Serial.println("BLE startAdvertising");
    pServer->startAdvertising();
    currentlyAdvertising = true;
};

bool BLE::isAdverstising()
{
    return currentlyAdvertising;
};

void BLE::handleRx(BLECharacteristic *pCharacteristic)
{
    Serial.println("BLE handleRx");
    if (pCharacteristic->getValue().length() < 1)
    {
        Serial.println("BLE handleRx: No value");
        return;
    }
    BLEMessageIds::MessageId messageId = (BLEMessageIds::MessageId)pCharacteristic->getData()[0];
    Serial.printf("BLE handleRx: Message id: %d\n", messageId);
    switch (messageId)
    {
    case BLEMessageIds::MessageId::initiateUWB:
        Serial.println("BLE handleRx: initiateUWB");
        try
        {
            handleInitiateUWB();
            Serial.println("BLE handleRx: initiateUWB: Success");
        }
        catch (const std::exception &e)
        {
            Serial.printf("BLE handleRx: initiateUWB: Exception: %s\n", e.what());
        }
        break;
    case BLEMessageIds::MessageId::configureAndStart:
        Serial.println("BLE handleRx: configureAndStart");
        try
        {
            handleConfigureAndStart(pCharacteristic->getData() + 1, pCharacteristic->getValue().length() - 1);
            Serial.println("BLE handleRx: configureAndStart: Success");
            sendAccessoryUWBDidStart();
        }
        catch (const std::exception &e)
        {
            Serial.printf("BLE handleRx: configureAndStart: Exception: %s\n", e.what());
        }
        break;
    case BLEMessageIds::MessageId::stopUWB:
        Serial.println("BLE handleRx: stopUWB");
        try {
            handleUWBshouldStop();
        } catch (const std::exception& e) {
            Serial.printf("BLE handleRx: stopUWB: Exception: %s\n", e.what());
        }
        break;
    default:
        Serial.printf("BLE handleRx: Unknown message id: %d\n", messageId);
        break;
    }
    Serial.println("BLE handleRx end");
};

void BLE::handleUWBshouldStop()
{
    Serial.println("BLE handleUWBshouldStop");
    niUWB->stop();
};

void BLE::handleConfigureAndStart(uint8_t *configData, size_t configDataLength)
{
    Serial.println("BLE handleConfigureAndStart");
    niUWB->handleUWBConfigData(configData, configDataLength);
    niUWB->configure();
    niUWB->start();
}

/*
Sent:             Received:
1: 0x01         0: 0x01 -> Has to be 1 (presumed Major Version)
2: 0x01         1: 0x00
3: 0x00         2: 0x01  -> Has to be 1 (presumed minor version)
4: 0x00         3: 0x00
5: 0x00         4: 0x19 (dec 25) DNC
6: 0x00         5: 0x45 (dec 69) DNC
7: 0x00         6: 0x55 (dec 85) DNC
8: 0x00         7: 0xa8 -> Session ID (uint32_t) DC
9: 0x00         8: 0x5f ^^ DC
10: 0x00        9: 0x00 ^^? DNC
11: 0x00        10: 0x00 ^^? DNC
12: 0x00        11: 0x0b -> Preamble Code DNC 
13: 0x00        12: 0x09 -> Channel Number DNC
14: 0x00        13: 0x06 DNC
15: 0x00        14: 0x00 DNC
16: 0x00        15: 0x10 -> Slot_Duration_RSTU DNC
17: 0x15        16: 0x0e (dec 14) DNC
18: 0x01        17: 0xc6 -> Block Duration ms DNC
19: 0x00        18: 0x00 DNC
20: 0x01        19: 0x03 -> ToF_Report = (count_leading_zeroes([19] - 3) >> 5) DNC
21: 0x00        20: 0x39 -> Static STS_IV[0] DC
22: 0x3f        21: 0x7b -> Static STS_IV[1] DC
23: 0xf5        22: 0x28 -> Static STS_IV[2] DC
24: 0x03        23: 0xd4 -> Static STS_IV[3] DC
25: 0x00        24: 0xb8 -> Static STS_IV[4] DC
26: 0xb8        25: 0xd7 -> Static STS_IV[5] DC 
27: 0x0b        26: 0x87 -> dst_addr -> DST_ADDR[0] DC
28: 0x00        27: 0xb4 -> dst_addr >> 8 -> DST_ADDR[1] DC
29: 0x00        28: 0x64 (uint16_t) -> Block_Timing_Stability DNC
30: 0x00        29: 0x00, ^^ DNC
31: 0x00
32: 0x01
33: 0x01
34: 0x01
35: 0x12
36: 0x00
37: 0x19
38: 0x00

*/

void BLE::sendAccessoryUWBDidStart()
{
    Serial.println("BLE sendUWBdidStart");
    if (transferService == NULL)
    {
        Serial.println("BLE handleTx: transferService is NULL");
        return;
    }
    if (transferService->getCharacteristic(ServiceUUIDs::QorvoNIService::txCharacteristicUUID) == NULL)
    {
        Serial.println("BLE handleTx: scCharacteristic is NULL");
        return;
    }
    uint8_t data = BLEMessageIds::MessageId::accessoryUWBDidStart;
    Serial.printf("BLE handleTx: Message id: %d\n", BLEMessageIds::MessageId::accessoryUWBDidStart);
    transferService->getCharacteristic(ServiceUUIDs::QorvoNIService::txCharacteristicUUID)->setValue(&data, 1);
    transferService->getCharacteristic(ServiceUUIDs::QorvoNIService::txCharacteristicUUID)->notify();
    Serial.println("BLE sendUWBdidStart end");
}

void BLE::respondToPhone(BLEMessageIds::MessageId messageId)
{
    Serial.println("BLE respondToPhone");
    if (transferService == NULL)
    {
        Serial.println("BLE handleTx: transferService is NULL");
        return;
    }
    if (transferService->getCharacteristic(ServiceUUIDs::QorvoNIService::txCharacteristicUUID) == NULL)
    {
        Serial.println("BLE handleTx: scCharacteristic is NULL");
        return;
    }
    int data = messageId;
    Serial.printf("BLE handleTx: Message id: %d\n", messageId);
    transferService->getCharacteristic(ServiceUUIDs::QorvoNIService::txCharacteristicUUID)->setValue(data);
    transferService->getCharacteristic(ServiceUUIDs::QorvoNIService::txCharacteristicUUID)->notify();
    Serial.println("BLE respondToPhone end");
};

void BLE::sendAccessorryConfigData(BLEMessageIds::MessageId messageId, AccessoryConfigurationData *data)
{
    Serial.println("BLE sendAccessorryConfigData");
    if (transferService == NULL)
    {
        Serial.println("BLE sendAccessorryConfigData: transferService is NULL");
        return;
    }
    if (transferService->getCharacteristic(ServiceUUIDs::QorvoNIService::txCharacteristicUUID) == NULL)
    {
        Serial.println("BLE sendAccessorryConfigData: scCharacteristic is NULL");
        return;
    }
    Serial.printf("BLE sendAccessorryConfigData: Message id: %d\n", messageId);
    Serial.printf("BLE sendAccessorryConfigData: Data: %d\n", data->majorVersion);
    Serial.printf("BLE sendAccessorryConfigData: Data: %d\n", data->minorVersion);
    Serial.printf("BLE sendAccessorryConfigData: Data: %d\n", data->preferredUpdateRate);
    Serial.printf("BLE sendAccessorryConfigData: Data: %d\n", data->uwbConfigDataLength);
    Serial.println("BLE sendAccessorryConfigData: Data: ");
    for (int i = 0; i < data->uwbConfigDataLength; i++)
    {
        Serial.printf("%02hhX ", data->uwbConfigData[i]);
    }
    Serial.println("BLE sendAccessorryConfigData: Data end ");

    u_int8_t *messageData = (u_int8_t *)malloc(sizeof(struct AccessoryConfigurationData) + 1);
    memset(messageData, 0, sizeof(struct AccessoryConfigurationData) + 1);
    messageData[0] = messageId;
    memcpy(messageData + 1, data, sizeof(struct AccessoryConfigurationData) + 1);
    transferService->getCharacteristic(ServiceUUIDs::QorvoNIService::txCharacteristicUUID)->setValue(messageData, data->uwbConfigDataLength + ACCESSORY_CONFIGURATION_DATA_FIX_LEN + 1);
    Serial.println("BLE respondToPhone: notify");
    transferService->getCharacteristic(ServiceUUIDs::QorvoNIService::txCharacteristicUUID)->notify();
    free(messageData);
    Serial.println("BLE respondToPhone end");
}

void BLE::handleInitiateUWB()
{
    Serial.println("BLE handleInitiateUWB");
    if (niUWB == NULL)
    {
        Serial.println("BLE handleInitiateUWB: niUWB is NULL");
        return;
    }
    // niUWB->configure();

    AccessoryConfigurationData *configData = new AccessoryConfigurationData();
    uint8_t *UWBconfig = new uint8_t[21];
    uint8_t dataLen = 0;
    niUWB->populateUWBConfigData(UWBconfig, &dataLen);
    // memcpy(UWBconfig, niUWB->getConfig(), sizeof(dwt_config_t));
    setupAccessoryConfigurationData(configData, UWBconfig);
    Serial.println("BLE handleInitiateUWB: Sending accessoryConfigurationData");
    sendAccessorryConfigData(BLEMessageIds::MessageId::accessoryConfigurationData, configData);
    delay(10);
    delete configData;
    delete[] UWBconfig;
    Serial.println("BLE handleInitiateUWB: Sent accessoryConfigurationData");
};

void ServerCallbacks::onConnect(BLEServer *pServer)
{
    Serial.println("BLE onConnect");
    if (ble != NULL)
    {
        ble->informConnected();
    }
};

void ServerCallbacks::onDisconnect(BLEServer *pServer)
{
    Serial.println("BLE onDisconnect");
    if (ble != NULL)
    {
        ble->informDisconnected();
        /// Immediately start advertising again
        ble->startAdvertising();
    }
};

ServerCallbacks::ServerCallbacks(BLE *ble)
{
    this->ble = ble;
};

RxCallbacks::RxCallbacks(BLE *ble)
{
    this->ble = ble;
};

void RxCallbacks::onWrite(BLECharacteristic *pCharacteristic)
{
    Serial.println("BLE RXCallbacks onWrite");
    ble->handleRx(pCharacteristic);
};

TxCallbacks::TxCallbacks(BLE *ble)
{
    this->ble = ble;
};

void TxCallbacks::onRead(BLECharacteristic *pCharacteristic)
{
    Serial.println("BLE TxCallbacks onRead");
    String uuid = String(pCharacteristic->getUUID().toString().c_str());
    uuid.toUpperCase();

    if (uuid == String(ServiceUUIDs::QorvoNIService::txCharacteristicUUID))
    {
        Serial.println("BLE TxCallbacks onRead: txCharacteristicUUID");
    }
    else if (uuid == String(ServiceUUIDs::QorvoNIService::rxCharacteristicUUID))
    {
        Serial.println("BLE TxCallbacks onRead: rxCharacteristicUUID");
    }
    else if (uuid == String(ServiceUUIDs::QorvoNIService::scCharacteristicUUID))
    {
        Serial.println("BLE TxCallbacks onRead: sccharacteristic");
    }
    else
    {
        Serial.printf("BLE TxCallbacks onRead: Unknown characteristic %s\n", uuid.c_str());
    }
    Serial.println("BLE TxCallbacks onRead end");
};
