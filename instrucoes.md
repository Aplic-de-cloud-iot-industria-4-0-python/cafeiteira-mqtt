Este projeto para Arduino integra diversas funcionalidades, incluindo a leitura de sensores, o controle de um relé e a comunicação com um broker MQTT. A seguir, detalhamos cada parte do código.

## Bibliotecas Importadas
```cpp
#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>
#include "DHT.h"
#include <WiFi.h>
#include <PubSubClient.h>
```
 
- **Wire.h**  e **hd44780** : Para controle do display LCD via I2C. 
- **DHT.h** : Para o sensor DHT11 de temperatura e umidade. 
- **WiFi.h** : Para conexão WiFi. 
- **PubSubClient.h** : Para comunicação MQTT.

## Parâmetros de Conexão WiFi e MQTT
```cpp
const char* ssid = "pedacinhodoceu2.4g"; // REDE
const char* password = "Pedacinho32377280"; // SENHA
const char* mqtt_broker = "b37.mqtt.one"; // Host do broker
const char* topic = "2bqsvw6678/"; // Tópico a ser subscrito e publicado
const char* mqtt_username = "2bqsvw6678"; // Usuário
const char* mqtt_password = "0efiqruwxy"; // Senha
const int mqtt_port = 1883; // Porta
```
 
- **ssid** : Nome da rede WiFi. 
- **password** : Senha da rede WiFi. 
- **mqtt_broker** : Endereço do broker MQTT. 
- **topic** : Tópico MQTT para subscrição e publicação. 
- **mqtt_username**  e **mqtt_password** : Credenciais do broker MQTT. 
- **mqtt_port** : Porta do broker MQTT.

## Configuração do Relé e Sensores
```cpp
#define BUTTON_PIN 13       // Pino do botão
#define DHTPIN 0            // Pino de dados do DHT11
#define DHTTYPE DHT11       // Tipo de sensor DHT (DHT11 ou DHT22)
#define WATER_SENSOR_PIN A0 // Pino analógico onde o sensor de água está conectado
#define CONTRAST_PIN A2     // Pino do potenciômetro para controlar o contraste do LCD
#define RED_LED_PIN 4       // LED vermelho
#define YELLOW_LED_PIN 3    // LED amarelo
#define GREEN_LED_PIN 2     // LED verde
#define RELAY_PIN 52        // Pino do relé

const int numReadings = 10; // Número de leituras para fazer a média
int readings[numReadings];  // Armazena as leituras analógicas
int readIndex = 0;          // Índice da leitura atual
int total = 0;              // Soma das leituras
int average = 0;            // Média das leituras

bool mqttStatus = false;
bool relayState = false; // Estado inicial do relé
bool coffeeMakerOn = false; // Estado da cafeteira

WiFiClient wifiClient;
PubSubClient client(wifiClient);
DHT dht(DHTPIN, DHTTYPE);
hd44780_I2Cexp lcd;
```
 
- **Pinos definidos**  para o botão, sensor DHT11, sensor de água, controle de contraste do LCD, LEDs e relé. 
- **Variáveis de leitura**  para armazenar e calcular a média das leituras do sensor de água. 
- **Variáveis de estado**  para gerenciar a conexão MQTT e o estado do relé.

## Protótipos de Funções
```cpp
bool connectMQTT();
void callback(char* topic, byte* payload, unsigned int length);
void toggleRelay(bool state);
void dht_sensor_getdata();
void water_sensor_getdata();
void displayCoffeeMakerState();
void displayHelloWorld();
```

Declaração das funções utilizadas no programa.

## Função `setup()`
```cpp
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
```

- Conecta o Arduino à rede WiFi.
- Conecta ao broker MQTT.
- Configura pinos do relé, LEDs, botão e sensores.
- Inicializa o sensor de temperatura e umidade (DHT11) e o sensor de nível de água.

## Função `loop()`
```cpp
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

  // Verifica se o tempo desde a última atualização é maior que o intervalo definido
  unsigned long currentTime = millis();
  if (currentTime - lastDisplayTime >= displayInterval) {
    // Atualiza o tempo da última exibição
    lastDisplayTime = currentTime;

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
      displayHelloWorld();
    }
  }

  // Verifica se o botão foi pressionado
  if (digitalRead(BUTTON_PIN) == LOW) {
    // Aguarda até que o botão seja liberado
    while (digitalRead(BUTTON_PIN) == LOW) {
      delay(10);
    }
  }

  // Adiciona um pequeno delay para evitar leituras muito frequentes
  delay(1000);
}
```

