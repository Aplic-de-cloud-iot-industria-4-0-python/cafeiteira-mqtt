#ifdef ENABLE_DEBUG // Verifica se a macro ENABLE_DEBUG está definida
#define DEBUG_ESP_PORT Serial // Define a porta serial para debug (para placas ESP)
#define NODEBUG_WEBSOCKETS // Desativa debug de WebSockets
#define NDEBUG // Desativa asserts do Arduino
#endif // Fim do bloco condicional ENABLE_DEBUG

#include <Arduino.h> // Biblioteca principal do Arduino
#ifdef ESP8266 // Verifica se a placa é ESP8266
#include <ESP8266WiFi.h> // Biblioteca WiFi para ESP8266
#endif
#ifdef ESP32  // Verifica se a placa é ESP32
#include <WiFi.h>  // Biblioteca WiFi para ESP32
#endif

#include "SinricPro_Generic.h"  // Biblioteca genérica para SinricPro
#include "SinricProTemperaturesensor.h"  // Biblioteca para sensores de temperatura SinricPro
#include "SinricProSwitch.h"  // Biblioteca para switches SinricPro
#include <DHT.h>  // Biblioteca para sensores DHT (verifique a compatibilidade)

#define WIFI_SSID "ESTACIO-VISITANTES" // Nome da rede WiFi
#define WIFI_PASS "estacio@2014" // Senha da rede WiFi

#define APP_KEY "95a76b9e-104f-4df1-893d-4367e854e948"  // Chave do aplicativo SinricPro
#define APP_SECRET "1704576a-48a1-4eda-b9e5-e6e5e69654bb-aa4363e2-22a5-46cb-9e90-b3adde88e9af"  // Segredo do aplicativo SinricPro
#define TEMP_SENSOR_ID "6655d2be888aa7f7a23095cd"  // ID do sensor de temperatura SinricPro
#define SWITCH_ID1 "6655d250888aa7f7a230956b"  // ID do primeiro switch SinricPro

#define BAUD_RATE 9600  // Taxa de comunicação serial
#define EVENT_WAIT_TIME 60000  // Tempo de espera por eventos (em milissegundos)

#ifdef ESP8266
#define DHT_PIN D0  // Pino do sensor DHT (ESP8266)
#define RELAY_PIN D1  // Pino do relé (ESP8266)
#endif
#ifdef ESP32
#define DHT_PIN 0  // Pino do sensor DHT (ESP32)
#define RELAY_PIN 4  // Pino do relé (ESP32)
#endif

#define DHTTYPE DHT11
DHT dht(DHT_PIN, DHTTYPE);

bool deviceIsOn;  // Indica se o dispositivo está ligado
float temperature;  // Valor da temperatura medida
float humidity;  // Valor da umidade medida
float lastTemperature;  // Valor anterior da temperatura
float lastHumidity;  // Valor anterior da umidade
unsigned long lastEvent = (-EVENT_WAIT_TIME);  // Timestamp do último evento enviado

bool onPowerState(const String &deviceId, bool &state) {
  // Verifica o ID do dispositivo (provavelmente para validação)
  Serial.printf("Temperaturesensor turned %s (via SinricPro) \r\n", state ? "on" : "off");

  // Atualiza o estado interno (`deviceIsOn`) baseado no comando recebido
  deviceIsOn = state;

  // Inverte o estado do relé de acordo com o comando recebido (`state`)
  digitalWrite(RELAY_PIN, !state);

  // Indica sucesso na manipulação do comando
  return true;
}

