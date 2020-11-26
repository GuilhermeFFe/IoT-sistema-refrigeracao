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
//const char* server = "api.thingspeak.com";

String dados = "";
unsigned int flag = 0;
unsigned int contador = 0;

// Dados sensores NTC digitais ////////
const int oneWireBus = 13; //porta que os NTCs digitais estão conectados
OneWire oneWire(oneWireBus);
DallasTemperature sensortemp(&oneWire);
int ndispositivos = 0;
float mediaD; // media digitais
float mediaA; //media dos analogicos pra ver a diferença contra os digitais. Teste de validação e verificar se a diferença é linear

// Dados sensores NTC analógicos
//Thermistor tempana0(A0); //VARIÁVEL DO TIPO THERMISTOR, INDICANDO O PINO ANALÓGICO (A2) EM QUE O TERMISTOR ESTÁ CONECTADO
// valores NTCs analógicos
const double VCC = 3.31; // NodeMCU on board 3.3v vcc
const double R2 = 10000; // 10k ohm series resistor
const double adc_resolution = 4095; // 10-bit adc
const double A = 0.001129148; // thermistor equation parameters
const double B = 0.000234125;
const double C = 0.0000000876741;
float Vout, Rth, tempAnalogico, adc_value;
int canalMultiplex = 0;

// variável sensores
float Temp1; // GPIO12   linha de sucção
float Temp2; // A0(14)   descarga
float Temp3; // GPIO13   D1 - linha de líquido
float Temp4; // A0(15)   entrada evaporador
float Temp5; // D1       D2 - ambiente
float Temp6; // D1       D3 - meio do condensador
float pressaoAlta;
float pressaoBaixa;
// Wattímetro, consumo do compressor
float W;   // D7(RX) D8(TX) Consumo
float V;   // D7(RX) D8(TX) Tensão
float I;   // D7(RX) D8(TX) Corrente
float FP;  // D7(RX) D8(TX) Fator de Potência
float Freq;// D7(RX) D8(TX) Frequência da rede
float Wh;  // D7(RX) D8(TX) Watt hora

//conversor para os sensores de pressao
Adafruit_ADS1115 ads(0x48);  // cria instância do conversor analogico digital ADC */


PZEM004Tv30 pzem(&Serial2); //usa o Serial2 do hardwareserial, pinos instânciados no construtor, padrão, 16 RX e 17 TX

  //  adc1_config_width(ADC_WIDTH_BIT_10);//Configura a resolucao
  //adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_0);//Configura a atenuacao
  
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// setup ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {
  pinMode(36, INPUT); // NTC 10k
  pinMode(39, INPUT); // NTC 10k
  pinMode(34, INPUT); // NTC 10k

  Serial.begin(9600);
  contador = contador + 1; // incrementa o contador pra saber a ordem das leituras. Usado pq o arduino/esp não tem relógio;

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

  Serial.println("Flag;Contador;T1;T2;T3;T4;T5;T6;pressaoBaixa;pressaoAlta;Watt;Tensao;Corrente;Fator Pot;Watt hora");
  //T1            float   °C    linha de sucção pouco após o evaporador, aprox. a meio caminho entre saída do evaporador e sucção do compressor A0(13)
  //T2            float   °C    temperatura de descarga, bem próximo do compressor A0(14)
  //T3            float   °C    linha de líquido bem entre a saída do condensador e antes do filtro secador D1
  //T4            float   °C    entrada evaporador mais próximo possível da saída do tubo capilar A0(15)
  //T5            float   °C    temperatura ambiente  D2
  //pressaoBaixa  float   bar   evaporação bem na saída do evaporador
  //pressaoAlta   float   bar   condensação pouco antes da entrada do filtro secador
  //W            W             potência instantânea consumida pelo compressor
  //dados extras:
  //T6            float   °C    condensador meio do condensador D3
  //V             V             tensão aplicada ao compressor
  //I             A             corrente consumida pelo compressor
  //FP            float    -    fator de potência do compressor
  //Wh                    W.h   consumo total do compressor
  //Freq          float   Hertz frequência da rede

  
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// loop ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void loop() {
  
  dados = dados + String(flag) + ";";
  contador = contador + 1;
  dados = dados + String(contador) + ";";


    verificarStatusWifi(); //conecta e reconecta no wifi
 
  leituraNTC_digitais(); //leitura sensores digitais
  leituraNTC_analogicos(); //leitura sensores analógicos
  leituraPressao(); //leitura transdutores de pressão
  consumo(); //wattímetro

  dados = dados + String(Temp1) + ";" + String(Temp2) + ";" + String(Temp3) + ";" + String(Temp4) + ";" + String(Temp5) +
          ";" + String(Temp6) + ";" + String(pressaoBaixa) + ";" + String(pressaoAlta) + ";" + String(W) + ";" +
          String(V) + ";" + String(I) + ";" + String(FP) + ";" + String(Wh);

  Serial.println(dados);

  //Serial.print("Média digital: ");
  //Serial.println(mediaD);
  //Serial.print("Média analógico: ");
  //Serial.println(mediaA);
  //Serial.print("Diferença dos NTCs (D-A): ");
  //mediaD = mediaD - mediaA;
  //Serial.println(mediaD);
  //Serial.print("Pressão baixa: ");
  //Serial.println(pressaoBaixa);
  //Serial.print("Pressão alta: ");
  //Serial.println(pressaoAlta);
  
  //mediaD = 0; //media dos NTCs digitais
  //mediaA = 0; //media dos NTCs analogicos
  
  //enviar_ThinkSpeak();
   
  dados = "";

  //delay(1000);

}
// fim loop ///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

