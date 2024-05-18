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
}

void loop() {
  // Atualiza dados do sensor DHT e exibe no LCD
  dht_sensor_getdata();
  
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
  lcd.print("Umidade: ");
  lcd.print(hm);
  lcd.print(" %");
}
