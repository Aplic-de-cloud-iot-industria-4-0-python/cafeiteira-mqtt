// ** ------------ BIBLIOTECAS ------------ ** 
#include <Wire.h>                              // Biblioteca padrão do Arduino para comunicação I2C.
#include <hd44780.h>                           // Biblioteca principal para o driver do LCD HD44780.
#include <hd44780ioClass/hd44780_I2Cexp.h>     // Biblioteca específica para interagir com o LCD HD44780 utilizando um expansor I2C.
#include "DHT.h"                               // Biblioteca para sensores DHT (verifique a compatibilidade com o seu modelo).
#include <WiFi.h>                              // Biblioteca para conexão e gerenciamento de WiFi.
#include <PubSubClient.h>                      // Biblioteca para comunicação com serviços de mensagens publish/subscribe (MQTT).
// **  ------------ ------------ ------------- ** 

// ** ------------ CONSTANTES ------------ **
// Parametros de conexão WiFi e MQTT
const char *ssid = "iPhone";                   // Nome da rede Wi-Fi à qual o Arduino Giga R1 WIFI se conectará.
const char *password = "13275274";             // Senha da rede Wi-Fi.
const char *mqtt_broker = "b37.mqtt.one";      // Endereço do broker MQTT ao qual o Arduino Giga R1 WIFI se conectará.
const char *topic = "2bqsvw6678/";             // Tópico MQTT que o Arduino Giga R1 WIFI usará para publicar e se inscrever em mensagens.
const char *mqtt_username = "2bqsvw6678";      // Nome de usuário para autenticação no broker MQTT.
const char *mqtt_password = "0efiqruwxy";      // Senha para autenticação no broker MQTT.
const int mqtt_port = 1883;                    // Porta de conexão com o broker MQTT.
// **  ------------ ------------ ------------- ** 

// ** ------------ DEFINIÇÕES DE MACROS ------------ ** 
#define RELAY_PIN 52                           // Pino do relé
#define DHTPIN 0                               // Pino de dados do DHT11
#define DHTTYPE DHT11                          // Tipo de sensor DHT (DHT11 ou DHT22)
#define WATER_SENSOR_PIN A0                    // Pino analógico onde o sensor de água está conectado
#define RED_LED_PIN 4                          // LED vermelho
#define YELLOW_LED_PIN 3                       // LED amarelo
#define GREEN_LED_PIN 2                        // LED verde
// **  ------------ ------------ ------------- ** 

// ** ------------ VARIÁVEIS GLOBAIS ------------ ** 
// Variáveis de Leitura de Água
const int numReadings = 10;                    // Número de leituras para fazer a média
int readings[numReadings];                     // Armazena as leituras analógicas
int readIndex = 0;                             // Índice da leitura atual
int total = 0;                                 // Soma das leituras
int average = 0;                               // Média das leituras

// Variáveis de Estado
bool mqttStatus = false;                       // Status da conexão com o servidor MQTT
bool relayState = false;                       // Estado inicial do relé (desligado)
bool displayTempHum = true;                    // Variável para alternar entre exibir temperatura e umidade e "Hello World"
bool displayWaterLevel = false;                // Variável para alternar entre exibir nível de água e outras informações
bool coffeeMakerOn = false;                    // Variável para indicar o estado da cafeteira
bool displayHello = false;                     // Alternar entre "Hello World" e outras informações no LCD

// Variável para rastrear o tempo
unsigned long lastDisplayTime = 0;             // Tempo da última atualização da exibição
const unsigned long displayInterval = 10000;   // Intervalo de 10 segundos

// Variáveis de debounce
unsigned long lastDebounceTime = 0;            // Tempo da última leitura válida do sinal de entrada (em milissegundos)
unsigned long debounceDelay = 50;              // Tempo mínimo que o sinal de entrada precisa estar alto para ser considerado válido (em milissegundos)
// **  ------------ ------------ ------------- ** 

