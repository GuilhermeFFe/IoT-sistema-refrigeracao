#include <OneWire.h> // NTC digital
#include <DallasTemperature.h> // NTC digital
#include "WiFi.h"
#include <PZEM004Tv30.h> // biblioteca do Wattímetro, já com a compatibilidade pra software serial ou hardware serial 
#include "Wire.h" // biblioteca pra usar a comunicação I2C
#include <driver/adc.h> // biblioteca para comandos das portas ADC da ESP32
#include "HardwareSerial.h" //somente para a ESP32
#include <Adafruit_ADS1015.h> // conversor ADC
#include "cactus_io_BME280_I2C.h"
#include <PubSubClient.h>
#include <ArduinoJson.h>
//#include <SPI.h>

// conexões wifi
const char* ssid =  "VIVOFIBRA-1650";     // Nome da rede WIFI
const char* pass =  "6343CC203B"; // Senha do WIFI
const char* mqtt_server = "192.168.15.73"; // IP local da Raspberry

WiFiClient  WifiClient;
PubSubClient client(WifiClient);

// Dados pro MQTT 
unsigned long tempoMsg = 0;
#define MSG_BUFFER_SIZE  (384)
char msg[MSG_BUFFER_SIZE];

//testando JSON
const int capacity = JSON_OBJECT_SIZE(384);
StaticJsonDocument<capacity> doc;
char dados[capacity];

// Dados sensores NTC digitais ////////
const int oneWireBus = 23; //porta que os NTCs digitais estão conectados
OneWire oneWire(oneWireBus);
DallasTemperature sensortemp(&oneWire);
int ndispositivos = 0;

// variável sensores
float temp1; // sucção
float temp2; // descarga
float temp3; // filtro secador
float temp4; // entrada evaporador
float temp5; // saída evaporador
float temp6; // linha líquido
float temp7; // meio evaporador
float temp8; // ambiente
float temp9; // compressor
float pressaoAlta;
float pressaoBaixa;

// Wattímetro, consumo do compressor
float W;   // Consumo
float V;   // Tensão
float I;   // Corrente
float FP;  // Fator de Potência
float Freq;// Frequência da rede
float Wh;  // Watt hora

//conversor para os sensores de pressao
Adafruit_ADS1115 ads(0x48);  // cria instância do conversor analogico digital ADC */

PZEM004Tv30 pzem(&Serial2); //usa o Serial2 do hardwareserial, pinos instânciados no construtor, padrão, 16 RX e 17 TX

//unsigned int tempoAtual = 0;
//unsigned int tempoAnterior = 0;

void verificarStatusWifi() {

  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    while (WiFi.status() != WL_CONNECTED) {
      WiFi.begin(ssid, pass); // Conectado numa rede WPA/WPA2. Mudar essa linha se usar uma rede aberta ou WEP.
      Serial.print(".");
      delay(5000);
    }
    Serial.println("\nConnected.");
  }

  randomSeed(micros()); // pra gerar um ID pro MQTT
  
  return;
}

// receber mensagens do MQTT, subscriber
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

}

