#include <Wire.h>
#include <hd44780.h>                       // main hd44780 header
#include <hd44780ioClass/hd44780_I2Cexp.h> // i2c expander i/o class header
#include "DHT.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include "SinricPro.h"
#include "SinricProSwitch.h"
#include "SinricProTemperaturesensor.h"

// Parametros de conexão WiFi e MQTT
const char *ssid = "Penelopecharmosa";
const char *password = "13275274";
const char *mqtt_broker = "b37.mqtt.one";
const char *topic = "2bqsvw6678/";
const char *mqtt_username = "2bqsvw6678";
const char *mqtt_password = "0efiqruwxy";
const int mqtt_port = 1883;

// Parametros SinricPro
#define APP_KEY "95a76b9e-104f-4df1-893d-4367e854e948"
#define APP_SECRET "1704576a-48a1-4eda-b9e5-e6e5e69654bb-aa4363e2-22a5-46cb-9e90-b3adde88e9af"
#define SWITCH_ID_1 "6654c189888aa7f7a230371d"
#define TEMP_SENSOR_ID "6654d1ad5d818a66fab14c68"
#define RELAYPIN_1 52
#define DHT_PIN 0
#define DHT_TYPE DHT11
#define BAUD_RATE 115200
#define EVENT_WAIT_TIME 60000

// Definições de pinos
#define BUTTON_PIN 13
#define WATER_SENSOR_PIN A0
#define CONTRAST_PIN A2
#define RED_LED_PIN 4
#define YELLOW_LED_PIN 3
#define GREEN_LED_PIN 2

// Variáveis de Leitura de Água
const int numReadings = 10;
int readings[numReadings];
int readIndex = 0;
int total = 0;
int average = 0;

// Variáveis de Estado
bool mqttStatus = false;
bool relayState = false;
bool displayTempHum = true;
bool displayWaterLevel = false;
bool coffeeMakerOn = false;
bool displayHello = false;
unsigned long lastDisplayTime = 0;
const unsigned long displayInterval = 10000;
unsigned long lastEvent = (-EVENT_WAIT_TIME);

// Variáveis de debounce
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

// Variáveis SinricPro
bool deviceIsOn = true;
float temperature = 0;
float humidity = 0;
float lastTemperature = 0;
float lastHumidity = 0;

WiFiClient wifiClient;
PubSubClient client(wifiClient);
DHT dht(DHT_PIN, DHT_TYPE);
hd44780_I2Cexp lcd;

// Protótipos
bool connectMQTT();
void callback(char *topic, byte *payload, unsigned int length);
void toggleRelay(bool state);
void dht_sensor_getdata();
void water_sensor_getdata();
void displayCoffeeMakerState();
void displayHelloWorld();
void handleTemperaturesensor();
void setupWiFi();
void setupSinricPro();

// Funções SinricPro
bool onPowerState1(const String &deviceId, bool &state)
{
  Serial.print("Device 1 turned ");
  Serial.println(state ? "on" : "off");
  digitalWrite(RELAYPIN_1, state ? HIGH : LOW);
  coffeeMakerOn = state;
  displayCoffeeMakerState();
  return true;
}

bool onPowerState(const String &deviceId, bool &state)
{
  Serial.print("Temperaturesensor turned ");
  Serial.print(state ? "on" : "off");
  Serial.println(" (via SinricPro)");
  deviceIsOn = state;
  return true;
}

void setup()
{
  Serial.begin(BAUD_RATE);
  dht.begin();

  // Inicializa o LCD
  lcd.begin(16, 2);

  // Inicializa o pino do botão
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Inicializa o pino de controle do contraste do LCD
  pinMode(CONTRAST_PIN, INPUT);

  setupWiFi();
  setupSinricPro();

  // Inicialização da conexão MQTT
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);
  mqttStatus = connectMQTT();

  // Configuração do pino do relé
  pinMode(RELAYPIN_1, OUTPUT);
  digitalWrite(RELAYPIN_1, LOW);

  // Configura os pinos dos LEDs como saída
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);

  // Inicializa o sensor de água
  pinMode(WATER_SENSOR_PIN, INPUT);
  delay(100);

  // Inicializa todas as leituras com 0
  for (int i = 0; i < numReadings; i++)
  {
    readings[i] = 0;
  }

  // Exibe a temperatura e a umidade no LCD
  dht_sensor_getdata();
}

