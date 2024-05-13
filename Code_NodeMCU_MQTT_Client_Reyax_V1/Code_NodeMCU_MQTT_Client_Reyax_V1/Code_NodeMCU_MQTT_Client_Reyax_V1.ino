#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char* ssid = "Penelopecharmosa";
const char* password = "13275274";
const char* mqtt_server = "test.mosquitto.org";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE    (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.println("Endereco IP: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println("Mensagem recebida...");
  Serial.print("Tópico: ");
  Serial.println(topic);

  Serial.print("Payload: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Verifica se a mensagem é "ligar" ou "desligar" e controla o relé na porta D1
  if (strcmp((char*)payload, "ligar") == 0) {
    Serial.println("Ligando o relé na porta D1");
    digitalWrite(D1, LOW);  // Ligar o relé
  } else if (strcmp((char*)payload, "desligar") == 0) {
    Serial.println("Desligando o relé na porta D1");
    digitalWrite(D1, HIGH); // Desligar o relé
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Tentando conexao MQTT...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("conectado");
      client.subscribe("estevam/teste"); // Inscreva-se no tópico para receber mensagens
    } else {
      Serial.print("falhou, rc=");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 5 segundos");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(D1, OUTPUT); // Configurar o pino D1 como saída

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  Serial.println("Setup completo.");
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    ++value;
    snprintf (msg, MSG_BUFFER_SIZE, "hello world #%ld", value);
  }
}