- Atualiza os dados dos sensores DHT e de nível de água.
- Mantém a conexão MQTT.
- Alterna entre modos de exibição no LCD a cada 10 segundos.
- Verifica se o botão foi pressionado.
- Adiciona um pequeno delay para evitar leituras muito frequentes.

## Função `connectMQTT()`
```cpp
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
```

Tenta conectar ao broker MQTT até 5 vezes e retorna o status da conexão.

## Função `callback()`
```cpp
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensagem recebida no tópico: ");
  Serial.println(topic);

  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("Mensagem: ");
  Serial.println(message);

  if (String(topic) == String(topic)) {
    if (message.equals("ligado")) {
      coffeeMakerOn = true;
    } else if (message.equals("desligado")) {
      coffeeMakerOn = false;
    }
    displayCoffeeMakerState(); // Atualiza a exibição quando o estado do relé mudar
  }

  if (message.equals("ligar")) {
    toggleRelay(true);
  } else if (message.equals("desligar")) {
    toggleRelay(false);
  }
}
```

Processa mensagens recebidas no tópico MQTT e controla o relé baseado nas mensagens "ligar" ou "desligar".

## Função `toggleRelay()`
```cpp
void toggleRelay(bool state) {
  relayState = state;
  digitalWrite(RELAY_PIN, state ? HIGH : LOW);
  client.publish(topic, state ? "ligado" : "desligado");
}
```

Liga ou desliga o relé e publica o novo estado no tópico MQTT.

## Função `dht_sensor_getdata()`
```cpp
void dht_sensor_getdata() {
  float hm = dht.readHumidity();
  Serial.print("Umidade: ");
  Serial.println(hm);
  float temp = dht.readTemperature();
  Serial.print("Temperatura: ");
  Serial.println(temp);

  // Converte os valores para String
  String tempString = String(temp, 2);
  String humString = String(hm, 2);

  // Publica os valores no broker MQTT
  client.publish("2bqsvw6678/temperatura2505", tempString.c_str());
  client.publish("2bqsvw6678/humidade2505", humString.c_str());

  // Exibe os valores no LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp: " + tempString + " C");
  lcd.setCursor(0, 1);
  lcd.print("Umid: " + humString + " %");
}
```

Lê os dados do sensor DHT11, imprime no monitor serial, publica os valores no tópico MQTT e exibe no LCD.

## Função `water_sensor_getdata()`
```cpp
void water_sensor_getdata() {
  total = total - readings[readIndex];
  readings[readIndex] = analogRead(WATER_SENSOR_PIN);
  total = total + readings[readIndex];
  readIndex = readIndex + 1;

  if (readIndex >= numReadings) {
    readIndex = 0;
  }

  average = total / numReadings;
  Serial.print("Nível de Água: ");
  Serial.println(average);

  String waterString = String(average);
  client.publish("2bqsvw6678/nivelAgua2505", waterString.c_str());

  // Atualiza os LEDs com base no nível de água
  if (average < 300) {
    digitalWrite(RED_LED_PIN, HIGH);
    digitalWrite(YELLOW_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, LOW);
  } else if (average < 700) {
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(YELLOW_LED_PIN, HIGH);
    digitalWrite(GREEN_LED_PIN, LOW);
  } else {
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(YELLOW_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, HIGH);
  }

  // Exibe o nível de água no LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Nivel da Agua:");
  lcd.setCursor(0, 1);
  lcd.print(waterString);
}
```

Lê o nível de água, calcula a média das leituras, imprime no monitor serial, publica o valor no tópico MQTT e atualiza os LEDs e o LCD com o nível de água.

## Função `displayCoffeeMakerState()`
```cpp
void displayCoffeeMakerState() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Cafeteira:");
  lcd.setCursor(0, 1);
  if (coffeeMakerOn) {
    lcd.print("Ligada");
  } else {
    lcd.print("Desligada");
  }
}
```

Exibe o estado atual da cafeteira no LCD.

## Função `displayHelloWorld()`
```cpp
void displayHelloWorld() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Hoje tem cafe?");
  lcd.setCursor(0, 1);
  lcd.print("  Ligada");
}
```

