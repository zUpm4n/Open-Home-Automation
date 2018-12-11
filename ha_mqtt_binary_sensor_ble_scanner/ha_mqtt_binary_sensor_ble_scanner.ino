/*
  MQTT Binary Sensor - Bluetooth LE Device Tracker - Home Assistant

  Libraries:
    - PubSubClient: https://github.com/knolleary/pubsubclient
    - ESP32 BLE:    https://github.com/nkolban/ESP32_BLE_Arduino

  Sources:
    - https://github.com/nkolban/ESP32_BLE_Arduino/blob/master/examples/BLE_scan/BLE_scan.ino
    - https://www.youtube.com/watch?v=KNoFdKgvskU

  Samuel M. - v1.0 - 01.2018
  If you like this example, please add a star! Thank you!
  https://github.com/mertenats/open-home-automation

  SDeSalve -  v1.01 - 09.12.2018
  https://github.com/sdesalve/Open-Home-Automation/
*/

typedef struct {
  String  address;
  bool    isDiscovered;
  long    lastDiscovery;
  bool    toNotify;
  char    mqttTopic[48];
} BLETrackedDevice;

#include "config.h"
#include <BLEDevice.h>
#include <WiFi.h>
#include <PubSubClient.h> // https://github.com/knolleary/pubsubclient
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"

#if defined(DEBUG_SERIAL)
#define     DEBUG_PRINT(x)    Serial.print(x)
#define     DEBUG_PRINTLN(x)  Serial.println(x)
#else
#define     DEBUG_PRINT(x)
#define     DEBUG_PRINTLN(x)
#endif

BLEScan*      pBLEScan;
WiFiClient    wifiClient;
PubSubClient  mqttClient(wifiClient);

///////////////////////////////////////////////////////////////////////////
//   BLUETOOTH
///////////////////////////////////////////////////////////////////////////
class MyAdvertisedDeviceCallbacks:
  public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      for (uint8_t i = 0; i < NB_OF_BLE_TRACKED_DEVICES; i++) {
        if (strcmp(advertisedDevice.getAddress().toString().c_str(), BLETrackedDevices[i].address.c_str()) == 0) {
          if (!BLETrackedDevices[i].isDiscovered) {
            BLETrackedDevices[i].isDiscovered = true;
            BLETrackedDevices[i].lastDiscovery = millis();
            BLETrackedDevices[i].toNotify = true;

            DEBUG_PRINT(F("INFO: Tracked device newly discovered, Address: "));
            DEBUG_PRINT(advertisedDevice.getAddress().toString().c_str());
            DEBUG_PRINT(F(", RSSI: "));
            DEBUG_PRINTLN(advertisedDevice.getRSSI());
          } else {
            BLETrackedDevices[i].lastDiscovery = millis();
            DEBUG_PRINT(F("INFO: Tracked device discovered, Address: "));
            DEBUG_PRINT(advertisedDevice.getAddress().toString().c_str());
            DEBUG_PRINT(F(", RSSI: "));
            DEBUG_PRINTLN(advertisedDevice.getRSSI());
          }
        } else {
          DEBUG_PRINT(F("INFO: Device discovered, Address: "));
          DEBUG_PRINT(advertisedDevice.getAddress().toString().c_str());
          DEBUG_PRINT(F(", RSSI: "));
          DEBUG_PRINTLN(advertisedDevice.getRSSI());
        }
      }
    }
};

///////////////////////////////////////////////////////////////////////////
//   MQTT
///////////////////////////////////////////////////////////////////////////
volatile unsigned long lastMQTTConnection = 0;
char MQTT_CLIENT_ID[7] = {0};
char MQTT_AVAILABILITY_TOPIC[sizeof(MQTT_CLIENT_ID) + sizeof(MQTT_AVAILABILITY_TOPIC_TEMPLATE) - 2] = {0};
/*
  Function called to publish to a MQTT topic with the given payload
*/
void publishToMQTT(char* p_topic, char* p_payload) {
  if (mqttClient.publish(p_topic, p_payload, true)) {
    DEBUG_PRINT(F("INFO: MQTT message published successfully, topic: "));
    DEBUG_PRINT(p_topic);
    DEBUG_PRINT(F(", payload: "));
    DEBUG_PRINTLN(p_payload);
  } else {
    DEBUG_PRINTLN(F("ERROR: MQTT message not published, either connection lost, or message too large. Topic: "));
    DEBUG_PRINT(p_topic);
    DEBUG_PRINT(F(" , payload: "));
    DEBUG_PRINTLN(p_payload);
  }
}
/*
  Function called to connect/reconnect to the MQTT broker
*/
void connectToMQTT() {
  memset(MQTT_CLIENT_ID, 0, sizeof(MQTT_CLIENT_ID));
  sprintf(MQTT_CLIENT_ID, "%06X", ESP.getEfuseMac());

  memset(MQTT_AVAILABILITY_TOPIC, 0, sizeof(MQTT_AVAILABILITY_TOPIC));
  sprintf(MQTT_AVAILABILITY_TOPIC, MQTT_AVAILABILITY_TOPIC_TEMPLATE, MQTT_CLIENT_ID);
  DEBUG_PRINT(F("INFO: MQTT availability topic: "));
  DEBUG_PRINTLN(MQTT_AVAILABILITY_TOPIC);

  if (!mqttClient.connected()) {
    if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD, MQTT_AVAILABILITY_TOPIC, 0, 1, MQTT_PAYLOAD_UNAVAILABLE)) {
      DEBUG_PRINTLN(F("INFO: The client is successfully connected to the MQTT broker"));
      publishToMQTT(MQTT_AVAILABILITY_TOPIC, MQTT_PAYLOAD_AVAILABLE);
    } else {
      DEBUG_PRINTLN(F("ERROR: The connection to the MQTT broker failed"));
      DEBUG_PRINT(F("INFO: MQTT username: "));
      DEBUG_PRINTLN(MQTT_USERNAME);
      DEBUG_PRINT(F("INFO: MQTT password: "));
      DEBUG_PRINTLN(MQTT_PASSWORD);
      DEBUG_PRINT(F("INFO: MQTT broker: "));
      DEBUG_PRINTLN(MQTT_SERVER);
    }
  } else {
    if (lastMQTTConnection < millis()) {
      lastMQTTConnection = millis() + MQTT_CONNECTION_TIMEOUT;
      publishToMQTT(MQTT_AVAILABILITY_TOPIC, MQTT_PAYLOAD_AVAILABLE);
    }
  }
}

