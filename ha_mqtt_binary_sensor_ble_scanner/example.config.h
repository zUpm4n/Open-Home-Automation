///////////////////////////////////////////////////////////////////////////
//  CONFIGURATION - SOFTWARE
///////////////////////////////////////////////////////////////////////////
#define NB_OF_BLE_TRACKED_DEVICES 2
BLETrackedDevice BLETrackedDevices[NB_OF_BLE_TRACKED_DEVICES] = {
  {"e0:e1:e2:e3:eb:6b", false, 0, false, {0}},
  {"fc:58:fa:29:93:05", false, 0, false, {0}}
};

#define BLE_SCANNING_PERIOD   5
#define MAX_NON_ADV_PERIOD    10000

// Location of the BLE scanner
#define LOCATION "casa"

// Debug output
#define DEBUG_SERIAL

// Wi-Fi credentials
#define WIFI_SSID     "TP-LINK_243752"
#define WIFI_PASSWORD "ssssssssssssssssss"

// Over-the-Air update
// Not implemented yet
//#define OTA
//#define OTA_HOSTNAME  ""    // hostname esp8266-[ChipID] by default
//#define OTA_PASSWORD  ""    // no password by default
//#define OTA_PORT      8266  // port 8266 by default

// MQTT
#define MQTT_USERNAME     "mqtt"
#define MQTT_PASSWORD     "ssssssssssss"
#define MQTT_SERVER       "192.168.5.149"
#define MQTT_SERVER_PORT  1883

#define MQTT_CONNECTION_TIMEOUT 5000 // [ms]

// MQTT availability: available/unavailable
#define MQTT_AVAILABILITY_TOPIC_TEMPLATE  "%s/availability" 
// MQTT binary sensor: <CHIP_ID>/sensor/<LOCATION>/<BLE_ADDRESS>
#define MQTT_SENSOR_TOPIC_TEMPLATE        "%s/sensor/%s/%s/state"

#define MQTT_PAYLOAD_ON   "ON"
#define MQTT_PAYLOAD_OFF  "OFF"

#define MQTT_PAYLOAD_AVAILABLE    "online"
#define MQTT_PAYLOAD_UNAVAILABLE  "offline"