// ** ------------ VARIÁVEIS DE OBJETO ------------ ** 
/*
WiFiClient wifiClient;:
  Cria um objeto WiFiClient para gerenciar a conexão WiFi do dispositivo.
PubSubClient client(wifiClient);:
  Cria um objeto PubSubClient para se comunicar com um broker MQTT. O construtor recebe o objeto wifiClient como argumento,
  indicando que a conexão WiFi será utilizada para a comunicação MQTT.
DHT dht(DHTPIN, DHTTYPE);:
  Cria um objeto DHT para interagir com o sensor DHT. O construtor recebe dois argumentos:
  DHTPIN define o pino ao qual o sensor está conectado, e DHTTYPE define o tipo específico do sensor DHT utilizado.
hd44780_I2Cexp lcd;:
  Cria um objeto lcd da classe hd44780_I2Cexp. Esta classe provavelmente é utilizada para controlar um display LCD HD44780 através de um expansor I2C.
*/
// Objetos
WiFiClient wifiClient;                         // Cliente para conexão WiFi
PubSubClient client(wifiClient);               // Cliente para comunicação MQTT (usa o cliente WiFi)
DHT dht(DHTPIN, DHTTYPE);                      // Sensor DHT (pino e tipo especificados)
hd44780_I2Cexp lcd;                            // Display LCD controlado por expansor I2C
// **  ------------ ------------ ------------- ** 

// ** ------------ PROTÓTIPOS ------------ ** 
/*
bool connectMQTT();:
  Declara o protótipo da função connectMQTT(). Esta função provavelmente tenta estabelecer uma conexão com o broker MQTT
  e retorna true se a conexão for bem-sucedida e false caso contrário.
void callback(char *topic, byte *payload, unsigned int length);:
  Declara o protótipo da função callback(). Esta função é chamada sempre que o cliente MQTT recebe uma mensagem publicada em um tópico específico.
  Os argumentos topic, payload, e length fornecem informações sobre a mensagem recebida.
void toggleRelay(bool state);:
  Declara o protótipo da função toggleRelay(). Esta função provavelmente controla um relé, ligando-o (state = true) ou desligando-o (state = false).
void dht_sensor_getdata();:
  Declara o protótipo da função dht_sensor_getdata(). Esta função provavelmente lê dados de temperatura e umidade do sensor DHT
  e os armazena em variáveis apropriadas.
void water_sensor_getdata();:
  Declara o protótipo da função water_sensor_getdata(). Esta função, aparentemente, não está implementada no código fornecido.
  Provavelmente, ela seria usada para obter dados de um sensor de água (não mostrado).
void displayCoffeeMakerState();:
  Declara o protótipo da função displayCoffeeMakerState(). Esta função provavelmente exibe informações sobre o estado da cafeteira no display LCD.
void displayHelloWorld();:
  Declara o protótipo da função displayHelloWorld(). Esta função provavelmente exibe a mensagem "Hello World" no display LCD.
*/
bool connectMQTT();                            // Função para conectar ao broker MQTT
void callback(char *topic, byte *payload, unsigned int length);  // Função callback para receber mensagens MQTT
void toggleRelay(bool state);                  // Função para ligar/desligar o relé
void dht_sensor_getdata();                     // Função para obter dados do sensor DHT
void water_sensor_getdata();                   // Função para obter dados do sensor de água (implementação não mostrada)
void displayCoffeeMakerState();                // Função para exibir o estado da cafeteira no LCD
void displayHelloWorld();                      // Função para exibir "Hello World" no LCD
// **  ------------ ------------ ------------- ** 

