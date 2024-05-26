#include "DHT.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <hd44780.h>                        // main hd44780 header
#include <hd44780ioClass/hd44780_I2Cexp.h>  // i2c expander i/o class header

// Parametros de conexão WiFi e MQTT
const char* ssid = "Penelopecharmosa"; // REDE
const char* password = "13275274"; // SENHA

const char* mqtt_broker = "b37.mqtt.one"; // Host do broker
const char* topic = "2bqsvw6678/"; // Tópico a ser subscrito e publicado
const char* mqtt_username = "2bqsvw6678"; // Usuário
const char* mqtt_password = "0efiqruwxy"; // Senha
const int mqtt_port = 1883; // Porta

// Pinos
const int relayPin = 52; // Pino do relé
const int dhtPin = 0; // Pino de dados do DHT11
const int waterSensorPin = A0;  // Pino analógico onde o sensor de água está conectado
const int numReadings = 10;  // Número de leituras para fazer a média
const int contrastPin = A2;     // Pino do potenciômetro para controlar o contraste do LCD
const int buttonPin = 13;       // Pino do botão

// Pinos dos LEDs
const int redLedPin = 4;       // LED vermelho
const int yellowLedPin = 3;    // LED amarelo
const int greenLedPin = 2;     // LED verde

// Variáveis de Leitura de Água
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

// Variáveis de debounce
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
bool lastButtonState = HIGH;
bool buttonState = HIGH;

// Objetos
WiFiClient wifiClient;
PubSubClient client(wifiClient);
DHT dht(dhtPin, DHT11);
hd44780_I2Cexp lcd;

