Esse código para o Arduino integra diversas funcionalidades, incluindo a leitura de sensores, o controle de um relé e a comunicação com um broker MQTT. Vamos detalhar o que cada parte do código faz:

### Bibliotecas Importadas
```cpp
#include "DHT.h"
#include <WiFi.h>
#include <PubSubClient.h>
```

Essas bibliotecas são necessárias para o funcionamento do sensor DHT11 (para temperatura e umidade), a conexão WiFi, e a comunicação MQTT, respectivamente.

### Parâmetros de Conexão WiFi e MQTT
```cpp
const char* ssid = "pedacinhodoceu2.4g"; // REDE
const char* password = "Pedacinho32377280"; // SENHA

const char* mqtt_broker = "b37.mqtt.one"; // Host do broker
const char* topic = "2bqsvw6678/"; // Tópico a ser subscrito e publicado
const char* mqtt_username = "2bqsvw6678"; // Usuário
const char* mqtt_password = "0efiqruwxy"; // Senha
const int mqtt_port = 1883; // Porta
```

Essas variáveis armazenam as credenciais da rede WiFi e do broker MQTT que serão usados para a conexão.

### Configuração do Relé e Sensores
```cpp
const int relayPin = 2; // Pino do relé

#define DHTTYPE DHT11 // Tipo de sensor DHT
#define dht_dpin 0 // Pino de dados do DHT11
DHT dht(dht_dpin, DHTTYPE);

const int waterSensorPin = A1;  // Pino do sensor de água

const int numReadings = 10;  // Número de leituras para fazer a média
int readings[numReadings];   // Armazena as leituras analógicas
int readIndex = 0;           // Índice da leitura atual
int total = 0;               // Soma das leituras
int average = 0;             // Média das leituras

bool mqttStatus = false;
bool relayState = false; // Estado inicial do relé

WiFiClient wifiClient;
PubSubClient client(wifiClient);
```

Configura o pino do relé, o sensor de temperatura e umidade (DHT11), e o sensor de nível de água.

### Protótipos de Funções
```cpp
bool connectMQTT();
void callback(char* topic, byte* payload, unsigned int length);
void toggleRelay(bool state);
void dht_sensor_getdata();
void water_sensor_getdata();
```

Declara as funções utilizadas no programa.

### Função `setup()`
```cpp
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
  
  // Inicializa o sensor de água
  pinMode(waterSensorPin, INPUT);
  delay(100);
  
  // Inicializa todas as leituras com 0
  for (int i = 0; i < numReadings; i++) {
    readings[i] = 0;
  }
}
```

- Conecta o Arduino à rede WiFi.
- Conecta ao broker MQTT.
- Configura o pino do relé.
- Inicializa os sensores DHT e de nível de água.

### Função `loop()`
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
  
  // Adiciona um pequeno delay para evitar leituras muito frequentes
  delay(2000);
}
```

- Lê os dados dos sensores.
- Mantém a conexão com o broker MQTT e processa mensagens recebidas.
- Espera 2 segundos antes de repetir o loop.

### Função `connectMQTT()`
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

### Função `callback()`
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

  if (message.equals("ligar")) {
    toggleRelay(false);
  } else if (message.equals("desligar")) {
    toggleRelay(true);
  }
}
```

Processa mensagens recebidas no tópico MQTT e controla o relé baseado nas mensagens "ligar" ou "desligar".

### Função `toggleRelay()`
```cpp
void toggleRelay(bool state) {
  relayState = state;
  digitalWrite(relayPin, state ? HIGH : LOW);
  client.publish(topic, state ? "ligado" : "desligado");
}
```

Liga ou desliga o relé e publica o novo estado no tópico MQTT.

### Função `dht_sensor_getdata()`
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
}
```

Lê os dados do sensor DHT11, imprime no monitor serial e publica os valores no tópico MQTT.

### Função `water_sensor_getdata()`
```cpp
void water_sensor_getdata() {
  total = total - readings[readIndex];
  readings[readIndex] = analogRead(waterSensorPin);
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
}
```

Lê o nível de água, calcula a média das leituras, imprime no monitor serial e publica o valor no tópico MQTT.

### Documentação

#### Bibliotecas 
- **DHT.h** : Para ler dados do sensor de temperatura e umidade DHT11. 
- **WiFi.h** : Para conectar o Arduino à rede WiFi. 
- **PubSubClient.h** : Para comunicação com o broker MQTT.

#### Parâmetros de Conexão 
- **WiFi** : Nome da rede (`ssid`) e senha (`password`). 
- **MQTT** : Endereço do broker (`mqtt_broker`), porta (`mqtt_port`), tópico (`topic`), nome de usuário (`mqtt_username`), e senha (`mqtt_password`).

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
- **loop()** : Atualiza dados dos sensores, mantém a conexão MQTT ativa, e adiciona um delay para evitar leituras muito frequentes. 
- **connectMQTT()** : Tenta conectar ao broker MQTT e retorna o status da conexão. 
- **callback()** : Processa mensagens recebidas no tópico MQTT e controla o relé. 
- **toggleRelay()** : Alterna o estado do relé e publica o novo estado no tópico MQTT. 
- **dht_sensor_getdata()** : Lê dados do sensor DHT11, imprime no monitor serial e publica no tópico MQTT. 
- **water_sensor_getdata()** : Lê dados do sensor de nível de água, calcula a média, imprime no monitor serial e publica no tópico MQTT.

### Funcionalidades

- Este projeto Arduino possui várias funcionalidades que se integram para criar um sistema de monitoramento e controle remoto para uma cafeteira ou dispositivo similar. Aqui estão as principais funcionalidades:
### 1. Conexão WiFi

O projeto conecta o Arduino a uma rede WiFi usando as credenciais fornecidas (SSID e senha). Isso permite que o dispositivo se comunique com o broker MQTT e receba comandos remotos.
### 2. Conexão MQTT

O Arduino se conecta a um broker MQTT, permitindo a publicação e a subscrição de mensagens em tópicos específicos. Isso é essencial para a comunicação remota e a integração com outras plataformas e dispositivos IoT.
### 3. Controle do Relé

O projeto controla um relé, que pode ser usado para ligar e desligar a cafeteira ou outro dispositivo. O estado do relé pode ser alterado remotamente através de mensagens MQTT recebidas.
### 4. Leitura de Sensores
#### Sensor de Temperatura e Umidade (DHT11) 
- **Leitura de Temperatura:**  O sensor DHT11 lê a temperatura ambiente. 
- **Leitura de Umidade:**  O sensor DHT11 também lê a umidade relativa do ar. 
- **Publicação dos Dados:**  Os valores de temperatura e umidade são publicados periodicamente em tópicos MQTT específicos.
#### Sensor de Nível de Água 
- **Leitura de Nível de Água:**  O sensor de nível de água lê a quantidade de água disponível na cafeteira. 
- **Cálculo da Média:**  Para obter leituras mais estáveis, o código calcula a média de várias leituras. 
- **Publicação dos Dados:**  O nível de água é publicado em um tópico MQTT específico.
### 5. Callback para Mensagens MQTT

O projeto inclui uma função de callback que processa mensagens recebidas nos tópicos subscritos. Dependendo da mensagem recebida ("ligar" ou "desligar"), o relé é acionado para ligar ou desligar a cafeteira.
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