void setup() {
  // Inicialização da comunicação serial
  Serial.begin(9600);                          // configura a comunicação serial com uma taxa de baudrate de 9600 bps. Isso permite que o Arduino envie e receba dados de um computador ou outro dispositivo através da porta serial.

  // Inicialização do sensor DHT
  dht.begin();                                 // Inicia a comunicação com o sensor DHT

  // Inicialização do display LCD
  lcd.begin(16, 2);                            // configura o display LCD com 16 colunas e 2 linhas. Isso define a resolução do display e permite que você exiba texto e imagens nele.
  lcd.clear();                                 // limpa a tela do LCD, removendo qualquer texto ou imagem que esteja exibida no momento.

  // Inicialização da conexão WiFi
  WiFi.begin(ssid, password);                  // Inicia a conexão WiFi com a rede SSID e senha especificadas
  while (WiFi.status() != WL_CONNECTED) {      // Aguarda a conexão WiFi ser estabelecida
    Serial.print(".");                         // Imprime um ponto na serial a cada iteração
    delay(1000);                               // Aguarda 1 segundo entre cada iteração
  }
  Serial.println("Conectado à rede WiFi!");    // Imprime mensagem de sucesso na conexão WiFi

  // Inicialização da conexão MQTT
  client.setServer(mqtt_broker, mqtt_port);    // Define o endereço do broker MQTT e a porta de conexão
  client.setCallback(callback);                // Define a função callback para receber mensagens MQTT
  mqttStatus = connectMQTT();                  // Tenta conectar-se ao broker MQTT e armazena o resultado na variável `mqttStatus`

  // Configuração do pino do relé
  pinMode(RELAY_PIN, OUTPUT);                  // Configura o pino do relé como saída
  digitalWrite(RELAY_PIN, LOW);                // Desliga o relé inicialmente

  // Configuração dos pinos dos LEDs como saída
  pinMode(RED_LED_PIN, OUTPUT);                // Configura o pino do LED vermelho como saída
  pinMode(YELLOW_LED_PIN, OUTPUT);             // Configura o pino do LED amarelo como saída
  pinMode(GREEN_LED_PIN, OUTPUT);              // Configura o pino do LED verde como saída

  // Inicialização do sensor de água
  pinMode(WATER_SENSOR_PIN, INPUT);            // Configura o pino do sensor de água como entrada
  delay(100);                                  // Aguarda 100ms para estabilização do sensor

  // Inicialização das leituras com 0
  for (int i = 0; i < numReadings; i++) {      // Percorre o vetor de leituras
    readings[i] = 0;                           // Inicializa cada elemento do vetor com 0
  }

  // Exibe temperatura e umidade no LCD
  dht_sensor_getdata();                        // Obtém e exibe temperatura e umidade no LCD
}

void loop() {
  /*
  A função dht_sensor_getdata() é chamada para obter e atualizar os dados de temperatura e umidade do sensor DHT.
  A função water_sensor_getdata() (implementação não mostrada) seria chamada para obter e atualizar os dados do nível da água do sensor de água.
  */
  // Atualiza dados dos sensores DHT e de água
  dht_sensor_getdata();                       // Obtém e atualiza dados de temperatura e umidade
  water_sensor_getdata();                     // Obtém e atualiza dados do nível da água (implementação não mostrada)

  /*
  Gerenciamento da conexão MQTT:

    A variável mqttStatus é verificada para saber se a conexão MQTT está estabelecida.
    Se a conexão estiver perdida (!client.connected()) a função connectMQTT() é chamada para tentar reconectar-se ao broker MQTT.
    A função client.loop() é chamada para processar mensagens recebidas do broker MQTT e publicar novas mensagens, caso necessário.
  */

  // Mantém a conexão MQTT
  if (mqttStatus) {                           // Verifica se a conexão MQTT está estabelecida
    if (!client.connected()) {                // Verifica se o cliente MQTT está conectado
      mqttStatus = connectMQTT();             // Tenta reconectar ao broker MQTT se necessário
    }
    client.loop();                            // Processa mensagens recebidas e publica novas mensagens
  }

  // Verifica se é necessário atualizar a exibição no LCD
  unsigned long currentTime = millis();       // Obtém o tempo atual em milissegundos
  if (currentTime - lastDisplayTime >= displayInterval) { // Compara com o intervalo de atualização
    lastDisplayTime = currentTime;            // Atualiza o tempo da última atualização

  // Alterna entre os modos de exibição
  if (displayTempHum) {
    displayTempHum = false;                   // Desativa a exibição de temperatura e umidade
    displayWaterLevel = true;                 // Ativa a exibição do nível da água
    displayHello = false;                     // Desativa a exibição de "Hello World"
  } else if (displayWaterLevel) {
    displayWaterLevel = false;                // Desativa a exibição do nível da água
    displayHello = true;                      // Ativa a exibição de "Hello World"
    displayTempHum = false;                   // Desativa a exibição de temperatura e umidade
  } else {
    displayTempHum = true;                    // Ativa a exibição de temperatura e umidade
    displayWaterLevel = false;                // Desativa a exibição do nível da água
    displayHello = false;                     // Desativa a exibição de "Hello World"
  }

  // Exibe de acordo com o modo de exibição atual
  if (displayTempHum) {
    dht_sensor_getdata();                     // Atualiza e exibe temperatura e umidade
  } else if (displayWaterLevel) {
    water_sensor_getdata();                   // Atualiza e exibe o nível da água (implementação não mostrada)
  } else {
    displayHelloWorld();                      // Exibe "Hello World" no LCD
    }
  }

  // Adiciona um pequeno delay para evitar leituras muito frequentes
  delay(1000);                                // Aguarda 1 segundo entre cada iteração do loop
}

