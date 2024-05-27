#include <Wire.h>
#include <hd44780.h>                        // main hd44780 header
#include <hd44780ioClass/hd44780_I2Cexp.h>  // i2c expander i/o class header
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

#define BUTTON_PIN 13       // Pino do botão
#define DHTPIN 0 // Pino de dados do DHT11
#define DHTTYPE DHT11       // Tipo de sensor DHT (DHT11 ou DHT22)
#define WATER_SENSOR_PIN A0  // Pino analógico onde o sensor de água está conectado
#define CONTRAST_PIN A2     // Pino do potenciômetro para controlar o contraste do LCD
#define RED_LED_PIN 4       // LED vermelho
#define YELLOW_LED_PIN 3    // LED amarelo
#define GREEN_LED_PIN 2     // LED verde
#define RELAY_PIN 52 // Pino do relé

// Variáveis de Leitura de Água
const int numReadings = 10;  // Número de leituras para fazer a média
int readings[numReadings];   // Armazena as leituras analógicas
int readIndex = 0;           // Índice da leitura atual
int total = 0;               // Soma das leituras
int average = 0;             // Média das leituras

// Variáveis de Estado
bool mqttStatus = false;
bool relayState = false; // Estado inicial do relé (desligado)
bool displayTempHum = true; // Variável para alternar entre exibir temperatura e umidade e "Hello World"
bool displayWaterLevel = false; // Variável para alternar entre exibir nível de água e outras informações
bool coffeeMakerOn = false; // Variável para indicar o estado da cafeteira
bool displayHello = false;

// Variáveis de debounce
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

// Objetos
WiFiClient wifiClient;
PubSubClient client(wifiClient);
DHT dht(DHTPIN, DHTTYPE);
hd44780_I2Cexp lcd;

// Protótipos
bool connectMQTT();
void callback(char* topic, byte* payload, unsigned int length);
void toggleRelay(bool state);
void dht_sensor_getdata();
void water_sensor_getdata();
void displayCoffeeMakerState();
void displayHelloWorld();

void setup() {
  Serial.begin(9600);

  // Inicializa o sensor DHT
  dht.begin();
  
  // Inicializa o LCD
  lcd.begin(16, 2); // Definindo 16 colunas e 2 linhas

  // Inicializa o pino do botão
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Inicializa o pino de controle do contraste do LCD
  pinMode(CONTRAST_PIN, INPUT);

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
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // Desliga o relé inicialmente

  // Configura os pinos dos LEDs como saída
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);

  // Inicializa o sensor de água
  pinMode(WATER_SENSOR_PIN, INPUT);
  delay(100);
  
  // Inicializa todas as leituras com 0
  for (int i = 0; i < numReadings; i++) {
    readings[i] = 0;
  }

  // Exibe a temperatura e a umidade no LCD
  dht_sensor_getdata();
}

void loop() {
  // Atualiza dados do sensor DHT
  dht_sensor_getdata();
  water_sensor_getdata();
  
  // Mantém a conexão MQTT
  if (mqttStatus) {
    if (!client.connected()) {
      mqttStatus = connectMQTT();
    }
    client.loop();
  }

  // Verifica se o botão foi pressionado
  if (digitalRead(BUTTON_PIN) == LOW) {
    // Alterna entre os modos de exibição
    if (displayTempHum) {
      displayTempHum = false;
      displayWaterLevel = true;
      displayHello = false;
    } else if (displayWaterLevel) {
      displayWaterLevel = false;
      displayHello = true;
    } else {
      displayTempHum = true;
      displayHello = false;
    }

    // Exibe com base no estado do modo de exibição
    if (displayTempHum) {
      dht_sensor_getdata();
    } else if (displayWaterLevel) {
      water_sensor_getdata();
    } else {
      displayHelloWorld(); // Chama a função para exibir "Hello World"
    }

    // Aguarda até que o botão seja liberado
    while (digitalRead(BUTTON_PIN) == LOW) {
      delay(10);
    }
  }

  // Adiciona um pequeno delay para evitar leituras muito frequentes
  delay(1000);
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
  digitalWrite(RELAY_PIN, state ? HIGH : LOW);
  client.publish(topic, state ? "ligado" : "desligado");
}

void dht_sensor_getdata() {
  float hm = dht.readHumidity();
  Serial.print("Umidade: ");
  Serial.println(hm);
  float temp = dht.readTemperature();
  Serial.print("Temperatura: ");
  Serial.println(temp);

  // Exibe a temperatura e umidade no LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(temp);
  lcd.print(" C");

  lcd.setCursor(0, 1);
  lcd.print("Umidade: ");
  lcd.print(hm);
  lcd.print(" %");

  // Atualiza o LED com base na umidade e temperatura lidas
  if (temp > 30.0) {
    digitalWrite(GREEN_LED_PIN, LOW); // Liga o LED se a temperatura for maior que 30 graus
  } else {
    digitalWrite(GREEN_LED_PIN, HIGH); // Desliga o LED se a temperatura for menor ou igual a 30 graus
  }

  // Converte os valores para String
  String tempString = String(temp, 2);
  String humString = String(hm, 2);

  // Publica os valores no broker MQTT
  client.publish("2bqsvw6678/temperatura2505", tempString.c_str());
  client.publish("2bqsvw6678/humidade2505", humString.c_str());
}

void water_sensor_getdata() {
  // Remove a leitura mais antiga e adiciona a nova leitura
  total -= readings[readIndex];
  readings[readIndex] = analogRead(WATER_SENSOR_PIN);
  total += readings[readIndex];
  readIndex = (readIndex + 1) % numReadings;

  // Calcula a média das leituras
  average = total / numReadings;

  // Converte o valor da média para um valor percentual
  float waterPercentage = (average / 1023.0) * 100;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Nivel de Agua:");
  lcd.setCursor(0, 1);
  lcd.print(waterPercentage);
  lcd.print(" %");

  // Envia a média das leituras para o monitor serial
  Serial.print("Nível de Água (%): ");
  Serial.println(waterPercentage);

  // Converte o valor para String
  String waterString = String(waterPercentage, 2);

  // Controle dos LEDs baseado no nível de água
  if (waterPercentage >= 0 && waterPercentage <= 25) {
    digitalWrite(RED_LED_PIN, HIGH);
    digitalWrite(YELLOW_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, LOW);
  } else if (waterPercentage > 25 && waterPercentage <= 50) {
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(YELLOW_LED_PIN, HIGH);
    digitalWrite(GREEN_LED_PIN, LOW);
  } else if (waterPercentage > 50 && waterPercentage <= 100) {
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(YELLOW_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, HIGH);
  }

  // Publica o valor no broker MQTT
  client.publish("2bqsvw6678/nivelAgua2505", waterString.c_str());
}

void displayCoffeeMakerState() {
  lcd.setCursor(0, 1);
  if (coffeeMakerOn) {
    lcd.print("Cafe: Ligado ");
  } else {
    lcd.print("Cafe: Desligado");
  }

  // Exibe o estado da cafeteira no Monitor Serial para debug
  Serial.print("Cafeteira: ");
  if (coffeeMakerOn) {
    Serial.println("Ligada");
  } else {
    Serial.println("Desligada");
  }
}

void displayHelloWorld() {
  // Exibe "Hello World" no LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Hoje tem cafe?");

  // Apaga todos os LEDs quando exibindo "Hello World"
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(YELLOW_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, LOW);

  // Exibe o estado da cafeteira
  displayCoffeeMakerState();
}
