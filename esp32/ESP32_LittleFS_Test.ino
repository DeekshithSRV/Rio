#include <WiFi.h>
#include <PubSubClient.h>
#include <Preferences.h>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// ================= DEFAULT WIFI =================
const char* DEFAULT_SSID = "ACT-ai_101807102098";
const char* DEFAULT_PASSWORD = "78216553";

// ================= MQTT =================
const char* MQTT_SERVER = "test.mosquitto.org";
const int MQTT_PORT = 1883;

// ================= BLE UUIDs =================
#define SERVICE_UUID        "12345678-1234-1234-1234-123456789abc"
#define CHARACTERISTIC_RX   "abcdefab-1234-5678-1234-abcdefabcdef"
#define CHARACTERISTIC_TX   "fedcbafe-4321-8765-4321-fedcbafedcba"

// ================= LED PINS =================
#define RED_LED      32      // FAN
#define YELLOW_LED   26      // LIGHT
#define GREEN_LED    27      // PUMP

// ================= OBJECTS =================
WiFiClient espClient;
PubSubClient mqttClient(espClient);
Preferences preferences;

BLECharacteristic *pTxCharacteristic;

// ================= DEVICE INFO =================
String deviceID;

// ================= MQTT TOPICS =================
String statusTopic;
String ackTopic;

String lightTopic;
String fanTopic;
String pumpTopic;

String wifiSSIDTopic;
String wifiPasswordTopic;
String wifiSaveTopic;

// ================= STATES =================
bool lightState = false;
bool fanState = false;
bool pumpState = false;

String ssid = "";
String password = "";

String newSSID = "";
String newPassword = "";

unsigned long lastHeartbeat = 0;

// =====================================================
// BLE MESSAGE
// =====================================================

void sendBLE(String msg)
{
  if (pTxCharacteristic != nullptr)
  {
    pTxCharacteristic->setValue(
      msg.c_str()
    );

    pTxCharacteristic->notify();
  }

  Serial.println(msg);
}

// =====================================================
// MQTT ACK
// =====================================================

void publishAck(const char* msg)
{
  if (mqttClient.connected())
  {
    mqttClient.publish(
      ackTopic.c_str(),
      msg
    );
  }
}

// =====================================================
// MQTT STATUS
// =====================================================

void publishStatus()
{
  if (mqttClient.connected())
  {
    mqttClient.publish(
      statusTopic.c_str(),
      "ONLINE",
      true
    );
  }
}

// =====================================================
// LIGHT
// =====================================================

void setLight(bool state)
{
  lightState = state;

  digitalWrite(
    YELLOW_LED,
    state
  );

  Serial.println(
    state ?
    "LIGHT ON" :
    "LIGHT OFF"
  );

  publishAck(
    state ?
    "LIGHT ON" :
    "LIGHT OFF"
  );
}

// =====================================================
// FAN
// =====================================================

void setFan(bool state)
{
  fanState = state;

  digitalWrite(
    RED_LED,
    state
  );

  Serial.println(
    state ?
    "FAN ON" :
    "FAN OFF"
  );

  publishAck(
    state ?
    "FAN ON" :
    "FAN OFF"
  );
}

// =====================================================
// PUMP
// =====================================================

void setPump(bool state)
{
  pumpState = state;

  digitalWrite(
    GREEN_LED,
    state
  );

  Serial.println(
    state ?
    "PUMP ON" :
    "PUMP OFF"
  );

  publishAck(
    state ?
    "PUMP ON" :
    "PUMP OFF"
  );
}

// =====================================================
// STATUS PRINT
// =====================================================

void printStatus()
{
  Serial.println();
  Serial.println(
    "===== STATUS ====="
  );

  Serial.print(
    "WiFi : "
  );

  Serial.println(
    WiFi.status() ==
    WL_CONNECTED ?
    "ONLINE" :
    "OFFLINE"
  );

  Serial.print(
    "MQTT : "
  );

  Serial.println(
    mqttClient.connected() ?
    "ONLINE" :
    "OFFLINE"
  );

  Serial.print(
    "Light : "
  );

  Serial.println(
    lightState ?
    "ON" :
    "OFF"
  );

  Serial.print(
    "Fan : "
  );

  Serial.println(
    fanState ?
    "ON" :
    "OFF"
  );

  Serial.print(
    "Pump : "
  );

  Serial.println(
    pumpState ?
    "ON" :
    "OFF"
  );

  Serial.println(
    "=================="
  );
}
// =====================================================
// SAVE WIFI CREDENTIALS
// =====================================================

void saveCredentials(
  String newSSID,
  String newPass)
{
  preferences.begin(
    "wifi",
    false
  );

  preferences.putString(
    "ssid",
    newSSID
  );

  preferences.putString(
    "pass",
    newPass
  );

  preferences.end();
}

// =====================================================
// CONNECT WIFI
// =====================================================

