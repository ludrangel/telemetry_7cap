#include <LoRa.h>
#include <SPI.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "Arduinojson.hpp"

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


#define BAND    915E6   // Frequência do rádio - podemos utilizar ainda: 433E6, 868E6, 915E6
#define PABOOST true

// variaveis
float values1[33]; // Declare values1 as an array of floats with size 
float values2[33]; // Declare values2 as an array of floats with size

// Declaração do cliente Wi-Fi e do cliente MQTT
WiFiClient espClient;
PubSubClient client(espClient);

// Função para dividir uma string em substrings com base em um delimitador
void splitString(const String& input, char delimiter, String* outputArray, int maxItems, bool onlyAlphaNumeric) {
  int itemCount = 0; // Contador de itens
  int startIndex = 0; // Índice inicial da substring
  int endIndex = input.indexOf(delimiter); // Índice final da substring

  // Enquanto houver substrings e não exceder o número máximo de itens
  while (endIndex >= 0 && itemCount < maxItems) {
    // Extrai a substring atual e armazena no array de saída
    String substring = input.substring(startIndex, endIndex);

    if (onlyAlphaNumeric) {
      // Remove caracteres não alfanuméricos da substring
      substring.replace("[^a-zA-Z0-9]", "");
    }

    outputArray[itemCount] = substring;
    itemCount++;

    // Atualiza os índices para a próxima substring
    startIndex = endIndex + 1;
    endIndex = input.indexOf(delimiter, startIndex);
  }

  // Verifica se há uma última substring após o último delimitador
  if (startIndex < input.length() && itemCount < maxItems) {
    String substring = input.substring(startIndex);

    if (onlyAlphaNumeric) {
      // Remove caracteres não alfanuméricos da substring
      substring.replace("[^a-zA-Z0-9]", "");
    }

    outputArray[itemCount] = substring;
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
  LoRa.setTxPower(20); // Define o nível de potência adequado para o seu módulo LoRa
  LoRa.setSpreadingFactor(12); // Define o Spreading Factor (fator de espalhamento) para 12
  LoRa.setSignalBandwidth(500E3); // Define a largura de banda de sinal para 500 kHz
  LoRa.setCodingRate4(5); // Define a taxa de codificação como 5 (4/5)
  LoRa.setPreambleLength(8); // Define o comprimento do Preamble como 8
  LoRa.setSyncWord(0x12); // Define a palavra de sincronização como 0x12
  LoRa.enableCrc(); // Ativa o CRC (Cyclic Redundancy Check)  // Para desativar : void disableCrc(); 
  LoRa.disableInvertIQ(); // Desative a inversão de IQ


  // Define o canal desejado (Canal 10)
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
  // Declare the variable receivedData
  String receivedData;
  // Verifica se há pacotes recebidos
  if (LoRa.parsePacket()) {
    // Lê os dados do pacote recebido
    while (LoRa.available()) {
      // Decodifica os dados recebidos
      receivedData = LoRa.readString();


      // Decodificação dos dados recebidos
      int values1[33];
      int values2[33];
      int itemCount = 0;


      // Dividir a string em substrings separadas por vírgula
      String values[66];
      splitString(receivedData, ',', values, 66, true);


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
    }
      

      Serial.println("Received data: " + receivedData);
      Serial.println("Decoded values for MPPT 1:");
      for (int i = 0; i < 33; i++) {
        Serial.print("Value ");
        Serial.print(i);
        Serial.print(": ");
        Serial.println(values1[i]);
      }
      Serial.println("Decoded values for MPPT 2:");
      for (int i = 0; i < 33; i++) {
        Serial.print("Value ");
        Serial.print(i);
        Serial.print(": ");
        Serial.println(values2[i]);
      }
    }
  }
// Função para publicar os valores decodificados no tópico MQTT
void publishValues(int values1[], int values2[]) {
  String mppt1DataJson;
  mppt1DataJson = "{\"ID\":" + String(1) + ", \"PV_VOLTAGE\":" + String(values1[0]) + ", \"PV_CURRENT\":" + String(values1[1]) +
                  ", \"PV_POWER_L\":" + String(values1[2]) + ", \"PV_POWER_H\":" + String(values1[3]) + ", \"BT_VOLTAGE\":" +
                  String(values1[4]) + ", \"BT_CURRENT\":" + String(values1[5]) + ", \"BT_POWER_L\":" + String(values1[6]) +
                  ", \"BT_POWER_H\":" + String(values1[7]) + ", \"LD_VOLTAGE\":" + String(values1[8]) + ", \"LD_CURRENT\":" +
                  String(values1[9]) + ", \"LD_POWER_L\":" + String(values1[10]) + ", \"LD_POWER_H\":" + String(values1[11]) +
                  ", \"BT_CELCIUS\":" + String(values1[12]) + ", \"EQ_CELCIUS\":" + String(values1[13]) + ", \"PC_CELCIUS\":" +
                  String(values1[14]) + ", \"BT_PERCENT\":" + String(values1[15]) + ", \"RMT_BT_TMP\":" + String(values1[16]) +
                  ", \"BT_RTD_PWR\":" + String(values1[17]) + ", \"BT_STATUS_\":" + String(values1[18]) + ", \"CE_STATUS_\":" +
                  String(values1[19]) + ", \"DE_STATUS_\":" + String(values1[20]) + ", \"MAX_PV_VOLTAGE\":" + String(values1[21]) +
                  ", \"MIN_PV_VOLTAGE\":" + String(values1[22]) + ", \"MAX_BT_VOLTAGE\":" + String(values1[23]) +
                  ", \"MIN_BT_VOLTAGE\":" + String(values1[24]) + ", \"CON_ENERGY_D_L\":" + String(values1[25]) +
                  ", \"CON_ENERGY_D_H\":" + String(values1[26]) + ", \"GEN_ENERGY_D_L\":" + String(values1[27]) +
                  ", \"GEN_ENERGY_D_H\":" + String(values1[28]) + ", \"GEN_ENERGY_T_L\":" + String(values1[29]) +
                  ", \"GEN_ENERGY_T_H\":" + String(values1[30]) + ", \"BATTERY___TEMP\":" + String(values1[31]) +
                  ", \"AMBIENT___TEMP\":" + String(values1[32]) + "}";
Serial.println(mppt1DataJson);

  String mppt2DataJson;
  mppt2DataJson = "{\"ID\":" + String(2) + ", \"PV_VOLTAGE\":" + String(values2[0]) + ", \"PV_CURRENT\":" + String(values2[1]) +
                  ", \"PV_POWER_L\":" + String(values2[2]) + ", \"PV_POWER_H\":" + String(values2[3]) + ", \"BT_VOLTAGE\":" +
                  String(values2[4]) + ", \"BT_CURRENT\":" + String(values2[5]) + ", \"BT_POWER_L\":" + String(values2[6]) +
                  ", \"BT_POWER_H\":" + String(values2[7]) + ", \"LD_VOLTAGE\":" + String(values2[8]) + ", \"LD_CURRENT\":" +
                  String(values2[9]) + ", \"LD_POWER_L\":" + String(values2[10]) + ", \"LD_POWER_H\":" + String(values2[11]) +
                  ", \"BT_CELCIUS\":" + String(values2[12]) + ", \"EQ_CELCIUS\":" + String(values2[13]) + ", \"PC_CELCIUS\":" +
                  String(values2[14]) + ", \"BT_PERCENT\":" + String(values2[15]) + ", \"RMT_BT_TMP\":" + String(values2[16]) +
                  ", \"BT_RTD_PWR\":" + String(values2[17]) + ", \"BT_STATUS_\":" + String(values2[18]) + ", \"CE_STATUS_\":" +
                  String(values2[19]) + ", \"DE_STATUS_\":" + String(values2[20]) + ", \"MAX_PV_VOLTAGE\":" + String(values2[21]) +
                  ", \"MIN_PV_VOLTAGE\":" + String(values2[22]) + ", \"MAX_BT_VOLTAGE\":" + String(values2[23]) +
                  ", \"MIN_BT_VOLTAGE\":" + String(values2[24]) + ", \"CON_ENERGY_D_L\":" + String(values2[25]) +
                  ", \"CON_ENERGY_D_H\":" + String(values2[26]) + ", \"GEN_ENERGY_D_L\":" + String(values2[27]) +
                  ", \"GEN_ENERGY_D_H\":" + String(values2[28]) + ", \"GEN_ENERGY_T_L\":" + String(values2[29]) +
                  ", \"GEN_ENERGY_T_H\":" + String(values2[30]) + ", \"BATTERY___TEMP\":" + String(values2[31]) +
                  ", \"AMBIENT___TEMP\":" + String(values2[32]) + "}";
Serial.println(mppt2DataJson);
  

  // Publica os valores no tópico MQTT
  client.publish("mppt1", mppt1DataJson.c_str());
  client.publish("mppt2", mppt2DataJson.c_str());
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
