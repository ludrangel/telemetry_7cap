#include <LoRa.h>
#include <SPI.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// Configurações da rede Wi-Fi
const char* ssid = "NOME_DA_REDE";
const char* password = "SENHA_DA_REDE";

// Configurações do broker MQTT
const char* mqttServer = "IP_DO_BROKER_MQTT";
const int mqttPort = 1883;
const char* mqttUsername = "NOME_DE_USUARIO";
const char* mqttPassword = "SENHA";

#define SCK     5    // GPIO5  -- SX127x's SCK
#define MISO    19   // GPIO19 -- SX127x's MISO
#define MOSI    27   // GPIO27 -- SX127x's MOSI
#define SS      18   // GPIO18 -- SX127x's CS
#define RST     14   // GPIO14 -- SX127x's RESET
#define DI00    26   // GPIO26 -- SX127x's IRQ(Interrupt Request)

#define BAND    433E6   // Frequência do rádio - podemos utilizar ainda: 433E6, 868E6, 915E6
#define PABOOST true

WiFiClient espClient;
PubSubClient client(espClient);

// Função para dividir uma string em substrings com base em um delimitador
void splitString(const String& input, char delimiter, String* outputArray, int maxItems) {
  int itemCount = 0; // Contador de itens
  int startIndex = 0; // Índice inicial da substring
  int endIndex = input.indexOf(delimiter); // Índice final da substring

  // Enquanto houver substrings e não exceder o número máximo de itens
  while (endIndex >= 0 && itemCount < maxItems) {
    // Extrai a substring atual e armazena no array de saída
    outputArray[itemCount] = input.substring(startIndex, endIndex);
    itemCount++;

    // Atualiza os índices para a próxima substring
    startIndex = endIndex + 1;
    endIndex = input.indexOf(delimiter, startIndex);
  }

  // Verifica se há uma última substring após o último delimitador
  if (startIndex < input.length() && itemCount < maxItems) {
    outputArray[itemCount] = input.substring(startIndex);
    itemCount++;
  }
}

void setup() {
  // Inicialização da porta serial
  Serial.begin(115200);

  // Conexão com a rede Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi!");

  // Configuração do LoRa
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DI00);

  if (!LoRa.begin(BAND)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  Serial.println("LoRa initialized!");

  // Configurações do LoRa para maior alcance
  LoRa.setTxPower(20); // Defina o nível de potência adequado para o seu módulo LoRa
  LoRa.setSpreadingFactor(12); // Defina o Spreading Factor (fator de espalhamento) para 12
  LoRa.setSignalBandwidth(500E3); // Defina a largura de banda de sinal para 500 kHz
  LoRa.setCodingRate4(5); // Defina a taxa de codificação como 5 (4/5)
  LoRa.setPreambleLength(8); // Defina o comprimento do Preamble como 8
  LoRa.setSyncWord(0x12); // Defina a palavra de sincronização como 0x12
  LoRa.enableCrc(); // Ative o CRC (Cyclic Redundancy Check)
  LoRa.disableInvertIQ(); // Desative a inversão de IQ

  // Defina o canal desejado (Canal 10)
  LoRa.setFrequency(915E6 + (10 * 500E3));
   
  Serial.println("Configuração do módulo LoRa concluída.");

  // Configuração do cliente MQTT
  client.setServer(mqttServer, mqttPort);

  // Conexão com o broker MQTT
  while (!client.connected()) {
    Serial.println("Connecting to MQTT broker...");
    if (client.connect("ESP32Client", mqttUsername, mqttPassword)) {
      Serial.println("Connected to MQTT broker!");
    } else {
      Serial.print("Failed to connect to MQTT broker, rc=");
      Serial.print(client.state());
      Serial.println(" Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

void loop() {
  // Verifica se há pacotes recebidos
  if (LoRa.parsePacket()) {
    // Lê os dados do pacote recebido
    while (LoRa.available()) {
      // Decodifica os dados recebidos
      String receivedData = LoRa.readString();

      // Decodificação dos dados recebidos
      int values1[33];
      int values2[33];
      int itemCount = 0;

      // Dividir a string em substrings separadas por vírgula
      String values[66];
      splitString(receivedData, ',', values, 66);

      // Verifica se a quantidade de valores é válida
      if (itemCount == 66) {
        // Decodifica os valores do MPPT 1
        for (int i = 0; i < 33; i++) {
          values1[i] = values[i].toInt();
        }

        // Decodifica os valores do MPPT 2
        for (int i = 0; i < 33; i++) {
          values2[i] = values[i + 33].toInt();
        }

        // Publica os valores decodificados no tópico MQTT
        publishValues(values1, values2);
      }

      Serial.println("Received data: " + receivedData);
    }
  }

  // Mantém a conexão com o broker MQTT
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}

// Função para publicar os valores decodificados no tópico MQTT
void publishValues(int values1[], int values2[]) {
  String mppt1DataJson;
  mppt1DataJson = "{\"ID\" :" +String(1) + ", \"PV_VOLTAGE\":" + String(values1[0]) +",
   \"PV_CURRENT\":" + String(values1[1]) +", \"PV_POWER_L\":" + String(values1[2]) +",
   \"PV_POWER_H\":" + String(values1[3]) +", \"BT_VOLTAGE\":" + String(values1[4]) +",
   \"BT_CURRENT\":" + String(values1[5]) +", \"BT_POWER_L\":" + String(values1[6]) +", 
   \"BT_POWER_H\":" + String(values1[5]) +",    }";

  // Cria uma string com os valores do MPPT 1 separados por vírgula
  String mppt1Data;
  for (int i = 0; i < 33; i++) {
    mppt1Data += String(values1[i]);
    if (i < 17) {
      mppt1Data += ",";
    }
  }

  // Cria uma string com os valores do MPPT 2 separados por vírgula
  String mppt2Data;
  for (int i = 0; i < 33; i++) {
    mppt2Data += String(values2[i]);
    if (i < 17) {
      mppt2Data += ",";
    }
  }

  // Publica os valores no tópico MQTT
  client.publish("mppt1", mppt1Data.c_str());
  client.publish("mppt2", mppt2Data.c_str());
}

// Função para reconectar ao broker MQTT
void reconnect() {
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    if (client.connect("ESP32Client", mqttUsername, mqttPassword)) {
      Serial.println("Connected to MQTT broker!");
    } else {
      Serial.print("Failed to connect to MQTT broker, rc=");
      Serial.print(client.state());
      Serial.println(" Retrying in 5 seconds...");
      delay(5000);
    }
  }
}