void connectWiFi()
{
  preferences.begin(
    "wifi",
    true
  );

  ssid =
    preferences.getString(
      "ssid",
      DEFAULT_SSID
    );

  password =
    preferences.getString(
      "pass",
      DEFAULT_PASSWORD
    );

  preferences.end();

  Serial.print(
    "Connecting WiFi"
  );

  WiFi.mode(WIFI_STA);

  WiFi.begin(
    ssid.c_str(),
    password.c_str()
  );

  int count = 0;

  while (
    WiFi.status() !=
    WL_CONNECTED &&
    count < 30)
  {
    delay(500);
    Serial.print(".");
    count++;
  }

  Serial.println();

  if (
    WiFi.status() ==
    WL_CONNECTED)
  {
    Serial.println(
      "WiFi Connected"
    );

    Serial.print(
      "IP : "
    );

    Serial.println(
      WiFi.localIP()
    );

    sendBLE(
      "WiFi Connected"
    );

    sendBLE(
      "IP : " +
      WiFi.localIP().toString()
    );
  }
  else
  {
    Serial.println(
      "WiFi Failed"
    );

    sendBLE(
      "WiFi Failed"
    );
  }
}

// =====================================================
// BLE CALLBACK
// =====================================================

class MyCallbacks :
  public BLECharacteristicCallbacks
{
  void onWrite(
    BLECharacteristic
    *pCharacteristic)
  {
    String cmd =
      pCharacteristic
      ->getValue()
      .c_str();

    cmd.trim();

    Serial.println(
      "BLE : " +
      cmd
    );

    // STATUS
    if (
      cmd.equalsIgnoreCase(
        "STATUS"))
    {
      if (
        WiFi.status() ==
        WL_CONNECTED)
      {
        sendBLE(
          "Connected"
        );

        sendBLE(
          "SSID : " +
          WiFi.SSID()
        );

        sendBLE(
          "IP : " +
          WiFi.localIP()
          .toString()
        );
      }
      else
      {
        sendBLE(
          "Not Connected"
        );
      }
    }

    // SETWIFI
    else if (
      cmd.startsWith(
        "SETWIFI,"))
    {
      int c1 =
        cmd.indexOf(',');

      int c2 =
        cmd.indexOf(
          ',',
          c1 + 1
        );

      if (c2 > 0)
      {
        String newSSID =
          cmd.substring(
            c1 + 1,
            c2
          );

        String newPass =
          cmd.substring(
            c2 + 1
          );

        sendBLE(
          "Saving WiFi"
        );

        saveCredentials(
          newSSID,
          newPass
        );

        ssid =
          newSSID;

        password =
          newPass;

        WiFi.disconnect(
          true
        );

        delay(1000);

        connectWiFi();
      }
      else
      {
        sendBLE(
          "Invalid Format"
        );

        sendBLE(
          "SETWIFI,SSID,PASSWORD"
        );
      }
    }
  }
};

// =====================================================
// MQTT CALLBACK
// =====================================================

void mqttCallback(
  char* topic,
  byte* payload,
  unsigned int length)
{
  String msg = "";

  for (
    unsigned int i = 0;
    i < length;
    i++)
  {
    msg +=
      (char)payload[i];
  }

  msg.trim();

  String upper =
    msg;

  upper.toUpperCase();

  String t =
    String(topic);

  Serial.println();
  Serial.print(
    "Topic : "
  );

  Serial.println(t);

  Serial.print(
    "Message : "
  );

  Serial.println(msg);

  // LIGHT
  if (t == lightTopic)
  {
    setLight(
      upper == "ON"
    );
  }

  // FAN
  else if (
    t == fanTopic)
  {
    setFan(
      upper == "ON"
    );
  }

  // PUMP
  else if (
    t == pumpTopic)
  {
    setPump(
      upper == "ON"
    );
  }

  // WIFI SSID
  else if (
    t ==
    wifiSSIDTopic)
  {
    newSSID = msg;

    Serial.println(
      "New SSID Received"
    );
  }

  // WIFI PASSWORD
  else if (
    t ==
    wifiPasswordTopic)
  {
    newPassword =
      msg;

    Serial.println(
      "New Password Received"
    );
  }

  // SAVE WIFI
  else if (
    t ==
    wifiSaveTopic)
  {
    if (
      newSSID.length() > 0 &&
      newPassword.length() > 0)
    {
      saveCredentials(
        newSSID,
        newPassword
      );

      publishAck(
        "WIFI SAVED"
      );

      Serial.println(
        "WiFi Saved"
      );

      delay(2000);

      ESP.restart();
    }
  }
}
// =====================================================
// CONNECT MQTT
// =====================================================