// no arduino, double e float possuem o mesmo tamanho /////////////////////////////////////////////
float calcTempAnalogico(int pino) {

  adc_value = 0;
  int i = 0;
  while (i < 50) {
    adc_value = adc_value + analogRead(pino);
    i++;
  }
  adc_value = adc_value / 50;
  //Serial.println(adc_value);
  //adc_value = adc_value * 0.925; // correcao no nodemCU
  adc_value = adc_value * 1.05;
  Vout = (adc_value * VCC) / adc_resolution;
  Rth = (VCC * R2 / Vout) - R2;

  tempAnalogico = (1 / (A + (B * log(Rth)) + (C * pow((log(Rth)), 3))));  // Temperatura em kelvin
  tempAnalogico = tempAnalogico - 273.15;  // Temperatura em graus Celsius
  return tempAnalogico;
}

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

  sensortemp.requestTemperatures();

  Temp3 = sensortemp.getTempCByIndex(0);
    ThingSpeak.setField(3, Temp3); //canal principal, campo 3
  //dados = dados + String(T3) + ";";

  Temp5 = sensortemp.getTempCByIndex(1);
    ThingSpeak.setField(5, Temp5); // canal principal, campo 5
  //dados = dados + String(T5) + ";";

  Temp6 = sensortemp.getTempCByIndex(2);
  ThingSpeak.setField(6, Temp6); //canal secundario
  //dados = dados + String(T6) + ";";

  mediaD = (Temp3 + Temp5 + Temp6) / 3;
  return;
}

// analogico testes //////////////////////////////////////////////////////////////////////////////////////////////////
// analogico testes //////////////////////////////////////////////////////////////////////////////////////////////////
void leituraNTC_analogicos() {

  Temp1 = calcTempAnalogico(36); //ADC1_CHANNEL_0
  Temp2 = calcTempAnalogico(39); //ADC1_CHANNEL_3
  Temp4 = calcTempAnalogico(34); //ADC1_CHANNEL_6

  ThingSpeak.setField(1, Temp1);
  ThingSpeak.setField(2, Temp2);
  ThingSpeak.setField(4, Temp4);
  //Serial.print("Teste função adc1 get voltage: ");
  //int teste = adc1_get_voltage(ADC1_CHANNEL_0);
  //int teste = adc1_get_raw(ADC1_CHANNEL_0);
  //Serial.println(teste);

  mediaA = (Temp1 + Temp2 + Temp4) / 3;

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
  pressaoAlta = (pressaoAlta * 0.1875) / 1000;
  pressaoAlta = (pressaoAlta * 30 - 19.8) / 2.64;

  pressaoBaixa = 0;
  i = 0;
  while (i < 50) {
    pressaoBaixa = pressaoBaixa + ads.readADC_SingleEnded(1);
    i++;
  }
  pressaoBaixa = pressaoBaixa / 50;
  pressaoBaixa = (pressaoBaixa * 0.1875) / 1000;
  pressaoBaixa = (pressaoBaixa * 10 - 6.6) / 2.64;

  //  ThingSpeak.setField(7, pressaoAlta);
  //pressaoAlta = pressaoAlta * 14.504; //convertendo pra psi

  //  ThingSpeak.setField(6, pressaoBaixa);

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
void enviar_ThinkSpeak(){

  // set the status
 // ThingSpeak.setStatus(myStatus);

  // write to the ThingSpeak channel
     int x = ThingSpeak.writeFields(canalNTCePressao, myWriteAPIKey);
     if(x == 200){
       Serial.println("Canal sensores NTC e pressão atualizado com sucesso.");
     }
     else{
       Serial.println("Problema atualizando canal. HTTP error code " + String(x));
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