// reconecta no broker mqtt
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// leituras dos digitais ////////////////////////////////////////////////////////////////////////////////////////////
// leituras dos digitais ////////////////////////////////////////////////////////////////////////////////////////////
void leituraNTC_digitais() {
  // 9 sensores
  sensortemp.requestTemperatures();

  temp1 = sensortemp.getTempCByIndex(0);
  //Serial.print("1: ");
  //Serial.println(temp1);
  temp2 = sensortemp.getTempCByIndex(1);
  //Serial.print("2: ");
  //Serial.println(temp2);
  temp3 = sensortemp.getTempCByIndex(2);
  //Serial.print("3: ");
  //Serial.println(temp3);
  temp4 = sensortemp.getTempCByIndex(3);
  //Serial.print("4: ");
  //Serial.println(temp4);
  temp5 = sensortemp.getTempCByIndex(4);
  //Serial.print("5: ");
  //Serial.println(temp5);
  temp6 = sensortemp.getTempCByIndex(5);
  //Serial.print("6: ");
  //Serial.println(temp6);
  temp7 = sensortemp.getTempCByIndex(6);
  //Serial.print("7: ");
  //Serial.println(temp7);
  temp8 = sensortemp.getTempCByIndex(7);
  //Serial.print("8: ");
  //Serial.println(temp8);
  temp9 = sensortemp.getTempCByIndex(8);
  //Serial.print("9: ");
  //Serial.println(temp9);

  return;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// sensores de pressão ////////////////////////////////////////////////////////////////////////////
void leituraPressao() {

  pressaoAlta = 0;
  int i = 0;
  while (i < 50) {
    pressaoAlta = pressaoAlta + ads.readADC_SingleEnded(0);
    i++;
  }
  pressaoAlta = pressaoAlta / 50; // tira a média das 50 leituras
  pressaoAlta = (pressaoAlta * 0.1875) / 1000; // obter tensão
  pressaoAlta = (pressaoAlta * 30 - 19.8) / 2.64; // fórmula obtida fazendo uma matriz com 0 bar a 30 bar e 0,66V a 3.3V

  pressaoBaixa = 0;
  i = 0;
  while (i < 50) {
    pressaoBaixa = pressaoBaixa + ads.readADC_SingleEnded(3);
    i++;
  }
  pressaoBaixa = pressaoBaixa / 50;
  pressaoBaixa = (pressaoBaixa * 0.1875) / 1000; // obter tensão
  pressaoBaixa = (pressaoBaixa * 10 - 6.6) / 2.64; // fórmula obtida fazendo uma matriz com 0 bar a 10 bar e 0,66V a 3.3V

  //pressaoAlta = pressaoAlta * 14.504; //convertendo pra psi

  return;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Leitura do consumo energético
void consumo() { 
  
  V = pzem.voltage();

  I = pzem.current();

  W = pzem.power();

  Wh = pzem.energy();

  Freq = pzem.frequency();

  FP = pzem.pf();

  return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// setup ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {

  Serial.begin(9600);

  WiFi.mode(WIFI_STA); //Esta linha esconde a visualização do ESP como hotspot wifi

  // configuração e checagem dos NTC digitais
  sensortemp.begin();
  Serial.println("Localizando Dispositivos ...");
  Serial.print("Encontrados ");
  ndispositivos = sensortemp.getDeviceCount();
  Serial.print(ndispositivos, DEC);
  Serial.println(" dispositivos.");
  Serial.println("");

  ads.begin(); // inicializando o conversor ADS

  pzem.resetEnergy();
  pzem.setAddress(0x42);

  verificarStatusWifi(); //conecta e reconecta no wifi

  client.setBufferSize(385); //setar o tamanho do buffer do payload mqtt
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// loop ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void loop() {

  verificarStatusWifi(); //conecta e reconecta no wifi

  // conectar no broker mqtt
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  leituraNTC_digitais(); // leitura sensores digitais
  leituraPressao(); // leitura transdutores de pressão
  consumo(); // leitura wattímetro

  //dados = dados + String(temp1) + ";" + String(temp2) + ";" + String(temp3) + ";" + String(temp4) + ";" + String(temp5) +
  //        ";" + String(temp6) + ";" + String(temp7) + ";" + String(temp8) + ";" + String(temp9) + ";" + String(pressaoBaixa) + 
  //        ";" + String(pressaoAlta) + ";" + String(W) + ";" + String(V) + ";" + String(I) + ";" + String(FP) + ";" + String(Wh);

  doc["compressor"] = false;
  doc["vent condensador"] = false;
  doc["vent evaporador"] = false;
  doc["succao"] = temp1;
  doc["descarga"] = temp2;
  doc["filtro secador"] = temp3;
  doc["entrada evaporador"] = temp4;
  doc["saida evaporador"] = temp5;
  doc["linha liquido"] = temp6;
  doc["meio evaporador"] = temp7;
  doc["ambiente"] = temp8;
  doc["compressor"] = temp9;
  doc["pressao baixa"] = pressaoBaixa;
  doc["pressao alta"] = pressaoAlta;
  doc["potencia"] = W;
  doc["tensao"] = V;
  doc["corrente"] = I;
  doc["fator potencia"] = FP;
  doc["watt hora"] = Wh;
  
  serializeJson(doc, dados);

  //serializeJson(doc, Serial);

  client.publish("dados_refrig", dados); // envia o JSON pro tópico dados_refrig no broker MQTT no node-red
 
  //Serial.println(tempoAtual); // pra saber a média de tempo que leva entre as leituras
  //delay(1000);

}