void loop()
{
  SinricPro.handle();
  handleTemperaturesensor();

  // Atualiza dados do sensor DHT
  dht_sensor_getdata();
  water_sensor_getdata();

  // Mantém a conexão MQTT
  if (mqttStatus)
  {
    if (!client.connected())
    {
      mqttStatus = connectMQTT();
    }
    client.loop();
  }

  // Verifica se o tempo desde a última atualização é maior que o intervalo definido
  unsigned long currentTime = millis();
  if (currentTime - lastDisplayTime >= displayInterval)
  {
    lastDisplayTime = currentTime;

    // Alterna entre os modos de exibição
    if (displayTempHum)
    {
      displayTempHum = false;
      displayWaterLevel = true;
      displayHello = false;
    }
    else if (displayWaterLevel)
    {
      displayWaterLevel = false;
      displayHello = true;
    }
    else
    {
      displayTempHum = true;
      displayHello = false;
    }

    // Exibe com base no estado do modo de exibição
    if (displayTempHum)
    {
      dht_sensor_getdata();
    }
    else if (displayWaterLevel)
    {
      water_sensor_getdata();
    }
    else
    {
      displayHelloWorld();
    }
  }

  // Verifica se o botão foi pressionado
  if (digitalRead(BUTTON_PIN) == LOW)
  {
    while (digitalRead(BUTTON_PIN) == LOW)
    {
      delay(10);
    }
  }

  delay(1000);
}

bool connectMQTT()
{
  byte tentativa = 0;
  while (!client.connected() && tentativa < 5)
  {
    if (client.connect("arduinoClient", mqtt_username, mqtt_password))
    {
      Serial.println("Conexão bem-sucedida ao broker MQTT!");
      client.subscribe(topic);
      client.subscribe(topic);
      return true;
    }
    else
    {
      Serial.print("Falha ao conectar: ");
      Serial.println(client.state());
      Serial.print("Tentativa: ");
      Serial.println(tentativa);
      delay(2000);
      tentativa++;
    }
  }
  Serial.println("Não foi possível conectar ao broker MQTT");
  return false;
}

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Mensagem recebida no tópico: ");
  Serial.println(topic);

  String message = "";
  for (int i = 0; i < length; i++)
  {
    message += (char)payload[i];
  }

  Serial.print("Mensagem: ");
  Serial.println(message);

  if (String(topic) == String(topic))
  {
    if (message.equals("ligado"))
    {
      coffeeMakerOn = true;
    }
    else if (message.equals("desligado"))
    {
      coffeeMakerOn = false;
    }
    displayCoffeeMakerState();
  }

  if (message.equals("ligar"))
  {
    toggleRelay(true);
  }
  else if (message.equals("desligar"))
  {
    toggleRelay(false);
  }
}

void toggleRelay(bool state)
{
  relayState = state;
  digitalWrite(RELAYPIN_1, state ? HIGH : LOW);
  client.publish(topic, state ? "ligado" : "desligado");
}

void dht_sensor_getdata()
{
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity))
  {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Serial.print("Umidade: ");
  Serial.println(humidity);
  Serial.print("Temperatura: ");
  Serial.println(temperature);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(temperature);
  lcd.print(" C");

  lcd.setCursor(0, 1);
  lcd.print("Umidade: ");
  lcd.print(humidity);
  lcd.print(" %");

  if (temperature > 30.0)
  {
    digitalWrite(GREEN_LED_PIN, LOW);
  }
  else
  {
    digitalWrite(GREEN_LED_PIN, HIGH);
  }

  String tempString = String(temperature, 2);
  String humString = String(humidity, 2);

  client.publish("2bqsvw6678/temperatura2505", tempString.c_str());
  client.publish("2bqsvw6678/humidade2505", humString.c_str());
}