///////////////////////////////////////////////////////////////////////////
//   SETUP() & LOOP()
///////////////////////////////////////////////////////////////////////////
void setup() {
#if defined(DEBUG_SERIAL)
  Serial.begin(115200);
#endif

  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(false);

  mqttClient.setServer(MQTT_SERVER, MQTT_SERVER_PORT);

  for (uint8_t i = 0; i < NB_OF_BLE_TRACKED_DEVICES; i++) {
    memset(MQTT_CLIENT_ID, 0, sizeof(MQTT_CLIENT_ID));
    sprintf(MQTT_CLIENT_ID, "%06X", ESP.getEfuseMac());

    char tmp_mqttTopic[sizeof(MQTT_CLIENT_ID) + sizeof(MQTT_SENSOR_TOPIC_TEMPLATE) + sizeof(LOCATION) + 12 - 4] = {0};
    char tmp_ble_address[13] = {0};
    String tmp_string_ble_address = BLETrackedDevices[i].address;
    tmp_string_ble_address.replace(":", "");
    tmp_string_ble_address.toCharArray(tmp_ble_address, sizeof(tmp_ble_address));
    sprintf(tmp_mqttTopic, MQTT_SENSOR_TOPIC_TEMPLATE, MQTT_CLIENT_ID, LOCATION, tmp_ble_address);
    memcpy(BLETrackedDevices[i].mqttTopic, tmp_mqttTopic, sizeof(tmp_mqttTopic) + 1);
    DEBUG_PRINT(F("INFO: MQTT sensor topic: "));
    DEBUG_PRINTLN(BLETrackedDevices[i].mqttTopic);
  }
}

void loop() {
  TIMERG0.wdt_wprotect = TIMG_WDT_WKEY_VALUE;
  TIMERG0.wdt_feed = 1;
  TIMERG0.wdt_wprotect = 0;
  pBLEScan->start(BLE_SCANNING_PERIOD);

  for (uint8_t i = 0; i < NB_OF_BLE_TRACKED_DEVICES; i++) {
    if (BLETrackedDevices[i].isDiscovered == true && BLETrackedDevices[i].lastDiscovery + MAX_NON_ADV_PERIOD < millis()) {
      BLETrackedDevices[i].isDiscovered = false;
      BLETrackedDevices[i].toNotify = true;
    }
  }

  if (WiFi.status() != WL_CONNECTED) {
    DEBUG_PRINT(F("INFO: WiFi connecting to: "));
    DEBUG_PRINTLN(WIFI_SSID);
    delay(10);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    randomSeed(micros());

    while (WiFi.status() != WL_CONNECTED) {
      DEBUG_PRINT(F("."));
      delay(500);
    }
    DEBUG_PRINTLN();
    DEBUG_PRINTLN(WiFi.localIP());
  }

  if (!mqttClient.connected()) {
    DEBUG_PRINT(F("INFO: Connecting to MQTT broker: "));
    DEBUG_PRINTLN(MQTT_SERVER);
    while (!mqttClient.connected()) {
      connectToMQTT();
    }
  }

  for (uint8_t i = 0; i < NB_OF_BLE_TRACKED_DEVICES; i++) {
    if (BLETrackedDevices[i].toNotify) {
      if (BLETrackedDevices[i].isDiscovered) {
        publishToMQTT(BLETrackedDevices[i].mqttTopic, MQTT_PAYLOAD_ON);
      } else {
        publishToMQTT(BLETrackedDevices[i].mqttTopic, MQTT_PAYLOAD_OFF);
      }
      BLETrackedDevices[i].toNotify = false;
    }
  }
  mqttClient.disconnect();
  //WiFi.mode(WIFI_OFF);
}