/*
Inicialização do Contador de Tentativas:

A variável tentativa é inicializada com o valor 0, indicando que ainda não houve tentativas de conexão.
Loop de Tentativas de Conexão:

Um loop while é executado enquanto as seguintes condições forem verdadeiras:
O cliente MQTT não está conectado (!client.connected())
O número de tentativas não atingiu o limite máximo (tentativa < 5)
Tentativa de Conexão:

A função client.connect() é chamada com os seguintes parâmetros:
"arduinoClient": ID do cliente a ser usado na conexão
mqtt_username: Nome de usuário para autenticação no broker
mqtt_password: Senha para autenticação no broker
Se a conexão for bem-sucedida:
Uma mensagem de sucesso é impressa na serial
O cliente assina o tópico principal (topic)
O cliente assina o tópico do relé (topic)
A função retorna true indicando que a conexão foi bem-sucedida
Se a conexão falhar:
Uma mensagem de erro é impressa na serial, incluindo o estado da conexão e o número da tentativa
Um atraso de 2 segundos (delay(2000)) é adicionado antes da próxima tentativa
O contador de tentativas é incrementado (tentativa++)
Mensagem de Falha Final:

Se o loop de tentativas for encerrado e a conexão ainda não tiver sido estabelecida, uma mensagem final de falha é impressa na serial.
Retorno de Falha:

A função retorna false para indicar que a conexão com o broker MQTT falhou.

*/
bool connectMQTT() {
  byte tentativa = 0; // Contador de tentativas de conexão

  while (!client.connected() && tentativa < 5) { // Laço para tentar a conexão
    if (client.connect("arduinoClient", mqtt_username, mqtt_password)) { // Tenta conectar ao broker
      Serial.println("Conexão bem-sucedida ao broker MQTT!"); // Mensagem de sucesso
      client.subscribe(topic); // Assina o tópico principal
      client.subscribe(topic); // Assina o tópico do relé
      return true; // Retorna `true` em caso de sucesso
    } else {
      Serial.print("Falha ao conectar: "); // Mensagem de erro
      Serial.println(client.state()); // Imprime o estado da conexão
      Serial.print("Tentativa: "); // Imprime a tentativa atual
      Serial.println(tentativa); // Imprime o contador de tentativas
      delay(2000); // Aguarda 2 segundos antes da próxima tentativa
      tentativa++; // Incrementa o contador de tentativas
    }
  }

  Serial.println("Não foi possível conectar ao broker MQTT"); // Mensagem de falha final
  return false; // Retorna `false` em caso de falha
}

