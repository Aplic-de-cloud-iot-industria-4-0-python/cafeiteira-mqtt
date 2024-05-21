#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"
#include <WiFi.h>
#include <PubSubClient.h>

// Parametros de conexão WiFi e MQTT
const char* ssid = "Penelopecharmosa"; // REDE
const char* password = "13275274"; // SENHA

const char* mqtt_broker = "b37.mqtt.one"; // Host do broker
const char* topic = "2bqsvw6678/"; // Tópico a ser subscrito e publicado
const char* mqtt_username = "2bqsvw6678"; // Usuário
const char* mqtt_password = "0efiqruwxy"; // Senha
const int mqtt_port = 1883; // Porta

// MQTT Broker com MQTTBox e NQTTX com mosquitto -> Conectado
// const char *mqtt_broker = "test.mosquitto.org";  // Host do broker
// const char *topic = "grupo5/cafeteira";            // Tópico a ser subscrito e publicado
// const char *mqtt_username = "";                 // Usuário
// const char *mqtt_password = "";                 // Senha
// const int mqtt_port = 1883;                     // Porta

// MQTTX
// const char *mqtt_broker = "broker.emqx.io";  // Host do broker
// const char *topic = "grupo5/cafeteira";            // Tópico a ser subscrito e publicado
// const char *mqtt_username = "2bqsvw6678";                 // Usuário
// const char *mqtt_password = "0efiqruwxy";                 // Senha
// // const char *client_id = "mqttx_80be1b8f";                 // Senha
// const int mqtt_port = 8083;                     // Porta

// // iotbind -> Utilizar o app do IoTBind
// const char *mqtt_broker = "b37.mqtt.one";  // Host do broker
// const char *topic = "45eiqx7836";            // Tópico a ser subscrito e publicado
// const char *mqtt_username = "45eiqx7836";                 // Usuário
// const char *mqtt_password = "357fgiuwyz";                 // Senha
// const int mqtt_port = 1883;                     // Porta

// // myqtthub -> Utilizar o app do IoTBind
// const char *mqtt_broker = "node02.myqtthub.com";  // Host do broker
// const char *topic = "grupo5/myqtthub";            // Tópico a ser subscrito e publicado
// const char *mqtt_username = "estevam5s";                 // Usuário
// const char *mqtt_password = "PX7ppiJ7-VBtBdAfH";                 // Senha
// const char *client_id = "estevamsouzalaureth@gmail.com";                 // Senha
// const int mqtt_port = 8883;                     // Porta

// Pino do relé
const int relayPin = 52; // Use o pino correto para o relé

// DHT sensor
#define DHTTYPE DHT11 // DHT 11
#define dht_dpin 0 // Pino de dados do DHT11
DHT dht(dht_dpin, DHTTYPE); 
int LED = 7; // Ajuste o pino LED se necessário

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Sensor de água
const int waterSensorPin = A1;  // Pino analógico onde o sensor de água está conectado
int waterLevel;
const int numReadings = 10;  // Número de leituras para fazer a média

int readings[numReadings];   // Armazena as leituras analógicas
int readIndex = 0;           // Índice da leitura atual
int total = 0;               // Soma das leituras
int average = 0;             // Média das leituras

// Variáveis
bool mqttStatus = false;
bool relayState = false; // Estado inicial do relé (desligado)

// Objetos
WiFiClient wifiClient;
PubSubClient client(wifiClient);

// Protótipos
bool connectMQTT();
void callback(char* topic, byte* payload, unsigned int length);
void toggleRelay(bool state);
void dht_sensor_getdata();
void water_sensor_getdata();

void setup() {
  Serial.begin(9600);

  // Inicialização da conexão WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("Conectado à rede WiFi!");

  // Inicialização da conexão MQTT
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);
  mqttStatus = connectMQTT();

  // Configuração do pino do relé
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW); // Desliga o relé inicialmente

  // Inicializa o sensor DHT
  dht.begin();
  
  // Inicializa o display LCD
  lcd.init();
  lcd.backlight();

  // Inicializa o sensor de água
  pinMode(waterSensorPin, INPUT);
  delay(100);
  
  // Inicializa todas as leituras com 0
  for (int i = 0; i < numReadings; i++) {
    readings[i] = 0;
  }
}

void loop() {
  // Atualiza dados do sensor DHT e exibe no LCD
  dht_sensor_getdata();
  water_sensor_getdata();
  
  // Mantém a conexão MQTT
  if (mqttStatus) {
    if (!client.connected()) {
      mqttStatus = connectMQTT();
    }
    client.loop();
  }
  
  // Adiciona um pequeno delay para evitar leituras muito frequentes
  delay(2000);
}

bool connectMQTT() {
  byte tentativa = 0;
  while (!client.connected() && tentativa < 5) {
    if (client.connect("arduinoClient", mqtt_username, mqtt_password)) {
      Serial.println("Conexão bem-sucedida ao broker MQTT!");
      client.subscribe(topic);
      return true;
    } else {
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

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensagem recebida no tópico: ");
  Serial.println(topic);

  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("Mensagem: ");
  Serial.println(message);

  if (message.equals("ligar")) {
    toggleRelay(false);
  } else if (message.equals("desligar")) {
    toggleRelay(true);
  }
}

void toggleRelay(bool state) {
  relayState = state;
  digitalWrite(relayPin, state ? HIGH : LOW);
  client.publish(topic, state ? "ligado" : "desligado");
}

void dht_sensor_getdata() {
  float hm = dht.readHumidity();
  Serial.print("Umidade: ");
  Serial.println(hm);
  float temp = dht.readTemperature();
  Serial.print("Temperatura: ");
  Serial.println(temp);

  // Atualiza o LED com base na umidade e temperatura lidas
  if (temp > 30.0) {
    digitalWrite(LED, LOW); // Liga o LED se a temperatura for maior que 30 graus
  } else {
    digitalWrite(LED, HIGH); // Desliga o LED se a temperatura for menor ou igual a 30 graus
  }

  // Exibe os valores no LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(temp);
  lcd.print(" C");
  lcd.setCursor(0, 1);
  lcd.print("Umid: ");
  lcd.print(hm);
  lcd.print(" %");

  // Converte os valores para String
  String tempString = String(temp, 2);
  String humString = String(hm, 2);

  // Publica os valores no broker MQTT
  client.publish("2bqsvw6678/temperatura2505", tempString.c_str());
  client.publish("2bqsvw6678/humidade2505", humString.c_str());
}

void water_sensor_getdata() {
  // Subtrai a última leitura da soma
  total = total - readings[readIndex];
  
  // Lê a nova leitura do sensor de água
  readings[readIndex] = analogRead(waterSensorPin);
  
  // Adiciona a nova leitura à soma
  total = total + readings[readIndex];
  
  // Atualiza o índice da leitura
  readIndex = readIndex + 1;
  
  // Se estamos no final do array, volta ao início
  if (readIndex >= numReadings) {
    readIndex = 0;
  }
  
  // Calcula a média
  average = total / numReadings;

  // Envia a média das leituras para o monitor serial
  Serial.print("Nível de Água: ");
  Serial.println(average);

  // Exibe o nível de água no LCD (2ª linha)
  lcd.setCursor(0, 1);
  lcd.print("Agua: ");
  lcd.print(average);
  lcd.print(" ");
  
  // Converte o valor para String
  String waterString = String(average);

  // Publica o valor no broker MQTT
  client.publish("2bqsvw6678/nivelAgua2505", waterString.c_str());
}
