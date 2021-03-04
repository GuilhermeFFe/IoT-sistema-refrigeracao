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
//const char* mqtt_server = "test.mosquitto.org"; // IP Guilherme da Raspberry


//Parâmetros de rede
//IPAddress local_ip(192, 168, 0, 121);
//IPAddress gateway(192, 168, 0, 1);
//IPAddress subnet(255, 255, 255, 0);
const uint32_t PORTA = 5000; //A porta que será utilizada (padrão 80)
//inicia o servidor na porta selecionada
//aqui testamos na porta 5000, ao invés da 80 padrão
WebServer server(PORTA);

// Algumas informações que podem ser interessantes
const uint32_t chipID = (uint32_t)(ESP.getEfuseMac() >> 32); // um ID exclusivo do Chip...
const String CHIP_ID = "<p> Chip ID: " + String(chipID) + "</p>"; // montado para ser usado no HTML
const String VERSION = "<p> Versão 1.1 fórmulas teóricas otimizadas </p>"; // Exemplo de um controle de versão

//Informações interessantes agrupadas
const String INFOS = VERSION + CHIP_ID;

//Sinalizador de autorização do OTA
boolean OTA_AUTORIZADO = false;



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

//testando JSON
const int capacity = JSON_OBJECT_SIZE(MSG_BUFFER_SIZE);
//StaticJsonDocument<capacity> doc;
DynamicJsonDocument doc(capacity);
DynamicJsonDocument comandos(100);
char dados[capacity]; // dados serializados pra enviar pro broker mqtt
char comandos_recebidos[100]; // dados que serão recebidos do broker mqtt pros comandos do compressor e exaustores/coolers/ventiladores

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

float Temp_Ev_calc;
float Temp_Cd_calc;
float Pot_teorica;

// Wattímetro, consumo do compressor
float W;   // Consumo
float V;   // Tensão
float I;   // Corrente
float FP;  // Fator de Potência
float Freq;// Frequência da rede
float Wh;  // Watt hora

//conversor para os sensores de pressao
Adafruit_ADS1115 ads(0x48);  // cria instância do conversor analogico digital ADC */

// criando objeto do wattímetro
PZEM004Tv30 pzem(&Serial2); //usa o Serial2 do hardwareserial, pinos instânciados no construtor, padrão, 16 RX e 17 TX

unsigned int tempoLoopDesligado = 600000;
unsigned int tempoAtual = 0;
unsigned int tempoAnterior = 0;


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// inicio funções
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

