//#include <SoftwareSerial.h> //usado no Arduino e no NodeMCU, ESP32 não é compatível
//#include <Thermistor.h> // NTC 10k, mas não está sendo usuado atualmente
#include <OneWire.h> // NTC digital
#include <DallasTemperature.h> // NTC digital
#include "WiFi.h"
#include "ThingSpeak.h"
#include <PZEM004Tv30.h> // biblioteca do Wattímetro, já com a compatibilidade pra software serial ou hardware serial 
#include "Wire.h" // biblioteca pra usar a comunicação I2C
#include <driver/adc.h> // biblioteca para comandos das portas ADC da ESP32
#include "HardwareSerial.h" //somente para a ESP32
#include <Adafruit_ADS1015.h> // conversor ADC

// conexões wifi e thingspeak
const char* myWriteAPIKey = "1J8YCXRMT94MVBXH";     // Write API key do canal ThingSpeak
unsigned int canalNTCePressao = 1202330; // canal dos NTCs e pressão
unsigned int canalConsumoeVibracao = 1207951; // canal do Wattímetro e acelerômetro
const char* ssid =  "VIVOFIBRA-1650";     // Nome da rede WIFI
const char* pass =  "6343CC203B"; // Senha do WIFI
WiFiClient  client;
const char* server = "api.thingspeak.com";

String dados = "";
unsigned int flag = 0;
unsigned int contador = 0;
float tempoAnterior = 0;
float tempoAtual = 0;
float tempo = 0;

// Dados sensores NTC digitais ////////
const int oneWireBus = 23; //porta que os NTCs digitais estão conectados
OneWire oneWire(oneWireBus);
DallasTemperature sensortemp(&oneWire);
int ndispositivos = 0;

// variável sensores
float temp1; // sucção
float temp2; // descarga
float temp3; // linha líquido / filtro secador
float temp4; // entrada evaporador
float temp5; // saída evaporador
float temp6; // 
float temp7; //
float temp8; // ambiente
float temp9; // ambiente
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
  return;
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

  ThingSpeak.setField(7, pressaoAlta);
  //pressaoAlta = pressaoAlta * 14.504; //convertendo pra psi

  ThingSpeak.setField(8, pressaoBaixa);

  return;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void consumo() {

  // Leitura do consumo energético
  V = pzem.voltage();
  //ThingSpeak.setField(1, volt);

  I = pzem.current();
  //ThingSpeak.setField(2, cur);

  W = pzem.power();
  //  ThingSpeak.setField(8, W);

  Wh = pzem.energy();
  //ThingSpeak.setField(4, ener);

  Freq = pzem.frequency();
  //ThingSpeak.setField(5, freq);

  FP = pzem.pf();
  //ThingSpeak.setField(6, pf);

  return;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// testes enviar dados pro ThingSpeak do canal dos sensores ///////////////////////////////////////
void enviar_ThinkSpeak() {

  // set the status
  // ThingSpeak.setStatus(myStatus);

  // write to the ThingSpeak channel
  int x = ThingSpeak.writeFields(canalNTCePressao, myWriteAPIKey);
  if (x == 200) {
    //Serial.println("Canal sensores NTC e pressão atualizado com sucesso.");
  }
  else {
    //Serial.println("Problema atualizando canal. HTTP error code " + String(x));
  }


  ///////////////////////////////////////////////////////////////////////////////////////////////////
  // testes enviar dados pro ThingSpeak do canal do consumo e acelerômetro //////////////////////////

  // set the status
  //ThingSpeak.setStatus(myStatus);

  // write to the ThingSpeak channel
  //   x = ThingSpeak.writeFields(canalConsumoeVibracao, myWriteAPIKey);
  //   if(x == 200){
  //     Serial.println("Canal consumo e  com sucesso.");
  //   }
  //   else{
  //     Serial.println("Problema atualizando canal. HTTP error code " + String(x));
  //   }

  return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// setup ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {

  Serial.begin(9600);

  WiFi.mode(WIFI_STA); //Esta linha esconde a visualização do ESP como hotspot wifi
  ThingSpeak.begin(client);

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

  Serial.println("Flag;Tempo;T1;T2;T3;T4;T5;T6;T7;T8;T9;pressaoBaixa;pressaoAlta;Watt;Tensao;Corrente;Fator Pot;Watt hora");

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// loop ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void loop() {

  verificarStatusWifi(); //conecta e reconecta no wifi

  dados = dados + String(flag) + ";";
  
  tempoAtual = millis(); // pra saber a média de tempo que leva entre as leituras
  tempo = tempoAtual / 1000;

  dados = dados + String(tempo) + ";";

  leituraNTC_digitais(); //leitura sensores digitais
  leituraPressao(); //leitura transdutores de pressão
  consumo(); //wattímetro

  dados = dados + String(temp1) + ";" + String(temp2) + ";" + String(temp3) + ";" + String(temp4) + ";" + String(temp5) +
          ";" + String(temp6) + ";" + String(temp7) + ";" + String(temp8) + ";" + String(temp9) + ";" + String(pressaoBaixa) + 
          ";" + String(pressaoAlta) + ";" + String(W) + ";" + String(V) + ";" + String(I) + ";" + String(FP) + ";" + String(Wh);

  Serial.println(dados);

  //enviar_ThinkSpeak();

  dados = "";

 
  //Serial.println(tempoAtual); // pra saber a média de tempo que leva entre as leituras
  //delay(1000);

}
// fim loop ///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