Exibe a mensagem "Hoje tem café?" no LCD e apaga todos os LEDs.---

Com esta documentação, você tem uma visão detalhada de como o projeto está estruturado e como cada parte do código funciona. Isso deve ajudar outros desenvolvedores a entender e contribuir com o seu projeto.

## Configuração do Broker MQTT

Abaixo estão as diferentes configurações para conectar o projeto a vários brokers MQTT na cloud. Comente ou descomente as seções relevantes no código para alternar entre os brokers.

### 1. MQTTBox e NQTTX com Mosquitto

```cpp
const char *mqtt_broker = "test.mosquitto.org";  // Host do broker
const char *topic = "grupo5/cafeteira";          // Tópico a ser subscrito e publicado
const char *mqtt_username = "";                  // Usuário
const char *mqtt_password = "";                  // Senha
const int mqtt_port = 1883;                      // Porta
```

### 2. MQTTX

```cpp
const char *mqtt_broker = "broker.emqx.io";      // Host do broker
const char *topic = "grupo5/cafeteira";          // Tópico a ser subscrito e publicado
const char *mqtt_username = "2bqsvw6678";        // Usuário
const char *mqtt_password = "0efiqruwxy";        // Senha
const int mqtt_port = 8083;                      // Porta
```

### 3. IoTBind

```cpp
const char *mqtt_broker = "b37.mqtt.one";        // Host do broker
const char *topic = "45eiqx7836";                // Tópico a ser subscrito e publicado
const char *mqtt_username = "45eiqx7836";        // Usuário
const char *mqtt_password = "357fgiuwyz";        // Senha
const int mqtt_port = 1883;                      // Porta
```

### 4. MyQttHub

```cpp
const char *mqtt_broker = "node02.myqtthub.com"; // Host do broker
const char *topic = "grupo5/myqtthub";           // Tópico a ser subscrito e publicado
const char *mqtt_username = "estevam5s";         // Usuário
const char *mqtt_password = "PX7ppiJ7-VBtBdAfH"; // Senha
const char *client_id = "estevamsouzalaureth@gmail.com"; // Cliente ID
const int mqtt_port = 8883;                      // Porta
```

## Estrutura do Código para cada Broker MQTT na Nuvem

O código principal está dividido nas seguintes seções: 
1. **Configuração do WiFi e MQTT** : Parâmetros de conexão para a rede WiFi e broker MQTT. 
2. **Definição dos Sensores e Pinos** : Configuração dos sensores DHT11 e sensor de nível de água. 
3. **Setup** : Inicialização da conexão WiFi e MQTT, configuração dos pinos e inicialização dos sensores. 
4. **Loop** : Atualização dos dados dos sensores, manutenção da conexão MQTT e publicação dos dados no broker.
## Como Utilizar 
1. **Configure a Rede WiFi** :
Atualize as variáveis `ssid` e `password` com o nome e senha da sua rede WiFi. 
2. **Escolha o Broker MQTT** :
Comente ou descomente a seção relevante no código para selecionar o broker MQTT desejado. 
3. **Carregue o Código no Microcontrolador** :
Carregue o código atualizado no seu microcontrolador (ESP32, Arduino, etc.). 
4. **Monitore os Dados** :
Conecte-se ao broker MQTT utilizando uma ferramenta como MQTTBox, MQTTX, ou qualquer outro cliente MQTT para visualizar os dados publicados.

## Exemplo de Uso
Com o broker configurado, os dados de temperatura, umidade e nível de água serão publicados nos tópicos MQTT configurados. Por exemplo, para o broker `test.mosquitto.org`: 
- Tópico para temperatura: `grupo5/cafeteira/temperatura` 
- Tópico para umidade: `grupo5/cafeteira/umidade` 
- Tópico para nível de água: `grupo5/cafeteira/nivelAgua`

