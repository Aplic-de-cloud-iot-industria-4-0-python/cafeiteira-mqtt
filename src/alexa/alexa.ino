#ifdef ENABLE_DEBUG
#define DEBUG_ESP_PORT Serial
#define NODEBUG_WEBSOCKETS
#define NDEBUG
#endif

#include <Arduino.h>
#ifdef ESP8266
#include <ESP8266WiFi.h>
#endif
#ifdef ESP32
#include <WiFi.h>
#endif

#include "SinricPro_Generic.h"
#include "SinricProTemperaturesensor.h"
#include "SinricProSwitch.h"

#include <DHT.h>;

#define WIFI_SSID "ZENAIDE.2G"
#define WIFI_PASS "32377280"

#define APP_KEY "95a76b9e-104f-4df1-893d-4367e854e948"
#define APP_SECRET "1704576a-48a1-4eda-b9e5-e6e5e69654bb-aa4363e2-22a5-46cb-9e90-b3adde88e9af"
#define TEMP_SENSOR_ID "6655d2be888aa7f7a23095cd"
#define SWITCH_ID1 "6655d250888aa7f7a230956b"

#define BAUD_RATE 9600
#define EVENT_WAIT_TIME 60000

#ifdef ESP8266
#define DHT_PIN D0
#define RELAY_PIN D1
#endif
#ifdef ESP32
#define DHT_PIN 0
#define RELAY_PIN 1
#endif
#define DHTTYPE DHT11
DHT dht(DHT_PIN, DHTTYPE);

bool deviceIsOn;
float temperature;
float humidity;
float lastTemperature;
float lastHumidity;
unsigned long lastEvent = (-EVENT_WAIT_TIME);

bool onPowerState(const String &deviceId, bool &state)
{
  Serial.printf("Temperaturesensor turned %s (via SinricPro) \r\n", state ? "on" : "off");
  deviceIsOn = state;
  digitalWrite(RELAY_PIN, !state); // Inverter o estado do relé
  return true;
}

void handleTemperaturesensor()
{
  if (deviceIsOn == false)
    return;

  unsigned long actualMillis = millis();
  if (actualMillis - lastEvent < EVENT_WAIT_TIME)
    return;

  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity))
  {
    Serial.printf("DHT reading failed!\r\n");
    return;
  }

  if (temperature == lastTemperature || humidity == lastHumidity)
    return;

  SinricProTemperaturesensor &mySensor = SinricPro[TEMP_SENSOR_ID];
  bool success = mySensor.sendTemperatureEvent(temperature, humidity);
  if (success)
  {
    Serial.printf("Temperature: %2.1f Celsius\tHumidity: %2.1f%%\r\n", temperature, humidity);
  }
  else
  {
    Serial.printf("Something went wrong...could not send Event to server!\r\n");
  }

  lastTemperature = temperature;
  lastHumidity = humidity;
  lastEvent = actualMillis;
}

void setupWiFi()
{
  Serial.printf("\r\n[Wifi]: Connecting");
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.printf(".");
    delay(250);
  }
  IPAddress localIP = WiFi.localIP();
  Serial.printf("connected!\r\n[WiFi]: IP-Address is %d.%d.%d.%d\r\n", localIP[0], localIP[1], localIP[2], localIP[3]);
}

void setupSinricPro()
{
  SinricProTemperaturesensor &mySensor = SinricPro[TEMP_SENSOR_ID];
  SinricProSwitch &mySwitch = SinricPro[SWITCH_ID1];
  mySensor.onPowerState(onPowerState);
  mySwitch.onPowerState(onPowerState);
  SinricPro.onConnected([]()
                        { Serial.printf("Connected to SinricPro\r\n"); });
  SinricPro.onDisconnected([]()
                           { Serial.printf("Disconnected from SinricPro\r\n"); });
  SinricPro.begin(APP_KEY, APP_SECRET);
}

void setup()
{
  Serial.begin(BAUD_RATE);
  Serial.printf("\r\n\r\n");
  pinMode(RELAY_PIN, OUTPUT);
  SinricProSwitch &mySwitch = SinricPro[SWITCH_ID1];
  mySwitch.onPowerState(onPowerState);
  dht.begin();

  setupWiFi();
  setupSinricPro();
}

void loop()
{
  SinricPro.handle();
  handleTemperaturesensor();
}
