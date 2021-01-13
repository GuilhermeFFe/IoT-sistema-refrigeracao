#include <OneWire.h> // NTC digital
#include <DallasTemperature.h> // NTC digital
#include <PZEM004Tv30.h> // biblioteca do Wattímetro, já com a compatibilidade pra software serial ou hardware serial 
#include "Wire.h" // biblioteca pra usar a comunicação I2C
#include <driver/adc.h> // biblioteca para comandos das portas ADC da ESP32
#include "HardwareSerial.h" //somente para a ESP32
#include <Adafruit_ADS1015.h> // conversor ADC
#include "cactus_io_BME280_I2C.h"
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <Update.h>
//#include <SPI.h>

// conexões wifi
const char* ssid =  "VIVOFIBRA-1650";     // Nome da rede WIFI
const char* pass =  "6343CC203B"; // Senha do WIFI
const char* mqtt_server = "192.168.15.73"; // IP local da Raspberry

//Parâmetros de rede
//IPAddress local_ip(192, 168, 0, 121);
//IPAddress gateway(192, 168, 0, 1);
//IPAddress subnet(255, 255, 255, 0);
const uint32_t PORTA = 5000; //A porta que será utilizada (padrão 80)

// Algumas informações que podem ser interessantes
const uint32_t chipID = (uint32_t)(ESP.getEfuseMac() >> 32); // um ID exclusivo do Chip...
const String CHIP_ID = "<p> Chip ID: " + String(chipID) + "</p>"; // montado para ser usado no HTML
const String VERSION = "<p> Versão: 0.9 </p>"; // Exemplo de um controle de versão

//Informações interessantes agrupadas
const String INFOS = VERSION + CHIP_ID;

//Sinalizador de autorização do OTA
boolean OTA_AUTORIZADO = false;

//inicia o servidor na porta selecionada
//aqui testamos na porta 5000, ao invés da 80 padrão
WebServer server(PORTA);

//Páginas HTML utilizadas no procedimento OTA
String verifica = "<!DOCTYPE html><html><head><title>ESP32 webOTA</title><meta charset='UTF-8'></head><body><h1>ESP32 webOTA</h1><h2>Digite a chave de verificação.<p>Clique em ok para continuar. . .</p></h2>" + INFOS + "<form method='POST' action='/avalia 'enctype='multipart/form-data'> <p><label>Autorização: </label><input type='text' name='autorizacao'></p><input type='submit' value='Ok'></form></body></html>";
String serverIndex = "<!DOCTYPE html><html><head><title>ESP32 webOTA</title><meta charset='UTF-8'></head><body><h1>ESP32 webOTA</h1><h2>Selecione o arquivo para a atualização e clique em atualizar.</h2>" + INFOS + "<form method='POST' action='/update' enctype='multipart/form-data'><p><input type='file' name='update'></p><p><input type='submit' value='Atualizar'></p></form></body></html>";
String Resultado_Ok = "<!DOCTYPE html><html><head><title>ESP32 webOTA</title><meta charset='UTF-8'></head><body><h1>ESP32 webOTA</h1><h2>Atualização bem sucedida!</h2>" + INFOS + "</body></html>";
String Resultado_Falha = "<!DOCTYPE html><html><head><title>ESP32 webOTA</title><meta charset='UTF-8'></head><body><h1>ESP32 webOTA</h1><h2>Falha durante a atualização. A versão anterior será recarregado.</h2>" + INFOS + "</body></html>";

WiFiClient  WifiClient;
PubSubClient client(WifiClient);

// Dados pro MQTT 
unsigned long tempoMsg = 0;
#define MSG_BUFFER_SIZE  (500)
char msg[MSG_BUFFER_SIZE];

//testando JSON
const int capacity = JSON_OBJECT_SIZE(MSG_BUFFER_SIZE);
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

unsigned int tempoAtual = 0;
unsigned int tempoAnterior = 0;

void verificarStatusWifi() {

  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    while (WiFi.status() != WL_CONNECTED) {
      WiFi.begin(ssid, pass); // Conectado numa rede WPA/WPA2. Mudar essa linha se usar uma rede aberta ou WEP.
      Serial.print(".");
      randomSeed(micros()); // pra gerar um ID pro MQTT
      delay(1000);
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
    }
    Serial.println("\nConnected.");
  }

  
  return;
}

