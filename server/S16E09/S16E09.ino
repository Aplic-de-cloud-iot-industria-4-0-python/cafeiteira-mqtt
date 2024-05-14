#include <WiFi.h>
#include <PubSubClient.h>

// Parametros de conexão
const char* ssid = "Penelopecharmosa"; // REDE
const char* password = "13275274";     // SENHA

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

MQTT ONE -> Utilizar um APP para ligar e desligar a cafeteira -> Conectado
const char *mqtt_broker = "b37.mqtt.one";  // Host do broker
const char *topic = "2bqsvw6678/";            // Tópico a ser subscrito e publicado
const char *mqtt_username = "2bqsvw6678";                 // Usuário
const char *mqtt_password = "0efiqruwxy";                 // Senha
const int mqtt_port = 1883;                     // Porta

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
}

void loop() {
  if (mqttStatus) {
    if (!client.connected()) {
      connectMQTT();
    }
    client.loop();
  }
}

bool connectMQTT() {
  byte tentativa = 0;
  while (!client.connected() && tentativa < 5) {
    if (client.connect("arduinoClient", mqtt_username, mqtt_password)) {
      Serial.println("Conexão bem-sucedida ao broker MQTT!");
      client.subscribe(topic);
      return true;
    }
    else {
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
  }
  else if (message.equals("desligar")) {
    toggleRelay(true);
  }
}

void toggleRelay(bool state) {
  relayState = state;
  digitalWrite(relayPin, state ? HIGH : LOW);
  client.publish(topic, state ? "ligado" : "desligado");
}