void connectMQTT()
{
  while (
      !mqttClient.connected() &&
      WiFi.status() == WL_CONNECTED)
  {
    Serial.println(
        "Connecting MQTT..."
    );

    if (
        mqttClient.connect(
            deviceID.c_str(),
            nullptr,
            nullptr,
            statusTopic.c_str(),
            1,
            true,
            "OFFLINE"))
    {
      Serial.println(
          "MQTT Connected"
      );

      // Subscribe Topics
      mqttClient.subscribe(
          lightTopic.c_str());

      mqttClient.subscribe(
          fanTopic.c_str());

      mqttClient.subscribe(
          pumpTopic.c_str());

      mqttClient.subscribe(
          wifiSSIDTopic.c_str());

      mqttClient.subscribe(
          wifiPasswordTopic.c_str());

      mqttClient.subscribe(
          wifiSaveTopic.c_str());

      // Auto Discovery
      mqttClient.publish(
          "home/discovery",
          deviceID.c_str(),
          true
      );

      publishStatus();

      Serial.println();
      Serial.println(
          "Subscribed Topics:"
      );

      Serial.println(lightTopic);
      Serial.println(fanTopic);
      Serial.println(pumpTopic);
      Serial.println(wifiSSIDTopic);
      Serial.println(wifiPasswordTopic);
      Serial.println(wifiSaveTopic);
    }
    else
    {
      Serial.print(
          "MQTT Failed RC="
      );

      Serial.println(
          mqttClient.state()
      );

      delay(3000);
    }
  }
}

// =====================================================
// SETUP
// =====================================================

void setup()
{
  Serial.begin(115200);

  // LEDs
  pinMode(
      RED_LED,
      OUTPUT);

  pinMode(
      YELLOW_LED,
      OUTPUT);

  pinMode(
      GREEN_LED,
      OUTPUT);

  digitalWrite(
      RED_LED,
      LOW);

  digitalWrite(
      YELLOW_LED,
      LOW);

  digitalWrite(
      GREEN_LED,
      LOW);

  // BLE Initialization
  BLEDevice::init(
      "ESP32_WIFI_SETUP");

  BLEServer *pServer =
      BLEDevice::createServer();

  BLEService *pService =
      pServer->createService(
          SERVICE_UUID);

  pTxCharacteristic =
      pService->createCharacteristic(
          CHARACTERISTIC_TX,
          BLECharacteristic::
              PROPERTY_NOTIFY);

  pTxCharacteristic
      ->addDescriptor(
          new BLE2902());

  BLECharacteristic
      *pRxCharacteristic =
          pService
              ->createCharacteristic(
                  CHARACTERISTIC_RX,
                  BLECharacteristic::
                      PROPERTY_WRITE);

  pRxCharacteristic
      ->setCallbacks(
          new MyCallbacks());

  pService->start();

  BLEAdvertising
      *pAdvertising =
          BLEDevice::getAdvertising();

  pAdvertising
      ->addServiceUUID(
          SERVICE_UUID);

  pAdvertising->start();

  Serial.println(
      "BLE Started");

  // Connect WiFi
  connectWiFi();

  // Device ID
  deviceID =
      WiFi.macAddress();

  deviceID.replace(
      ":",
      ""
  );

  // MQTT Topics
  statusTopic =
      "home/" +
      deviceID +
      "/device/status";

  ackTopic =
      "home/" +
      deviceID +
      "/ack";

  lightTopic =
      "home/" +
      deviceID +
      "/actions/light";

  fanTopic =
      "home/" +
      deviceID +
      "/actions/fan";

  pumpTopic =
      "home/" +
      deviceID +
      "/actions/pump";

  wifiSSIDTopic =
      "home/" +
      deviceID +
      "/actions/wifi/ssid";

  wifiPasswordTopic =
      "home/" +
      deviceID +
      "/actions/wifi/password";

  wifiSaveTopic =
      "home/" +
      deviceID +
      "/actions/wifi/save";

  // MQTT Setup
  mqttClient.setServer(
      MQTT_SERVER,
      MQTT_PORT);

  mqttClient.setCallback(
      mqttCallback);

  connectMQTT();

  Serial.println();
  Serial.println(
      "================================");

  Serial.print(
      "Device ID : ");

  Serial.println(
      deviceID);

  Serial.println(
      "================================");

  Serial.println(
      "Commands:"
  );

  Serial.println(
      "S -> Show Status"
  );

  Serial.println(
      "================================");
}

// =====================================================
// LOOP
// =====================================================

void loop()
{
  // Reconnect WiFi
  if (
      WiFi.status() !=
      WL_CONNECTED)
  {
    Serial.println(
        "WiFi Lost..."
    );

    connectWiFi();
  }

  // Reconnect MQTT
  if (
      WiFi.status() ==
          WL_CONNECTED &&
      !mqttClient.connected())
  {
    connectMQTT();
  }

  mqttClient.loop();

  // Heartbeat
  if (
      millis() -
          lastHeartbeat >
      5000)
  {
    publishStatus();

    lastHeartbeat =
        millis();
  }

  // Serial Commands
  if (Serial.available())
  {
    String cmd =
        Serial.readStringUntil(
            '\n');

    cmd.trim();
    cmd.toUpperCase();

    if (cmd == "S")
    {
      printStatus();
    }
  }
}