void atualizacao_OTA() {

  if (WiFi.status() == WL_CONNECTED) { //aguarda a conexão

    //atende uma solicitação para a raiz
    // e devolve a página 'verifica'
    server.on("/", HTTP_GET, []() { //atende uma solicitação para a raiz
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", verifica);
    });

    //atende uma solicitação para a página avalia
    server.on("/avalia", HTTP_POST, [] () {
      Serial.println("Em server.on /avalia: args= " + String(server.arg("autorizacao"))); //somente para debug

      if (server.arg("autorizacao") != "projetoIFSC") { // confere se o dado de autorização atende a avaliação

        //se não atende, serve a página indicando uma falha
        server.sendHeader("Connection", "close");
        server.send(200, "text/html", Resultado_Falha);
        //ESP.restart();
      }
      else {
        //se atende, solicita a página de índice do servidor
        // e sinaliza que o OTA está autorizado
        OTA_AUTORIZADO = true;
        server.sendHeader("Connection", "close");
        server.send(200, "text/html", serverIndex);
      }
    });

    //serve a página de indice do servidor
    //para seleção do arquivo
    server.on("/serverIndex", HTTP_GET, []() {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", serverIndex);
    });

    //tenta iniciar a atualização . . .
    server.on("/update", HTTP_POST, []() {
      //verifica se a autorização é false.
      //Se for falsa, serve a página de erro e cancela o processo.
      if (OTA_AUTORIZADO == false) {
        server.sendHeader("Connection", "close");
        server.send(200, "text/html", Resultado_Falha);
        return;
      }
      //Serve uma página final que depende do resultado da atualização
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", (Update.hasError()) ? Resultado_Falha : Resultado_Ok);
      delay(1000);
      ESP.restart();
    }, []() {
      //Mas estiver autorizado, inicia a atualização
      HTTPUpload& upload = server.upload();
      if (upload.status == UPLOAD_FILE_START) {
        Serial.setDebugOutput(true);
        Serial.printf("Atualizando: %s\n", upload.filename.c_str());
        if (!Update.begin()) {
          //se a atualização não iniciar, envia para serial mensagem de erro.
          Update.printError(Serial);
        }
      }
      else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
          //se não conseguiu escrever o arquivo, envia erro para serial
          Update.printError(Serial);
        }
      }
      else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) {
          //se finalizou a atualização, envia mensagem para a serial informando
          Serial.printf("Atualização bem sucedida! %u\nReiniciando...\n", upload.totalSize);
        }
        else {
          //se não finalizou a atualização, envia o erro para a serial.
          Update.printError(Serial);
        }
        Serial.setDebugOutput(false);
      }
      else {
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
  else {
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
    comandos_recebidos[i] = payload[i];
    Serial.print((char)payload[i]);
  }
  Serial.println();

  deserializeJson(comandos, comandos_recebidos);

  //const char* sensor = doc["sensor"];
  //long time          = doc["time"];
  //double latitude    = doc["data"][0];
  //double longitude   = doc["data"][1];

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
      //client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("comandos");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println("tentando novamente em  5 segundos");
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
  pressaoAlta = (pressaoAlta * 30 - 19.8) / 2.64; // fórmula obtida fazendo uma matriz com 0 bar a 30 bar e 0,66V a 3.3V. Valor do 21.8 na formula é 19.8, foi acrescentado 1 para correção das variações e ficar o mesmo valor do manômetro

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

///////////////////////////////////////////////////////////////////////////////////////////////////
// Controle dos comandos recebidos pelo mqtt. Liga/desliga compressor, exaustores
void controle() {
  //comando:  {"compressor":false,"vent_evaporador":false,"vent_condensador":false}

  if (comandos["compressor"] == false) {
    doc["motor"] = false;
    digitalWrite(5, HIGH);
    Serial.println("compressor desligado");
  } else {
    doc["motor"] = true;
    digitalWrite(5, LOW);
    Serial.println("compressor ligado");
  }
  
  if (comandos["vent_evaporador"] == false) {
    doc["vent evaporador"] = false;
    digitalWrite(18, HIGH);
  } else {
    doc["vent evaporador"] = true;
    digitalWrite(18, LOW);
  }
  
  if (comandos["vent_condensador"] == false) {
    doc["vent condensador"] = false;
    digitalWrite(19, HIGH);
  } else {
    doc["vent condensador"] = true;
    digitalWrite(19, LOW);
  }
  return;
}

void calculosTeoricos(){

//Temp_Ev_calc = -59,9235 + 47,7325*P_EVexp - 17,8049*P_EVexp^2 + 3,86138*P_EVexp^3 - 0,329592*P_EVexp^4
// formula original:  Temp_Ev_calc =  -59.9235 + 47.7325 * (pressaoBaixa + 1.015) - 17.8049 * pow((pressaoBaixa+1.015),2) + 3.86138 * pow((pressaoBaixa+1.015),3) - 0.329592 * pow((pressaoBaixa+1.015),4);
Temp_Ev_calc =  -59.9235 + 47.7325 * (pressaoBaixa + 1.015) - 17.8049 * ((pressaoBaixa+1.015)*(pressaoBaixa+1.015)) + 3.86138 * ((pressaoBaixa+1.015)*(pressaoBaixa+1.015)*(pressaoBaixa+1.015)) 
- 0.329592 * ((pressaoBaixa+1.015)*(pressaoBaixa+1.015)*(pressaoBaixa+1.015)*(pressaoBaixa+1.015));


//T_CDcalc = -26,6328 + 11,3174*P_CDexp - 0,691216*P_CDexp^2 + 0,0259686*P_CDexp^3 - 0,000396834*P_CDexp^4
// formula original:  Temp_Cd_calc =  -26.6328 + 11.3174 * (pressaoAlta +1.015) - 0.691216 * pow((pressaoAlta+1.015),2) + 0.0259686 * pow((pressaoAlta+1.015),3) - 0.000396834 * pow((pressaoAlta+1.015),4);
Temp_Cd_calc =  -26.6328 + 11.3174 * (pressaoAlta +1.015) - 0.691216 * ((pressaoAlta+1.015)*(pressaoAlta+1.015)) + 0.0259686 * ((pressaoAlta+1.015)*(pressaoAlta+1.015)*(pressaoAlta+1.015)) 
- 0.000396834 * ((pressaoAlta+1.015)*(pressaoAlta+1.015)*(pressaoAlta+1.015)*(pressaoAlta+1.015));

//Pot_CPcalc1=8,24941434E+01-5,37328089E-01*T_EVexp-6,26019814E-02*T_EVexp^2-4,33566434E-04*T_EVexp^3+9,41134033E-01*T_CDexp+3,47902098E-03*T_CDexp^2-1,51515151E-05*T_CDexp^3+3,84205128E-02*T_EVexp*T_CDexp+4,07342657E-04*T_EVexp*T_CDexp^2+1,05920745E-03*T_EVexp^2*T_CDexp+1,04895104E-06*T_EVexp^2*T_CDexp^2
// formula original:  Pot_teorica = 82.4941434 - 0.537328089 * temp4 - 0.0626019814 * pow(temp4,2) - 0.000433566434 * pow(temp4,3) + 0.941134033 * temp3 + 0.00347902098 * pow(temp3,2) - 0.0000151515151 * 
//                pow(temp3,3) + 0.0384205128 * temp4 * temp3 + 0.000407342657 * temp4 * pow(temp3,2) + 0.00105920745 * pow(temp4,2) * temp3 + 0.00000104895104 * pow(temp4,2) * pow(temp3,2);

Pot_teorica = 82.4941434 - 0.537328089 * temp4 - 0.0626019814 * (temp4 * temp4) - 0.000433566434 * (temp4 * temp4 * temp4) + 0.941134033 * temp3 + 0.00347902098 * (temp3 * temp3) - 0.0000151515151 * 
                (temp3 * temp3 * temp3) + 0.0384205128 * temp4 * temp3 + 0.000407342657 * temp4 * (temp3 * temp3) + 0.00105920745 * (temp4 * temp4) * temp3 + 0.00000104895104 * (temp4 * temp4) * 
                (temp3 * temp3);
                
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// setup ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {

  Serial.begin(115200);

  pinMode(5, OUTPUT);
  pinMode(18, OUTPUT);
  pinMode(19, OUTPUT);
  digitalWrite(5, HIGH);
  digitalWrite(18, HIGH);
  digitalWrite(19, HIGH);
  // seta por padrao o compressor e exaustores para desligado/off toda vez que a ESP inicializa
  comandos["compressor"] = false;
  comandos["vent_evaporador"] = false;
  comandos["vent_condensador"] = false;
  doc["motor"] = false;
  doc["vent condensador"] = false;
  doc["vent evaporador"] = false;
  controle();

  //WiFi.mode(WIFI_STA); // Esta linha esconde a visualização do ESP como hotspot wifi
  WiFi.mode(WIFI_AP_STA); // Comfigura o ESP32 como ponto de acesso e estação

  // configuração e checagem dos NTC digitais
  sensortemp.begin();
  Serial.println("Localizando sensores de temperatura ...");
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

  tempoAnterior = millis(); // para verificação do tempo que leva no loop

  //comando:  {"compressor":false,"vent_evaporador":false,"vent_condensador":false}
  // loop para ficar "dormindo" quando o sistema não coleta dados, baseado no compressor, se estiver desligado, entra no loop
  while (comandos["compressor"] == false) {
    tempoAtual = millis();
    tempoAtual = tempoAtual - tempoAnterior;
    if (tempoAtual > tempoLoopDesligado) break; // fica preso no loop o tempo definido pra só depois enviar os dados, como um delay, mas mantendo ativo as funções de mqtt, OTA e wifi

    verificarStatusWifi(); //conecta e reconecta no wifi

    //manipula clientes conectados
    server.handleClient();

    // conectar no broker mqtt
    if (!client.connected()) {
      reconnect();
    }
    client.loop();
    Serial.println("loop controle compressor");

    controle();

    delay(2000);
  }

  verificarStatusWifi(); //conecta e reconecta no wifi

  server.handleClient(); //manipula clientes conectados

  if (!client.connected()) {
    reconnect(); // conectar no broker mqtt
  }
  client.loop();

  controle(); // função de verificação dos comandos do compressor e exaustores, enviados pelo subscriber mqtt

  leituraNTC_digitais(); // leitura sensores digitais
  leituraPressao(); // leitura transdutores de pressão
  consumo(); // leitura wattímetro
  

  //dados = dados + String(temp1) + ";" + String(temp2) + ";" + String(temp3) + ";" + String(temp4) + ";" + String(temp5) +
  //        ";" + String(temp6) + ";" + String(temp7) + ";" + String(temp8) + ";" + String(temp9) + ";" + String(pressaoBaixa) +
  //        ";" + String(pressaoAlta) + ";" + String(W) + ";" + String(V) + ";" + String(I) + ";" + String(FP) + ";" + String(Wh);

  // condição caso o wattímetro esteja desligado, substituí o nan (null no json recebido pelo node-red) por 0 pra poder armazenar no influxDB
  if (isnan(W)) W = 0;
  if (isnan(V)) V = 0;
  if (isnan(I)) I = 0;
  if (isnan(FP)) FP = 0;
  if (isnan(Wh)) Wh = 0;
  if (isnan(Freq)) Freq = 0;

  if (doc["pressao baixa"] < 0) doc["pressao baixa"] = 0;
  if (doc["pressao alta"] < 0) doc["pressao alta"] = 0;

  // JSON
//  doc["motor"] = false;
//  doc["vent condensador"] = false;
//  doc["vent evaporador"] = false;
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
  doc["frequencia"] = Freq;

  calculosTeoricos(); // calcula T3, T4 e Pot, teoricamente, pra comparar com o medido no sistema.

  doc["Temp_Ev_calc"] = Temp_Ev_calc;
  doc["Temp_Cd_calc"] = Temp_Cd_calc;
  doc["Pot_teorica"] = Pot_teorica;

  serializeJson(doc, dados); // serializa em "texto" os dados Json para ser enviado pelo mqtt

  //serializeJson(doc, Serial); // para imprimir

  client.publish("dados_refrig", dados); // envia o JSON pro tópico dados_refrig no broker MQTT no node-red

  tempoAtual = millis();
  tempoAtual = tempoAtual - tempoAnterior;
  Serial.println(tempoAtual); // pra saber a média de tempo que leva entre as leituras

}