void callback(char *topic, byte *payload, unsigned int length) {
  // Imprime informações sobre a mensagem recebida
  Serial.print("Mensagem recebida no tópico: ");
  Serial.println(topic); // Tópico da mensagem

  String message = ""; // String para armazenar a mensagem recebida
  for (int i = 0; i < length; i++) { // Percorre o payload da mensagem
    message += (char)payload[i]; // Concatena cada caractere do payload na string
  }

  Serial.print("Mensagem: ");
  Serial.println(message); // Mensagem recebida

  // Verifica se o tópico recebido é igual ao tópico esperado para controle da cafeteira
   if (String(topic) == String(topic)) { // Substitua "topic" pelo tópico real
    if (message.equals("ligado")) {
      coffeeMakerOn = true; // Ativa a cafeteira
    } else if (message.equals("desligado")) {
      coffeeMakerOn = false; // Desativa a cafeteira
    }
    displayCoffeeMakerState(); // Atualiza a exibição no LCD
  }

  // Verifica se a mensagem é um comando genérico de ligar ou desligar
  if (message.equals("ligar")) {
    toggleRelay(true); // Liga o relé
  } else if (message.equals("desligar")) {
    toggleRelay(false); // Desliga o relé
  }
}

void toggleRelay(bool state) {
  // Atualiza o estado do relé
  relayState = state;

  // Controla o pino do relé
  digitalWrite(RELAY_PIN, state ? LOW : HIGH);

  // Publica mensagem no tópico para indicar o novo estado
  client.publish(topic, state ? "ligado" : "desligado");
}

void dht_sensor_getdata() {
  // Obtém dados de temperatura e umidade do sensor DHT
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // Imprime dados de temperatura e umidade na serial para debug (opcional)
  Serial.print("Umidade: ");
  Serial.println(humidity);
  Serial.print("Temperatura: ");
  Serial.println(temperature);

  // Exibe temperatura e umidade no LCD
  lcd.clear(); // Limpa a tela do LCD
  lcd.setCursor(0, 0); // Define o cursor na linha 0, coluna 0
  lcd.print("Temp: ");
  lcd.print(temperature);
  lcd.print(" C"); // Imprime "C" para Celsius

  lcd.setCursor(0, 1); // Define o cursor na linha 1, coluna 0
  lcd.print("Umidade: ");
  lcd.print(humidity);
  lcd.print(" %"); // Imprime "%" para porcentagem

  /*
  Conversão para String:

    A função String(temperature, 2) converte o valor da temperatura para uma string, formatando-o para exibir duas casas decimais.
     O resultado é armazenado na variável temperatureString.
    A função String(humidity, 2) converte o valor da umidade para uma string, formatando-o para exibir duas casas decimais.
      O resultado é armazenado na variável humidityString.
  */
  // Converte valores para strings com duas casas decimais
  String temperatureString = String(temperature, 2);
  String humidityString = String(humidity, 2);

  /*
  Publicação no broker MQTT:

    A função client.publish() publica a string contendo a temperatura no tópico "2bqsvw6678/temperatura2505".
    A função client.publish() publica a string contendo a umidade no tópico "2bqsvw6678/humidade2505".
  */
  // Publica os valores no broker MQTT
  client.publish("2bqsvw6678/temperatura2505", temperatureString.c_str());
  client.publish("2bqsvw6678/humidade2505", humidityString.c_str());
}

