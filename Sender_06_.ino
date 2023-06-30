#include <LoRa.h>
#include "SSD1306Wire.h"
#include <ModbusMaster.h>
#include <SPI.h> //responsável pela comunicação serial
#include <Wire.h>  //responsável pela comunicação i2c


#define SCK     5    // GPIO5  -- SX127x's SCK
#define MISO    19   // GPIO19 -- SX127x's MISO
#define MOSI    27   // GPIO27 -- SX127x -- SX127x's MOSI
#define SS      18   // GPIO18 -- SX127x's CS
#define RST     14   // GPIO14 -- SX127x's RESET
#define DI00    26   // GPIO26 -- SX127x's IRQ(Interrupt Request)


#define BAND    915E6   // Frequência do rádio - podemos utilizar ainda: 433E6, 868E6, 915E6
#define PABOOST true


//parametros: address,SDA,SCL 
SSD1306Wire display(0x3c, 4, 15); //construtor do objeto que controlaremos o display


//variável responsável por armazenar o valor do contador (enviaremos esse valor para o outro Lora)
unsigned int counter = 0;


//Declare um array de caracteres para payload
char payload[256];


// Configuração do Modbus
#define MODBUS_SERIAL Serial // Comunicação serial a ser utilizada
#define MODBUS_BAUDRATE 9600 // Taxa de baudrate
ModbusMaster node1; // Endereço Modbus do primeiro MPPT é 1
ModbusMaster node2; // Endereço Modbus do segundo MPPT é 2


///////////////////
//Endereço Modbus//
///////////////////


//número de registradores
const int NUM_REGISTERS = 33;


// Dados em tempo real (somente leitura)
const int PV_VOLTAGE = 0x3100; // Tensão do painel fotovoltaico
const int PV_CURRENT = 0x3101; // Corrente do painel fotovoltaico
const int PV_POWER_L = 0x3102; // Energia do painel fotovoltaico (parte baixa)
const int PV_POWER_H = 0x3103; // Energia do painel fotovoltaico (parte alta)
const int BT_VOLTAGE = 0x3104; // Tensão da bateria
const int BT_CURRENT = 0x3105; // Corrente de carga da bateria
const int BT_POWER_L = 0x3106; // Potência de carregamento da bateria (parte baixa)
const int BT_POWER_H = 0x3107; // Potência de carregamento da bateria (parte alta)
const int LD_VOLTAGE = 0x310C; // Tensão de carga (Tensão de saída do equipamento de descarregamento)
const int LD_CURRENT = 0x310D; // Corrente de carga(Corrente de saída do equipamento de descarregamento)
const int LD_POWER_L = 0x310E; // Potência de carga (parte baixa)(Potência de saída do equipamento de descarregamento)
const int LD_POWER_H = 0x310F; // Potência de carga (parte alta)(Potência de saída do equipamento de descarregamento)
const int BT_CELCIUS = 0x3110; // Temperatura da bateria
const int EQ_CELCIUS = 0x3111; // Temperatura dentro da caixa
const int PC_CELCIUS = 0x3112; // Temperatura da superfície do dissipador de calor
const int BT_PERCENT = 0x311A; // Porcentagem da capacidade restante da bateria
const int RMT_BT_TMP = 0x311B; // Temperatura da bateria medida pelo sensor remoto de temperatura
const int BT_RTD_PWR = 0x311D; // Tensão nominal do sistema atual. 1200, 2400, 3600, 4800 representam 12V，24V，36V，48V


// Status em tempo real (somente leitura)
const int BT_STATUS_ = 0x3200; // Status da bateria
const int CE_STATUS_ = 0x3201; // Status do equipamento de carregamento
const int DE_STATUS_ = 0x3202; // Status do equipamento de descarga