### Documentação
#### Bibliotecas 
- **DHT.h** : Para ler dados do sensor de temperatura e umidade DHT11. 
- **WiFi.h** : Para conectar o Arduino à rede WiFi. 
- **PubSubClient.h** : Para comunicação com o broker MQTT.
#### Parâmetros de Conexão 
- **WiFi** : Nome da rede (`ssid`) e senha (`password`). 
- **MQTT** : Endereço do broker (`mqtt_broker`), porta (`mqtt_port`), tópico (`topic`), nome de usuário (`mqtt_username`) e senha (`mqtt_password`).
#### Configuração de Pinos 
- **relayPin** : Pino conectado ao relé. 
- **dht_dpin** : Pino de dados do sensor DHT11. 
- **waterSensorPin** : Pino analógico para o sensor de nível de água.
#### Variáveis e Objetos 
- **dht** : Objeto para o sensor DHT11. 
- **readings, readIndex, total, average** : Variáveis para leituras e média do sensor de água. 
- **mqttStatus, relayState** : Variáveis para estados do MQTT e do relé. 
- **wifiClient, client** : Objetos para cliente WiFi e MQTT.
#### Funções 
- **setup()** : Configura a conexão WiFi e MQTT, inicializa sensores e relé. 
- **loop()** : Atualiza dados dos sensores, mantém a conexão MQTT ativa e adiciona um delay para evitar leituras muito frequentes. 
- **connectMQTT()** : Tenta conectar ao broker MQTT e retorna o status da conexão. 
- **callback()** : Processa mensagens recebidas no tópico MQTT e controla o relé. 
- **toggleRelay()** : Alterna o estado do relé e publica o novo estado no tópico MQTT. 
- **dht_sensor_getdata()** : Lê dados do sensor DHT11, imprime no monitor serial e publica no tópico MQTT. 
- **water_sensor_getdata()** : Lê dados do sensor de nível de água, calcula a média, imprime no monitor serial e publica no tópico MQTT.

### Funcionalidades
Este projeto Arduino possui várias funcionalidades que se integram para criar um sistema de monitoramento e controle remoto para uma cafeteira ou dispositivo similar. Aqui estão as principais funcionalidades: 
1. **Conexão WiFi** :
- O projeto conecta o Arduino a uma rede WiFi usando as credenciais fornecidas (SSID e senha). Isso permite que o dispositivo se comunique com o broker MQTT e receba comandos remotos. 
2. **Conexão MQTT** :
- O Arduino se conecta a um broker MQTT, permitindo a publicação e a subscrição de mensagens em tópicos específicos. Isso é essencial para a comunicação remota e a integração com outras plataformas e dispositivos IoT. 
3. **Controle do Relé** :
- O projeto controla um relé, que pode ser usado para ligar e desligar a cafeteira ou outro dispositivo. O estado do relé pode ser alterado remotamente através de mensagens MQTT recebidas. 
4. **Leitura de Sensores** : 
- **Sensor de Temperatura e Umidade (DHT11)** :
- O sensor DHT11 lê a temperatura ambiente.
- O sensor DHT11 também lê a umidade relativa do ar.
- Os valores de temperatura e umidade são publicados periodicamente em tópicos MQTT específicos. 
- **Sensor de Nível de Água** :
- O sensor de nível de água lê a quantidade de água disponível na cafeteira.
- Para obter leituras mais estáveis, o código calcula a média de várias leituras.
- O nível de água é publicado em um tópico MQTT específico. 
5. **Callback para Mensagens MQTT** :
- O projeto inclui uma função de callback que processa mensagens recebidas nos tópicos subscritos. Dependendo da mensagem recebida ("ligar" ou "desligar"), o relé é acionado para ligar ou desligar a cafeteira. 

### 6. Feedback Visual
Embora não utilizado diretamente neste projeto, há um pino configurado para um LED que pode ser usado para indicar o estado do sistema ou alertar sobre condições específicas (por exemplo, alta temperatura).

### Resumo das Funcionalidades 
1. **Conexão e Comunicação:** 
- Conecta-se à rede WiFi.
- Conecta-se a um broker MQTT.
- Publica e subscreve a tópicos MQTT. 
2. **Controle de Dispositivos:** 
- Controle remoto do relé via MQTT. 
3. **Monitoramento Ambiental:** 
- Leitura de temperatura e umidade com o sensor DHT11.
- Leitura do nível de água com um sensor de nível de água.
- Publicação dos dados dos sensores em tópicos MQTT. 
4. **Processamento de Mensagens:** 
- Recebe e processa comandos MQTT para ligar e desligar o relé.

### Resumo
Esse código conecta o Arduino a uma rede WiFi e a um broker MQTT, lê dados de sensores de temperatura, umidade e nível de água, e publica esses dados em tópicos MQTT. Também controla um relé através de mensagens recebidas via MQTT, permitindo o controle remoto de dispositivos conectados ao relé.