void atualizacao_OTA(){

  if (WiFi.status() == WL_CONNECTED){ //aguarda a conexão
  
    //atende uma solicitação para a raiz
    // e devolve a página 'verifica'
    server.on("/", HTTP_GET, [](){ //atende uma solicitação para a raiz   
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", verifica);
    });

    //atende uma solicitação para a página avalia
    server.on("/avalia", HTTP_POST, [] (){
      Serial.println("Em server.on /avalia: args= " + String(server.arg("autorizacao"))); //somente para debug

      if (server.arg("autorizacao") != "projetoIFSC"){// confere se o dado de autorização atende a avaliação
      
        //se não atende, serve a página indicando uma falha
        server.sendHeader("Connection", "close");
        server.send(200, "text/html", Resultado_Falha);
        //ESP.restart();
      }
      else{      
        //se atende, solicita a página de índice do servidor
        // e sinaliza que o OTA está autorizado
        OTA_AUTORIZADO = true;
        server.sendHeader("Connection", "close");
        server.send(200, "text/html", serverIndex);
      }
    });

    //serve a página de indice do servidor
    //para seleção do arquivo
    server.on("/serverIndex", HTTP_GET, [](){
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", serverIndex);
    });

    //tenta iniciar a atualização . . .
    server.on("/update", HTTP_POST, [](){
      //verifica se a autorização é false.
      //Se for falsa, serve a página de erro e cancela o processo.
      if (OTA_AUTORIZADO == false){
        server.sendHeader("Connection", "close");
        server.send(200, "text/html", Resultado_Falha);
        return;
      }
      //Serve uma página final que depende do resultado da atualização
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", (Update.hasError()) ? Resultado_Falha : Resultado_Ok);
      delay(1000);
      ESP.restart();
    }, [](){
      //Mas estiver autorizado, inicia a atualização
      HTTPUpload& upload = server.upload();
      if (upload.status == UPLOAD_FILE_START){
        Serial.setDebugOutput(true);
        Serial.printf("Atualizando: %s\n", upload.filename.c_str());
        if (!Update.begin()){
          //se a atualização não iniciar, envia para serial mensagem de erro.
          Update.printError(Serial);
        }
      }
      else if (upload.status == UPLOAD_FILE_WRITE){
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize){
          //se não conseguiu escrever o arquivo, envia erro para serial
          Update.printError(Serial);
        }
      }
      else if (upload.status == UPLOAD_FILE_END){
        if (Update.end(true)){
          //se finalizou a atualização, envia mensagem para a serial informando
          Serial.printf("Atualização bem sucedida! %u\nReiniciando...\n", upload.totalSize);
        }
        else{
          //se não finalizou a atualização, envia o erro para a serial.
          Update.printError(Serial);
        }
        Serial.setDebugOutput(false);
      }
      else{
        //se não conseguiu identificar a falha no processo, envia uma mensagem para a serial
        Serial.printf("Atualização falhou inesperadamente! (possivelmente a conexão foi perdida.): status=%d\n", upload.status);
      }
    });

    server.begin(); //inicia o servidor

    Serial.println(INFOS); //envia as informações armazenadas em INFOS, para debug

    //Envia para a serial o IP atual do ESP
    Serial.print("Servidor em: ");
    Serial.println( WiFi.localIP().toString() + ":" + PORTA);
  }
  else{
    //avisa se não onseguir conectar no WiFi
    Serial.println("Falha ao conectar ao WiFi.");
  }
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

  Serial.begin(115200);

  
  WiFi.mode(WIFI_STA); // Esta linha esconde a visualização do ESP como hotspot wifi
  //WiFi.mode(WIFI_AP_STA); // Comfigura o ESP32 como ponto de acesso e estação

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

  atualizacao_OTA(); //sobe a página html pra atualizacao via OTA

  client.setBufferSize(MSG_BUFFER_SIZE); //setar o tamanho do buffer do payload mqtt
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// loop ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void loop() {

  tempoAnterior = millis();
  
  verificarStatusWifi(); //conecta e reconecta no wifi

  //manipula clientes conectados
  server.handleClient();

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

  // condição caso o wattímetro esteja desligado, substituí o nan (null no json recebido pelo node-red) por 0 pra poder armazenar no influxDB
  if(isnan(W)) W = 0;
  if(isnan(V)) V = 0;
  if(isnan(I)) I = 0;
  if(isnan(FP)) FP = 0;
  if(isnan(Wh)) Wh = 0;
  
  // JSON
  doc["motor"] = false;
  doc["vent condensador"] = false;
  doc["vent evaporador"] = false;
  doc["succao"] = temp1;
  doc["descarga"] = temp2;
  doc["filtro secador"] = temp3;
  doc["entrada evaporador"] = temp4;
  doc["saida evaporador"] = temp5;
  doc["linha liquido"] = temp6;
  doc["meio evaporador"] = temp7;
  doc["ambiente"] = temp8; // T8 - ambiente
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

  tempoAtual = millis();
  tempoAtual = tempoAtual - tempoAnterior;
  Serial.println(tempoAtual); // pra saber a média de tempo que leva entre as leituras
  //delay(1000);

}