void handleTemperaturesensor() {
  // Verifica se o dispositivo está ligado
  if (deviceIsOn == false) {
    return;
  }

  // Obtém o tempo atual em milissegundos
  unsigned long actualMillis = millis();

  // Verifica se o tempo desde o último evento é suficiente para enviar um novo
  if (actualMillis - lastEvent < EVENT_WAIT_TIME) {
    return;
  }

  // Realiza a leitura da temperatura e umidade do sensor DHT
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

  // Verifica se a leitura obtida é inválida (NaN)
  if (isnan(temperature) || isnan(humidity)) {
    Serial.printf("DHT reading failed!\r\n");
    return;
  }

  // Verifica se os valores lidos mudaram desde a última leitura
  if (temperature == lastTemperature || humidity == lastHumidity) {
    return;
  }

  // Obtém a referência para o sensor de temperatura SinricPro
  SinricProTemperaturesensor &mySensor = SinricPro[TEMP_SENSOR_ID];

  // Envia o evento de temperatura para o SinricPro
  bool success = mySensor.sendTemperatureEvent(temperature, humidity);
  if (success) {
    // Imprime a temperatura e umidade lidas no monitor serial
    Serial.printf("Temperature: %2.1f Celsius\tHumidity: %2.1f%%\r\n", temperature, humidity);
  } else {
    // Imprime mensagem de erro no monitor serial caso o envio falhe
    Serial.printf("Something went wrong...could not send Event to server!\r\n");
  }

  // Atualiza as variáveis de controle para a próxima leitura
  lastTemperature = temperature;
  lastHumidity = humidity;
  lastEvent = actualMillis;
}


// ---------------------------
void setupWiFi() {
  // Imprime mensagem de início de conexão WiFi no monitor serial
  Serial.printf("\r\n[Wifi]: Connecting");

  // Tenta conectar à rede WiFi especificada por WIFI_SSID e WIFI_PASS
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  // Loop de espera pela conexão WiFi
  while (WiFi.status() != WL_CONNECTED) {
    // Imprime um ponto para indicar progresso da conexão
    Serial.printf(".");
    // Adiciona um pequeno delay para evitar sobrecarga do processador
    delay(250);
  }

  // Obtém o endereço IP local atribuído ao dispositivo
  IPAddress localIP = WiFi.localIP();

  // Imprime mensagem de sucesso da conexão e exibe o endereço IP local
  Serial.printf("connected!\r\n[WiFi]: IP-Address is %d.%d.%d.%d\r\n", localIP[0], localIP[1], localIP[2], localIP[3]);
}

void setupSinricPro() {
  // Obtém referências para os objetos sensor de temperatura e switch SinricPro
  SinricProTemperaturesensor &mySensor = SinricPro[TEMP_SENSOR_ID];
  SinricProSwitch &mySwitch = SinricPro[SWITCH_ID1];

  // Configura callbacks para eventos de liga/desliga do sensor de temperatura
  mySensor.onPowerState(onPowerState);

  // Configura callbacks para eventos de liga/desliga do primeiro switch
  mySwitch.onPowerState(onPowerState);

  // Define callback para evento de conexão com o SinricPro
  SinricPro.onConnected([]() {
    Serial.printf("Connected to SinricPro\r\n");
  });

  // Define callback para evento de desconexão do SinricPro
  SinricPro.onDisconnected([]() {
    Serial.printf("Disconnected from SinricPro\r\n");
  });

  // Inicia a comunicação com o SinricPro utilizando a chave e segredo da aplicação
  SinricPro.begin(APP_KEY, APP_SECRET);
}

void setup() {
  // Inicia a comunicação serial
  Serial.begin(BAUD_RATE);
  Serial.printf("\r\n\r\n");

  // Configura o pino do relé como saída
  pinMode(RELAY_PIN, OUTPUT);

  // Obtém a referência para o primeiro switch SinricPro
  SinricProSwitch &mySwitch = SinricPro[SWITCH_ID1];

  // Define a função de callback para eventos de alteração de estado do switch
  mySwitch.onPowerState(onPowerState);

  // Inicializa o sensor DHT
  dht.begin();

  // Chama funções para configuração do WiFi e SinricPro
  setupWiFi();
  setupSinricPro();
}

void loop() {
  // Processa os eventos do SinricPro
  SinricPro.handle();

  // Manipula o sensor de temperatura e envia eventos para o SinricPro
  handleTemperaturesensor();
}