void water_sensor_getdata() {
  /*
  Atualização do Array de Leituras:
    A função mantém um array readings para armazenar as últimas numReadings leituras do sensor.
    O valor mais antigo do array (readings[readIndex]) é subtraído do total (total) de leituras acumuladas.
    Um novo valor é lido do sensor de nível de água usando analogRead(WATER_SENSOR_PIN).
    O novo valor é armazenado na posição atual (readIndex) do array readings.
    O novo valor é adicionado ao total (total) de leituras acumuladas.
    O índice readIndex é incrementado circularmente usando o módulo %. Isso garante que o array não ultrapasse o tamanho alocado.
  
  Cálculo da Média:
    A média (average) é calculada dividindo o total (total) de leituras acumuladas pelo número de leituras (numReadings) consideradas.
  
  Conversão para Porcentagem:
    O valor da média (average) é dividido por 1023 (valor máximo lido pelo ADC) e multiplicado por 100 para converter a leitura em uma porcentagem do nível de água.
  */
  // Atualiza o array de leituras do sensor de nível de água
  total -= readings[readIndex]; // Remove a leitura mais antiga do total
  readings[readIndex] = analogRead(WATER_SENSOR_PIN); // Lê novo valor do sensor
  total += readings[readIndex]; // Adiciona a nova leitura ao total
  readIndex = (readIndex + 1) % numReadings; // Incrementa o índice circular

  // Calcula a média das leituras
  average = total / numReadings;

  // Converte a média para porcentagem do nível de água
  float waterPercentage = (average / 1023.0) * 100;

  // Exibe o nível de água no LCD
  lcd.clear(); // Limpa a tela do LCD
  lcd.setCursor(0, 0); // Define o cursor na linha 0, coluna 0
  lcd.print("Nivel de Agua:");
  lcd.setCursor(0, 1); // Define o cursor na linha 1, coluna 0
  lcd.print(waterPercentage);
  lcd.print(" %");

  // Imprime o nível de água na serial (opcional)
  Serial.print("Nível de Água (%): ");
  Serial.println(waterPercentage);

  // Converte o valor da média para string com duas casas decimais
  String waterString = String(waterPercentage, 2);

  // Controla os LEDs de acordo com o nível de água
  if (waterPercentage >= 0 && waterPercentage <= 25) {
    digitalWrite(RED_LED_PIN, HIGH); // LED vermelho aceso
    digitalWrite(YELLOW_LED_PIN, LOW); // LED amarelo apagado
    digitalWrite(GREEN_LED_PIN, LOW); // LED verde apagado (nível baixo)
  } else if (waterPercentage > 25 && waterPercentage <= 50) {
    digitalWrite(RED_LED_PIN, LOW); // LED vermelho apagado
    digitalWrite(YELLOW_LED_PIN, HIGH); // LED amarelo aceso
    digitalWrite(GREEN_LED_PIN, LOW); // LED verde apagado (nível médio)
  } else if (waterPercentage > 50 && waterPercentage <= 100) {
    digitalWrite(RED_LED_PIN, LOW); // LED vermelho apagado
    digitalWrite(YELLOW_LED_PIN, LOW); // LED amarelo apagado
    digitalWrite(GREEN_LED_PIN, HIGH); // LED verde aceso (nível alto)
  }

  // Publica o nível de água no broker MQTT
  client.publish("2bqsvw6678/nivelAgua2505", waterString.c_str());
}

void displayCoffeeMakerState() {
  // Atualiza a exibição do estado da cafeteira no LCD
  lcd.setCursor(0, 1); // Define o cursor na linha 1, coluna 0
  if (coffeeMakerOn) {
    lcd.print("Cafe: Ligado "); // Exibe "Cafe: Ligado"
  } else {
    lcd.print("Cafe: Desligado"); // Exibe "Cafe: Desligado"
  }

  // Imprime o estado da cafeteira na serial para debug (opcional)
  Serial.print("Cafeteira: ");
  if (coffeeMakerOn) {
    Serial.println("Ligada"); // Imprime "Ligada"
  } else {
    Serial.println("Desligada"); // Imprime "Desligada"
  }
}

void displayHelloWorld() {
  // Exibe a mensagem "Hello World" no LCD
  lcd.clear(); // Limpa a tela do LCD
  lcd.setCursor(0, 0); // Define o cursor na linha 0, coluna 0
  lcd.print("Hoje tem cafe?"); // Exibe "Hoje tem cafe?"

  // Apaga todos os LEDs enquanto a mensagem "Hello World" está sendo exibida
  digitalWrite(RED_LED_PIN, LOW); // LED vermelho desligado
  digitalWrite(YELLOW_LED_PIN, LOW); // LED amarelo desligado
  digitalWrite(GREEN_LED_PIN, LOW); // LED verde desligado

  // Exibe o estado da cafeteira no LCD
  displayCoffeeMakerState(); // Chama a função para atualizar o estado da cafeteira
}