void water_sensor_getdata()
{
  total -= readings[readIndex];
  readings[readIndex] = analogRead(WATER_SENSOR_PIN);
  total += readings[readIndex];
  readIndex = (readIndex + 1) % numReadings;

  average = total / numReadings;

  float waterPercentage = (average / 1023.0) * 100;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Nivel de Agua:");
  lcd.setCursor(0, 1);
  lcd.print(waterPercentage);
  lcd.print(" %");

  Serial.print("Nível de Água (%): ");
  Serial.println(waterPercentage);

  String waterString = String(waterPercentage, 2);

  if (waterPercentage >= 0 && waterPercentage <= 25)
  {
    digitalWrite(RED_LED_PIN, HIGH);
    digitalWrite(YELLOW_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, LOW);
  }
  else if (waterPercentage > 25 && waterPercentage <= 50)
  {
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(YELLOW_LED_PIN, HIGH);
    digitalWrite(GREEN_LED_PIN, LOW);
  }
  else if (waterPercentage > 50 && waterPercentage <= 100)
  {
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(YELLOW_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, HIGH);
  }

  client.publish("2bqsvw6678/nivelAgua2505", waterString.c_str());
}

void displayCoffeeMakerState()
{
  lcd.setCursor(0, 1);
  if (coffeeMakerOn)
  {
    lcd.print("Cafe: Ligado ");
  }
  else
  {
    lcd.print("Cafe: Desligado");
  }

  Serial.print("Cafeteira: ");
  if (coffeeMakerOn)
  {
    Serial.println("Ligada");
  }
  else
  {
    Serial.println("Desligada");
  }
}

void displayHelloWorld()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Hoje tem cafe?");

  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(YELLOW_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, LOW);

  displayCoffeeMakerState();
}

void handleTemperaturesensor()
{
  if (!deviceIsOn)
    return;

  unsigned long actualMillis = millis();
  if (actualMillis - lastEvent < EVENT_WAIT_TIME)
    return;

  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity))
  {
    Serial.println("DHT reading failed!");
    return;
  }

  if (temperature == lastTemperature && humidity == lastHumidity)
    return;

  SinricProTemperaturesensor &mySensor = SinricPro[TEMP_SENSOR_ID];
  bool success = mySensor.sendTemperatureEvent(temperature, humidity);
  if (success)
  {
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print(" Celsius\tHumidity: ");
    Serial.print(humidity);
    Serial.println("%");
  }
  else
  {
    Serial.println("Something went wrong...could not send Event to server!");
  }

  lastTemperature = temperature;
  lastHumidity = humidity;
  lastEvent = actualMillis;
}

void setupWiFi()
{
  Serial.print("\r\n[Wifi]: Connecting");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(250);
  }
  IPAddress localIP = WiFi.localIP();
  Serial.print("connected!\r\n[WiFi]: IP-Address is ");
  Serial.println(localIP);
}

void setupSinricPro()
{
  pinMode(RELAYPIN_1, OUTPUT);
  SinricProSwitch &mySwitch1 = SinricPro[SWITCH_ID_1];
  mySwitch1.onPowerState(onPowerState1);

  SinricProTemperaturesensor &mySensor = SinricPro[TEMP_SENSOR_ID];
  mySensor.onPowerState(onPowerState);

  SinricPro.onConnected([]()
                        { Serial.println("Connected to SinricPro"); });
  SinricPro.onDisconnected([]()
                           { Serial.println("Disconnected from SinricPro"); });

  SinricPro.begin(APP_KEY, APP_SECRET);
}