// Protótipos
bool connectMQTT();
void callback(char* topic, byte* payload, unsigned int length);
void toggleRelay(bool state);
void dht_sensor_getdata();
void water_sensor_getdata();
void displayCoffeeMakerState();
void displayTemperatureAndHumidity();
void displayWaterLevelFunc();
void displayHelloWorld();
void controlCoffeeMaker();

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
  
  // Inicializa o LCD
  lcd.begin(16, 2); // Definindo 16 colunas e 2 linhas

  // Inicializa o pino de controle do contraste do LCD
  pinMode(contrastPin, INPUT);

  // Inicializa o pino do botão
  pinMode(buttonPin, INPUT_PULLUP);

  // Configura os pinos dos LEDs como saída
  pinMode(redLedPin, OUTPUT);
  pinMode(yellowLedPin, OUTPUT);
  pinMode(greenLedPin, OUTPUT);

  // Inicializa o sensor de água
  pinMode(waterSensorPin, INPUT);
  delay(100);
  
  // Inicializa todas as leituras com 0
  for (int i = 0; i < numReadings; i++) {
    readings[i] = 0;
  }

  // Exibe a temperatura e a umidade no LCD
  displayTemperatureAndHumidity();
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

  // Leitura do botão com debounce
  bool reading = digitalRead(buttonPin);

  // Verifica se o botão foi pressionado
  if (reading == LOW) {
    // Inverte o estado do modo de exibição
    if (displayTempHum) {
      displayTempHum = false;
      displayWaterLevel = true;
    } else if (displayWaterLevel) {
      displayWaterLevel = false;
      coffeeMakerOn = !coffeeMakerOn; // Inverte o estado da cafeteira
      controlCoffeeMaker();
    } else {
      displayTempHum = true;
    }

    // Exibe com base no estado do modo de exibição
    if (displayTempHum) {
      displayTemperatureAndHumidity();
        // Aguarda até que o botão seja liberado
        while (digitalRead(buttonPin) == LOW) {
          delay(10);
      }
    } else if (displayWaterLevel) {
      displayWaterLevelFunc();
        // Aguarda até que o botão seja liberado
        while (digitalRead(buttonPin) == LOW) {
          delay(10);
      }
    } else {
      displayHelloWorld();
        // Aguarda até que o botão seja liberado
        while (digitalRead(buttonPin) == LOW) {
          delay(10);
      }
    }
  }

  // Mensagem de debug
  Serial.println("Botão pressionado, estado atualizado.");

  lastButtonState = reading;

  // Adiciona um pequeno delay para evitar leituras muito frequentes
  delay(500);
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
    digitalWrite(greenLedPin, LOW); // Liga o LED se a temperatura for maior que 30 graus
  } else {
    digitalWrite(greenLedPin, HIGH); // Desliga o LED se a temperatura for menor ou igual a 30 graus
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
  readings[readIndex] = analogRead(waterSensorPin);
  total += readings[readIndex];
  readIndex = (readIndex + 1) % numReadings;

  // Calcula a média das leituras
  average = total / numReadings;

  // Converte o valor da média para um valor percentual
  float waterPercentage = (average / 1023.0) * 100;

  // Envia a média das leituras para o monitor serial
  Serial.print("Nível de Água (%): ");
  Serial.println(waterPercentage);

  // Converte o valor para String
  String waterString = String(waterPercentage, 2);

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

void displayTemperatureAndHumidity() {
  // Realiza a leitura da temperatura e umidade do sensor DHT
  float temperature = dht.readTemperature(); // Lê a temperatura em Celsius
  float humidity = dht.readHumidity();       // Lê a umidade relativa do ar

  // Exibe a temperatura e umidade no LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(temperature);
  lcd.print(" C");

  lcd.setCursor(0, 1);
  lcd.print("Umidade: ");
  lcd.print(humidity);
  lcd.print(" %");

  // Exibe a temperatura e umidade no Monitor Serial para debug
  Serial.print("Temperatura: ");
  Serial.print(temperature);
  Serial.println(" C");
  Serial.print("Umidade: ");
  Serial.print(humidity);
  Serial.println(" %");
}

void displayWaterLevelFunc() {
  // Lê o valor do sensor de nível de água
  int waterLevel = analogRead(waterSensorPin);

  // Converte o valor lido em um nível de água (porcentagem)
  float waterLevelPercent = (waterLevel / 1023.0) * 100.0;

  // Exibe o nível de água no LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Nivel de Agua:");
  lcd.setCursor(0, 1);
  lcd.print(waterLevelPercent);
  lcd.print(" %");

  // Exibe o nível de água no Monitor Serial para debug
  Serial.print("Nivel de Agua: ");
  Serial.print(waterLevelPercent);
  Serial.println(" %");

  // Controle dos LEDs baseado no nível de água
  if (waterLevelPercent >= 0 && waterLevelPercent <= 25) {
    digitalWrite(redLedPin, HIGH);
    digitalWrite(yellowLedPin, LOW);
    digitalWrite(greenLedPin, LOW);
  } else if (waterLevelPercent > 25 && waterLevelPercent <= 50) {
    digitalWrite(redLedPin, LOW);
    digitalWrite(yellowLedPin, HIGH);
    digitalWrite(greenLedPin, LOW);
  } else if (waterLevelPercent > 50 && waterLevelPercent <= 100) {
    digitalWrite(redLedPin, LOW);
    digitalWrite(yellowLedPin, LOW);
    digitalWrite(greenLedPin, HIGH);
  }
}

void displayHelloWorld() {
  // Exibe "Hello World" no LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Hoje tem cafe?");

  // Apaga todos os LEDs quando exibindo "Hello World"
  digitalWrite(redLedPin, LOW);
  digitalWrite(yellowLedPin, LOW);
  digitalWrite(greenLedPin, LOW);

  // Exibe o estado da cafeteira
  displayCoffeeMakerState();
}

void controlCoffeeMaker() {
  if (coffeeMakerOn) {
    digitalWrite(relayPin, HIGH); // Liga a cafeteira
  } else {
    digitalWrite(relayPin, LOW);  // Desliga a cafeteira
  }
}