// Parâmetros Estatísticos (somente leitura)
const int MAX_PV_VOLTAGE = 0x3300; // Tensão fotovoltaica máxima de entrada hoje
const int MIN_PV_VOLTAGE = 0x3301; // Tensão fotovoltaica mínima de entrada hoje
const int MAX_BT_VOLTAGE = 0x3302; // Tensão máxima da bateria hoje
const int MIN_BT_VOLTAGE = 0x3303; // Tensão mínima da bateria hoje
const int CON_ENERGY_D_L = 0x3304; // Energia consumida hoje (Baixa), 00: 00 Limpar todos os dias   
const int CON_ENERGY_D_H = 0x3305; // Energia consumida hoje (Alta), 00: 00 Limpar todos os dias 
const int GEN_ENERGY_D_L = 0x330C; // Energia gerada hoje (Baixa), 00: 00 Limpar todos os dias.  
const int GEN_ENERGY_D_H = 0x330D; // Energia gerada hoje (Alta), 00: 00 Limpar todos os dias.
const int GEN_ENERGY_T_L = 0x3312; // Total de energia gerada (Baixa)
const int GEN_ENERGY_T_H = 0x3313; // Total de energia gerada (Alta)
const int BATTERY___TEMP = 0x331D; // Temperatura da bateria
const int AMBIENT___TEMP = 0x331E; // Temperatura Ambiente





void setup() {


  // Configuração do LoRa
  //configura os pinos como saida
  pinMode(16,OUTPUT); //RST do oled
  pinMode(25,OUTPUT);
  
  digitalWrite(16, LOW);    // reseta o OLED
  delay(50); 
  digitalWrite(16, HIGH); // enquanto o OLED estiver ligado, GPIO16 deve estar HIGH


  display.init(); //inicializa o display
  display.flipScreenVertically(); 
  display.setFont(ArialMT_Plain_10); //configura a fonte para um tamanho maior


  Serial.println("Configuração do módulo LoRa concluída.");


  delay(1500);
  display.clear(); //apaga todo o conteúdo da tela do display
  
  SPI.begin(SCK,MISO,MOSI,SS); //inicia a comunicação serial com o Lora
  LoRa.setPins(SS,RST,DI00); //configura os pinos que serão utlizados pela biblioteca (deve ser chamado antes do LoRa.begin)


  //inicializa o Lora com a frequencia específica.
  if (!LoRa.begin(BAND))
  {
    display.drawString(0, 0, "Starting LoRa failed!");
    display.display(); //mostra o conteúdo na tela
    while (1);
  }
  //indica no display que inicilizou corretamente.
  display.drawString(0, 0, "LoRa Initial success!");
  display.display(); //mostra o conteúdo na tela
  delay(1000);


  // Configurações do LoRa para maior alcance
  LoRa.setTxPower(20); // Defina o nível de potência adequado para o seu módulo LoRa
  LoRa.setSpreadingFactor(12); // Defina o Spreading Factor (fator de espalhamento) para 12
  LoRa.setSignalBandwidth(500E3); // Defina a largura de banda de sinal para 500 kHz
  LoRa.setCodingRate4(5); // Defina a taxa de codificação como 5 (4/5)
  LoRa.setPreambleLength(8); // Defina o comprimento do Preamble como 8
  LoRa.setSyncWord(0x12); // Defina a palavra de sincronização como 0x12
  LoRa.enableCrc(); // Ative o CRC (Cyclic Redundancy Check)  // Para desativar : void disableCrc(); 
  LoRa.disableInvertIQ(); // Desative a inversão de IQ


  // Defina o canal desejado (Canal 10)
  LoRa.setFrequency(915E6 + (10 * 500E3));


  Serial.println("Configuração do módulo LoRa concluída.");


  // Configuração do Modbus
  MODBUS_SERIAL.begin(MODBUS_BAUDRATE);
  node1.begin(1, MODBUS_SERIAL); // Endereço Modbus do primeiro MPPT é 1
  node2.begin(2, MODBUS_SERIAL); // Endereço Modbus do segundo MPPT é 2
}


void loop() {


  // Leitura dos valores dos dois MPPTs
  uint16_t values1[NUM_REGISTERS];
  uint16_t values2[NUM_REGISTERS];


  // Leitura dos valores do primeiro MPPT
  uint8_t result1 = node1.readInputRegisters(PV_VOLTAGE,NUM_REGISTERS);
  if (result1 == node1.ku8MBSuccess) {
    for (uint8_t i = 0; i < NUM_REGISTERS; i++) {
      values1[i] = node1.getResponseBuffer(i);
    }
  }


  // Leitura dos valores do segundo MPPT
  uint8_t result2 = node2.readInputRegisters(PV_VOLTAGE,NUM_REGISTERS);
  if (result2 == node2.ku8MBSuccess) {
    for (uint8_t i = 0; i < NUM_REGISTERS; i++) {
      values2[i] = node2.getResponseBuffer(i);
    }
  }


  // Montagem do pacote de dados a ser enviado via LoRa
  String data_node1 = "";
  data_node1 += String(1) + ",";  //primeiro dado identifica o id do mppt
  data_node1 += String(values1[0]) + ","; // Tensão do painel fotovoltaico
  data_node1 += String(values1[1]) + ","; // Corrente do painel fotovoltaico
  data_node1 += String(values1[2]) + ","; // Energia do painel fotovoltaico (parte baixa)
  data_node1 += String(values1[3]) + ","; // Energia do painel fotovoltaico (parte alta)
  data_node1 += String(values1[4]) + ","; // Tensão da bateria
  data_node1 += String(values1[5]) + ","; // Corrente de carga da bateria
  data_node1 += String(values1[6]) + ","; // Potência de carregamento da bateria (parte baixa)
  data_node1 += String(values1[7]) + ","; // Potência de carregamento da bateria (parte alta)
  data_node1 += String(values1[8]) + ","; // Tensão de carga
  data_node1 += String(values1[9]) + ","; // Corrente de carga
  data_node1 += String(values1[10]) + ","; // Potência de carga (parte baixa)
  data_node1 += String(values1[11]) + ","; // Potência de carga (parte alta)
  data_node1 += String(values1[12]) + ","; // Temperatura da bateria
  data_node1 += String(values1[13]) + ","; // Temperatura dentro da caixa
  data_node1 += String(values1[14]) + ","; // Temperatura da superfície do dissipador de calor
  data_node1 += String(values1[15]) + ","; // Porcentagem da capacidade restante da bateria
  data_node1 += String(values1[16]) + ","; // Temperatura da bateria medida pelo sensor remoto de temperatura
  data_node1 += String(values1[17]) + ","; // Tensão nominal do sistema atual
  data_node1 += String(values1[18]) + ","; // Status da bateria
  data_node1 += String(values1[19]) + ","; // Status do equipamento de carregamento
  data_node1 += String(values1[20]) + ","; // Status do equipamento de descarga
  data_node1 += String(values1[21]) + ","; // Tensão fotovoltaica máxima de entrada hoje
  data_node1 += String(values1[22]) + ","; // Tensão fotovoltaica mínima de entrada hoje
  data_node1 += String(values1[23]) + ","; // Tensão máxima da bateria hoje
  data_node1 += String(values1[24]) + ","; // Tensão mínima da bateria hoje
  data_node1 += String(values1[25]) + ","; // Energia consumida hoje (Baixa)
  data_node1 += String(values1[26]) + ","; // Energia consumida hoje (Alta)
  data_node1 += String(values1[27]) + ","; // Energia gerada hoje (Baixa)
  data_node1 += String(values1[28]) + ","; // Energia gerada hoje (Alta)
  data_node1 += String(values1[29]) + ","; // Total de energia gerada (Baixa)
  data_node1 += String(values1[30]) + ","; // Total de energia gerada (Alta)
  data_node1 += String(values1[31]) + ","; // Temperatura da bateria
  data_node1 += String(values1[32]); // Temperatura Ambiente



  String data_node2 = "";
  data_node2 += String(2) + ","; //primeiro dado identifica o id do mppt
  data_node2 += String(values2[0]) + ","; // Tensão do painel fotovoltaico
  data_node2 += String(values2[1]) + ","; // Corrente do painel fotovoltaico
  data_node2 += String(values2[2]) + ","; // Energia do painel fotovoltaico (parte baixa)
  data_node2 += String(values2[3]) + ","; // Energia do painel fotovoltaico (parte alta)
  data_node2 += String(values2[4]) + ","; // Tensão da bateria
  data_node2 += String(values2[5]) + ","; // Corrente de carga da bateria
  data_node2 += String(values2[6]) + ","; // Potência de carregamento da bateria (parte baixa)
  data_node2 += String(values2[7]) + ","; // Potência de carregamento da bateria (parte alta)
  data_node2 += String(values2[8]) + ","; // Tensão de carga
  data_node2 += String(values2[9]) + ","; // Corrente de carga
  data_node2 += String(values2[10]) + ","; // Potência de carga (parte baixa)
  data_node2 += String(values2[11]) + ","; // Potência de carga (parte alta)
  data_node2 += String(values2[12]) + ","; // Temperatura da bateria
  data_node2 += String(values2[13]) + ","; // Temperatura dentro da caixa
  data_node2 += String(values2[14]) + ","; // Temperatura da superfície do dissipador de calor
  data_node2 += String(values2[15]) + ","; // Porcentagem da capacidade restante da bateria
  data_node2 += String(values2[16]) + ","; // Temperatura da bateria medida pelo sensor remoto de temperatura
  data_node2 += String(values2[17]) + ","; // Tensão nominal do sistema atual
  data_node2 += String(values1[18]) + ","; // Status da bateria
  data_node2 += String(values1[19]) + ","; // Status do equipamento de carregamento
  data_node2 += String(values1[20]) + ","; // Status do equipamento de descarga
  data_node2 += String(values1[21]) + ","; // Tensão fotovoltaica máxima de entrada hoje
  data_node2 += String(values1[22]) + ","; // Tensão fotovoltaica mínima de entrada hoje
  data_node2 += String(values1[23]) + ","; // Tensão máxima da bateria hoje
  data_node2 += String(values1[24]) + ","; // Tensão mínima da bateria hoje
  data_node2 += String(values1[25]) + ","; // Energia consumida hoje (Baixa)
  data_node2 += String(values1[26]) + ","; // Energia consumida hoje (Alta)
  data_node2 += String(values1[27]) + ","; // Energia gerada hoje (Baixa)
  data_node2 += String(values1[28]) + ","; // Energia gerada hoje (Alta)
  data_node2 += String(values1[29]) + ","; // Total de energia gerada (Baixa)
  data_node2 += String(values1[30]) + ","; // Total de energia gerada (Alta)
  data_node2 += String(values1[31]) + ","; // Temperatura da bateria
  data_node2 += String(values1[32]); // Temperatura Ambiente
  



 //apaga o conteúdo do display
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_16);
  
  display.drawString(0, 0, "Pacotes enviados");
  display.drawString(40, 26, String(counter) );
  display.display(); //mostra o conteúdo na tela
   
  // Converte a string para um array de caracteres
  char payload_node1[256];
  data_node1.toCharArray(payload_node1, 256);


  char payload_node2[256];
  data_node2.toCharArray(payload_node2, 256);


  // Envia o pacote LoRa com os dados MPPT1
  //beginPacket : abre um pacote para adicionarmos os dados para envio
  LoRa.beginPacket();
  //print: adiciona os dados no pacote
  LoRa.print(payload_node1);
  LoRa.print(counter);
  //endPacket : fecha o pacote e envia
  LoRa.endPacket(); //retorno= 1:sucesso | 0: falha
  
  //incrementa uma unidade no contador
  counter++;


  digitalWrite(25, HIGH);   // liga o LED indicativo
  delay(500);                       // aguarda 500ms
  digitalWrite(25, LOW);    // desliga o LED indicativo
  delay(500);


  // Envia o pacote LoRa com os dados MPPT2
  //beginPacket : abre um pacote para adicionarmos os dados para envio
  LoRa.beginPacket();
  //print: adiciona os dados no pacote
  LoRa.print(payload_node2);
  LoRa.print(counter);
  //endPacket : fecha o pacote e envia
  LoRa.endPacket(); //retorno= 1:sucesso | 0: falha


  //incrementa uma unidade no contador
  counter++;


  digitalWrite(25, HIGH);   // liga o LED indicativo
  delay(500);                       // aguarda 500ms
  digitalWrite(25, LOW);    // desliga o LED indicativo
  delay(500); 


  delay(5000);// Aguarda 5 segundos antes de enviar o proximo pacote de dados
}



