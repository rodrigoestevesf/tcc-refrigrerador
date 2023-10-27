// --------------------Incluindo bibliotecas-------------------------

#include <WiFi.h> // Biblioteca do Wifi
#include <PubSubClient.h> // Biblioteca conexão MQTT

#include <OneWire.h> // Biblioteca do protocolo OneWire
#include <DallasTemperature.h> // Biblioteca do sensor de temperatura
#include "EmonLib.h" // Biblioteca do sensor de corrente

#include <SPI.h> //Para usar I2C
#include <Wire.h> //Para usar I2C
#include <Adafruit_GFX.h> //Biblioteca de efeitos do display
#include <Adafruit_SSD1306.h> //Biblioteca do display

// ---------------Definindo parâmetros do WiFI----------------------

const char* REDE = "SSID"; // SSID / nome da REDE WiFi que deseja se conectar
const char* SENHA = "PASSWORD"; // SENHA da REDE WiFi que deseja se conectar
WiFiClient wifiClient; // Definindo o objeto wifiClient

// ---------------Definindo parâmetros do MQTT-----------------------

int BROKER_PORTA = 1883; // Porta do Broker MQTT
const char* BROKER_MQTT = "test.mosquitto.org"; //URL do broker MQTT

#define ID_MQTT  "TCC_REFRIGERADOR_ESP32"  //ID MQTT
PubSubClient MQTT(wifiClient);      // Instancia o Cliente MQTT passando o objeto wifiClient

// --------------Definindo parâmetros do display---------------------

#define LARGURA_DISPLAY 128 // Largura do display
#define ALTURA_DISPLAY 32 // Altura do display
#define OLED_RESET -1 // Pino de reset do display (se não tiver usar "-1" para resetar o microcontrolador)
#define SCREEN_ADDRESS 0x3C // Endereço I2C; Para displays 0x3D para 128x64 e 0x3C para 128x32

Adafruit_SSD1306 display(LARGURA_DISPLAY, ALTURA_DISPLAY, &Wire, OLED_RESET); // Cria-se o display

// -----Definindo parâmetros do OneWire e Sensor de Temperatura------

#define ONE_WIRE_INPUT 4 // Terminal que será conectado o barramento 1-Wire
OneWire oneWire(ONE_WIRE_INPUT); // Cria-se um objeto oneWire com funções do mesmo protocolo, recebendo o terminal do barramento
DallasTemperature DS18B20(&oneWire); // Cria-se um objeto "sensor" com funções do sensor DS18B20

// -----------Definindo parâmetros do Sensor de Corente--------------

#define pinCor 32 // Terminal que será conectado o sensor de corrente
#define cal 3.6 // Valor da calibração do sensor
EnergyMonitor SCT013; // Cria objeto SCT013

// ------------------Criando variaveis do programa-------------------

int Ntela = 0; // Tela respectiva do display
String telas[6] = {"Geladeira", "Freezer", "Motor", "Corrente", "Potencia", "Valor"}; // Lista com os nomes dos compartimentos da geladeira

unsigned long ultimo_tempo_display = 0; // Variável unsigned long que receberá ao último tempo para a função espera
unsigned long ultimo_tempo_MQTT = 0; // Variável unsigned long que receberá ao último tempo para a função espera
unsigned long ultimo_tempo_LDR = 0; // Variável unsigned long que receberá ao último tempo para a função espera

double Valor = 0;
int tensao = 127;
double Potencia = 0;
float tarifa = 0.83605;

#define LDR_pin 33
float tensao_min_LDR = 1.5;
bool ultimo_estado_LDR = false;
int vezes_aberta = 0;
int tempo_aberto = 0;

char ValorVar[10];

// ------------------Criando bitmaps para o display------------------

const unsigned char termometro [] PROGMEM = {
  0x00, 0x60, 0x00, 0x00, 0xf0, 0x00, 0x01, 0x98, 0x00, 0x01, 0x08, 0x00, 0x01, 0x08, 0x00, 0x01,
  0x38, 0x00, 0x01, 0x08, 0x00, 0x01, 0x08, 0x00, 0x01, 0x38, 0x00, 0x01, 0x08, 0x00, 0x01, 0x08,
  0x00, 0x01, 0x38, 0x00, 0x01, 0x08, 0x00, 0x01, 0x68, 0x00, 0x01, 0x68, 0x00, 0x01, 0x68, 0x00, // Bitmap de um termômetro
  0x01, 0x68, 0x00, 0x01, 0x68, 0x00, 0x01, 0x68, 0x00, 0x01, 0x68, 0x00, 0x01, 0x68, 0x00, 0x02,
  0xf4, 0x00, 0x02, 0xf4, 0x00, 0x03, 0xfc, 0x00, 0x03, 0xfc, 0x00, 0x03, 0xfc, 0x00, 0x03, 0xfc,
  0x00, 0x02, 0xf4, 0x00, 0x02, 0xf4, 0x00, 0x01, 0x08, 0x00, 0x01, 0xf8, 0x00, 0x00, 0xf0, 0x00
};

const unsigned char eletricidade [] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0xf0, 0x00, 0x01, 0x98, 0x00, 0x03, 0x0c, 0x00, 0x02,
  0x04, 0x00, 0x06, 0x06, 0x00, 0x0c, 0x03, 0x00, 0x0c, 0x63, 0x00, 0x18, 0xe1, 0x80, 0x10, 0xf0,
  0x80, 0x31, 0xf0, 0xc0, 0x60, 0x60, 0x00, 0x60, 0x60, 0x00, 0xc0, 0x40, 0x30, 0xc0, 0x00, 0x30, // Bitmap de eletricidade
  0xc0, 0x00, 0x30, 0x7f, 0xff, 0xe0, 0x3f, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char cifrao [] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0xd0, 0x00, 0x01, 0xf8, 0x00, 0x07, 0xfc, 0x00, 0x06, 0xde, 0x00, 0x0c,
  0xd6, 0x00, 0x0c, 0xd2, 0x00, 0x06, 0xd0, 0x00, 0x07, 0xf0, 0x00, 0x01, 0xf8, 0x00, 0x00, 0xfe,
  0x00, 0x00, 0xdf, 0x00, 0x00, 0xd3, 0x00, 0x0c, 0xd3, 0x00, 0x0c, 0xd3, 0x00, 0x07, 0xde, 0x00, // Bitmap um cifrão
  0x03, 0xfe, 0x00, 0x01, 0xf8, 0x00, 0x00, 0xd0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char WiFi_bitmap [] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xf8, 0x00,
  0x00, 0xff, 0xff, 0x00, 0x03, 0xff, 0xff, 0xc0, 0x0f, 0xff, 0xff, 0xf0, 0x1f, 0xe0, 0x07, 0xf8,
  0x3f, 0x00, 0x00, 0xfc, 0xfc, 0x0f, 0xf0, 0x3f, 0xf8, 0x7f, 0xfe, 0x1f, 0xf0, 0xff, 0xff, 0x0f,
  0x63, 0xff, 0xff, 0xc6, 0x07, 0xf0, 0x0f, 0xe0, 0x0f, 0xc0, 0x03, 0xf0, 0x0f, 0x07, 0xe0, 0xf0, // Bitmap do WiFi
  0x0e, 0x1f, 0xf8, 0x70, 0x04, 0x3f, 0xfc, 0x20, 0x00, 0x7f, 0xfe, 0x00, 0x00, 0xf8, 0x1f, 0x00,
  0x00, 0xf0, 0x0f, 0x00, 0x00, 0xe1, 0x87, 0x00, 0x00, 0x07, 0xe0, 0x00, 0x00, 0x0f, 0xf0, 0x00,
  0x00, 0x0f, 0xf0, 0x00, 0x00, 0x0f, 0xf0, 0x00, 0x00, 0x0f, 0xf0, 0x00, 0x00, 0x07, 0xe0, 0x00,
  0x00, 0x03, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char MQTT_bitmap [] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x80, 0x00,
  0x01, 0xf0, 0xf8, 0x00, 0x07, 0x00, 0x06, 0x00, 0x0c, 0x00, 0x01, 0x80, 0x18, 0x00, 0x00, 0x80,
  0x10, 0x00, 0x00, 0xc0, 0x10, 0x00, 0x00, 0xc0, 0x1c, 0x00, 0x01, 0xc0, 0x17, 0x00, 0x07, 0xc0,
  0x11, 0xf0, 0xfc, 0xc0, 0x10, 0x3f, 0xe0, 0xc0, 0x18, 0x00, 0x00, 0xc0, 0x1c, 0x00, 0x01, 0xc0,
  0x17, 0x80, 0x0e, 0xc0, 0x10, 0xff, 0xf8, 0xc0, 0x10, 0x02, 0x00, 0xc0, 0x18, 0x00, 0x01, 0xc0, // Bitmap do MQTT
  0x1e, 0x00, 0x07, 0xe0, 0x13, 0xc0, 0x1c, 0x30, 0x10, 0xff, 0xfc, 0x10, 0x10, 0x00, 0x18, 0x18,
  0x0c, 0x00, 0x30, 0x0c, 0x06, 0x00, 0x20, 0x04, 0x01, 0xe0, 0x60, 0x04, 0x00, 0x1f, 0xe0, 0x04,
  0x00, 0x00, 0x30, 0x18, 0x00, 0x00, 0x0f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char geladeira_bitmap [] PROGMEM = {
  0x01, 0xff, 0xff, 0x80, 0x01, 0xff, 0xff, 0x80, 0x01, 0xff, 0xff, 0x80, 0x01, 0xff, 0xf9, 0x80,
  0x01, 0xff, 0xf9, 0x80, 0x01, 0xff, 0xf9, 0x80, 0x01, 0xff, 0xf9, 0x80, 0x01, 0xff, 0xf9, 0x80,
  0x01, 0xff, 0xf9, 0x80, 0x01, 0xff, 0xf9, 0x80, 0x01, 0xff, 0xf9, 0x80, 0x01, 0xff, 0xf9, 0x80,
  0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xf9, 0x80, 0x01, 0xff, 0xf9, 0x80, 0x01, 0xff, 0xf9, 0x80, // Bitmap de uma geladeira
  0x01, 0xff, 0xf9, 0x80, 0x01, 0xff, 0xf9, 0x80, 0x01, 0xff, 0xf9, 0x80, 0x01, 0xff, 0xf9, 0x80,
  0x01, 0xff, 0xf9, 0x80, 0x01, 0xff, 0xf9, 0x80, 0x01, 0xff, 0xf9, 0x80, 0x01, 0xff, 0xf9, 0x80,
  0x01, 0xff, 0xf9, 0x80, 0x01, 0xff, 0xf9, 0x80, 0x01, 0xff, 0xff, 0x80, 0x01, 0xff, 0xff, 0x80,
  0x01, 0xff, 0xff, 0x80, 0x01, 0xff, 0xff, 0x80, 0x01, 0xff, 0xff, 0x80, 0x01, 0xff, 0xff, 0x80
};

const unsigned char conectado_bitmap [] PROGMEM = {
  0x00, 0x01, 0x00, 0x02, 0x00, 0x24, 0x00, 0xf8, 0x07, 0xf8, 0x0b, 0xfc, 0x0d, 0xfc, 0x0e, 0xf8, // Bitmap de conectado
  0x1f, 0x70, 0x1f, 0xb0, 0x3f, 0xd0, 0x1f, 0xe0, 0x1f, 0x00, 0x26, 0x00, 0x40, 0x00, 0x80, 0x00
};

const unsigned char desconectado_bitmap [] PROGMEM = {
  0x00, 0x01, 0x00, 0x02, 0x00, 0x4c, 0x00, 0x84, 0x01, 0x40, 0x00, 0xa0, 0x00, 0x54, 0x0e, 0x28, // Bitmap de desconectado
  0x16, 0x10, 0x2b, 0x80, 0x05, 0x80, 0x02, 0x80, 0x21, 0x00, 0x32, 0x00, 0x40, 0x00, 0x80, 0x00
};

// -------------------------Criando funções--------------------------

void receberValores(char* topic, byte* payload, unsigned int length); // Cria-se a função para ser utilizada pelo setCallback(), em iniciar()


/* Função "exibirConexao()"

  Exibe no display as conexões WiFi e MQTT
  ---------------------------------------------------------------------
  String conexao = Conexão referente
  bool estado = Estado conectado ou desconectado
*/

void exibirConexao(String conexao, bool estado) {

  display.clearDisplay();
  display.setTextColor(WHITE);

  if (conexao == "WiFi") {

    display.drawBitmap(0, 0, WiFi_bitmap, 32, 32, 1);

    display.setTextSize(1); // Escolhe o tamanho da fonte
    display.setCursor(35, 0);
    display.print("Conectando...");

    display.setCursor(35, 10);
    display.print("Rede:" + String(REDE));
    display.setCursor(35, 20);
    display.print("Senha:*****");

  } else {

    display.drawBitmap(0, 0, MQTT_bitmap, 32, 32, 1);

    display.setTextSize(1); // Escolhe o tamanho da fonte
    display.setCursor(35, 0);
    display.print("Conectando...");

    display.setCursor(35, 10);
    display.print("test.mosquitto");
    display.setCursor(35, 20);
    display.print("PORTA:" + String(BROKER_PORTA));
  }

  if (estado == 1) {

    display.drawBitmap(108, 12, conectado_bitmap, 16, 16, 1);

  } else {

    display.drawBitmap(108, 12, desconectado_bitmap, 16, 16, 1);

  }

  display.display(); // Atualiza o display
}

/* Função "conectaWifi()"

  Conecta ao Wifi
  ---------------------------------------------------------------------
  String REDE = Nome da REDE WiFi
  String SENHA = SENHA do Wifi

*/

void conectaWiFi() {

  // Se o WiFi já estiver conectado ele para a função
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  int tentaWifi=0;
  
  while (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(REDE, SENHA); // Conecta na REDE WI-FI
    exibirConexao("WiFi", (WiFi.status() == WL_CONNECTED));
    tentaWifi+=1;

    if (tentaWifi>=5){

      ESP.restart();
      
    }
    
    delay(2500);
  }

  exibirConexao("WiFi", (WiFi.status() == WL_CONNECTED));
  delay(500);
}


/* Função "conectaMQTT()"

  Conecta ao broker MQTT
  ---------------------------------------------------------------------
  X
*/

void conectaMQTT() {

  // Se o MQTT já estiver conectado ele para a função
  if (MQTT.connected()) {
    return;
  }

  while (!MQTT.connected()) {
    MQTT.connect(ID_MQTT);
    exibirConexao("MQTT", MQTT.connected());
    delay(500);
  }

  delay(1000);
}

/* Função "controleAbertura()"

  Faz todos as aplicações LDR
  ---------------------------------------------------------------------
  X
*/

void controleAbertura() {

  float tensao_LDR = analogRead(LDR_pin) / 4095 * 3.3;

  if (tensao_LDR < tensao_min_LDR && ultimo_estado_LDR == 0) {

    digitalWrite(19, 1);
    vezes_aberta += 1;
    ultimo_estado_LDR = 1;
    ultimo_tempo_LDR = millis();

  } else {
    if (tensao_LDR > tensao_min_LDR && ultimo_estado_LDR == 1) {

      digitalWrite(19, 0);
      tempo_aberto += (millis() - ultimo_tempo_LDR) / 1000;
      ultimo_estado_LDR = 0;

    }
  }

}

/* Função "iniciarConexoes()"

  Função responsável por iniciar todas conexões
  ---------------------------------------------------------------------
  X
*/

void iniciarConexoes() {

  SCT013.current(pinCor, cal); // Inicia conexão com o sensor de corrente junto ao valor de calibração

  //SSD1306_SWITCHCAPVCC = gerar tensão para display 3.3V internamente
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // 0x3C sendo o end I2C

  conectaWiFi(); // Inicia conexão com o WiFi

  MQTT.setServer(BROKER_MQTT, BROKER_PORTA);
  conectaMQTT(); // Inicia conexão com o broker MQTT
  MQTT.setCallback(receberValores); //Define a funcao chamada quando se recebe valores no mqtt  

  DS18B20.begin(); // Inicia a conexão com os sensores DS18B20  
}


/* Função "mantemConexoes()"

  Mantém conexões Wi-Fi e broker MQTT
  ---------------------------------------------------------------------
  X
*/

void mantemConexoes() {
  conectaWiFi();
  conectaMQTT();
}

/* Função "exibirValores()"

  Exibe no display o bitmap escolhido, a parte da geladeira em questão
  e sua temperatura em °C com uma casa decimal
  ---------------------------------------------------------------------
  String nome = Recebe qual tela exibir
  float temp = Recebe valores em float
*/

void exibirValores(String nome, float valor) {

  display.setTextSize(2); // Escolhe o tamanho da fonte
  display.setTextColor(WHITE); // Escolhe a cor da fonte
  display.clearDisplay(); // Limpa do display

  // Se o nome for da parte referente se exibe ela
  if (nome == ("Geladeira") || nome == ("Freezer") || nome == ("Motor")) {

    display.drawBitmap(0, 0, termometro, 20, 32, 1); // Desenha o bitmap em sua devida coordenada e com seu devido tamanho
    display.setCursor(23, 18); // Coloca o cursor em uma ordenada
    display.print(valor, 1); // Escreve no display a variável float "temp"
    display.setCursor(95, 12); // Coloca o cursor em uma ordenada
    display.print("o"); // Escreve no display a String "o" como fosse "°"
    display.setCursor(110, 18); // Coloca o cursor em uma ordenada
    display.print("C"); // Escreve no display a String "C"

  } else {
    if (nome == "Corrente") {

      display.drawBitmap(0, 10, eletricidade, 20, 32, 1); // Desenha o bitmap em sua devida coordenada e com seu devido tamanho
      display.setCursor(36, 18); // Coloca o cursor em uma ordenada
      display.print(valor, 2); // Escreve no display a variável float "temp"
      display.setCursor(100, 18); // Coloca o cursor em uma ordenada
      display.print("A"); // Escreve no display a String "A"

    } else {
      if (nome == "Potencia") {

        display.drawBitmap(0, 10, eletricidade, 20, 32, 1); // Desenha o bitmap em sua devida coordenada e com seu devido tamanho
        display.setCursor(36, 18); // Coloca o cursor em uma ordenada
        display.print(valor, 2); // Escreve no display a variável float "temp"
        display.setCursor(100, 18); // Coloca o cursor em uma ordenada
        display.print("W"); // Escreve no display a String "W"

      } else {
        if (nome == "Valor") {

          display.drawBitmap(0, 10, cifrao, 20, 32, 1); // Desenha o bitmap em sua devida coordenada e com seu devido tamanho
          display.setCursor(45, 18); // Coloca o cursor em uma ordenada
          display.print(valor, 2); // Escreve no display a variável float "temp"
          display.setCursor(20, 18); // Coloca o cursor em uma ordenada
          display.print("R$"); // Escreve no display a String "R$"
        }
      }
    }
  }

  display.setCursor(20, 0); // Coloca o cursor em uma ordenada
  display.println(nome); // Escreve no display a variável String "nome"

  display.display(); // Atualiza o display
}


/* Função "corrente()"

  Função que retorna o valor de corrente atual do sensor
  ---------------------------------------------------------------------
  int Index = Index do sensor referente
*/

double corrente() {

  double corrente = SCT013.calcIrms(1480); // Armazena em uma variável a corrente do sensor
  return corrente; // Retorna o valor da corrente
}

/* Função "enviarCorr()"

  Envia corrente para o topico referente
  ---------------------------------------------------------------------
  float valor = Valor da corrente a enviar
*/

void enviarCorr(double valor) {

  dtostrf(valor, 4, 3, ValorVar);
  MQTT.publish("TCC_REFRIGERADOR_CORRENTE", ValorVar);
}

/* Função "enviarLDR()"

  Enviar quantidade e tempo de abertura da geladeira
  ---------------------------------------------------------------------
  X
*/

void enviarLDR() {

  String Var_vezes_aberta = String(vezes_aberta);
  String Var_tempo_aberto = String(tempo_aberto);

  const char *MQTT_vezes_aberta = Var_vezes_aberta.c_str();
  const char *MQTT_tempo_aberto = Var_tempo_aberto.c_str();

  MQTT.publish("TCC_REFRIGERADOR_ABERTOS", MQTT_vezes_aberta);
  MQTT.publish("TCC_REFRIGERADOR_TEMPO", MQTT_tempo_aberto);

}

/* Função "temperatura()"

  Função que recebe o index do sensor referente e retorna a temperatura dele
  em °C
  ---------------------------------------------------------------------
  int Index = Index do sensor referente
*/

float temperatura(int Index) {

  DS18B20.requestTemperatures(); // Recebe a temperatura do sensor em questão
  float tempC = DS18B20.getTempCByIndex(Index); // Armazena em uma variável a temperatura do sensor
  return tempC; // Retorna o valor da temperatura
}

/* Função "enviarTemp()"

  Envia temperatura referente para o topico referente
  ---------------------------------------------------------------------
  String parte = Qual parte da temperatura enviar
*/

void enviarTemp(String parte) {

  if (parte == "Geladeira") {

    dtostrf(temperatura(0), 4, 1, ValorVar);
    MQTT.publish("TCC_REFRIGERADOR_TEMP_GELADEIRA", ValorVar);

  } else {
    if (parte == "Freezer") {

      dtostrf(temperatura(1), 4, 1, ValorVar);
      MQTT.publish("TCC_REFRIGERADOR_TEMP_FREEZER", ValorVar);

    } else {
      if (parte == "Motor") {

        dtostrf(temperatura(2), 4, 1, ValorVar);
        MQTT.publish("TCC_REFRIGERADOR_TEMP_MOTOR", ValorVar);

      }
    }
  }
}

/* Função "valor()"

  Função que retorna o valor de corrente atual do sensor
  ---------------------------------------------------------------------
  int Index = Index do sensor referente
*/

double valor() {

  Potencia = (tensao * corrente()) / 1000;
  Valor += Potencia * (tarifa / 3600);
}

/* Função "enviarValor()"

  Envia valor referente para o tópico referente
  ---------------------------------------------------------------------
  double valor = valor a ser enviado
*/

void enviarValor(double valor) {

  dtostrf(valor, 4, 6, ValorVar);
  MQTT.publish("TCC_REFRIGERADOR_VALOR", ValorVar);
}

/* Função "receberValores()"

  Recebe o valor de tensao usado no programa
  ---------------------------------------------------------------------
  String parte = Qual parte da temperatura enviar
*/

void receberValores(char* topic, byte* payload, unsigned int length) {

  char* msg;  

  //obtem a string do payload recebido
  for (int i = 0; i < length; i++)
  {
    char c = (char)payload[i];
    msg += c;
  }

  if(topic=="TCC_REFRIGERADOR_TENSAO")
  {
    if (msg == "127") {
      tensao = 127;
    }

    if (msg == "220") {
      tensao = 220;
    }
  }

}

/* Função "enviarPot()"

  Envia temperatura referente para o topico referente
  ---------------------------------------------------------------------
  String parte = Qual parte da temperatura enviar
*/

void enviarPot(double valor) {

  dtostrf(valor, 4, 3, ValorVar);

  MQTT.publish("TCC_REFRIGERADOR_POTENCIA", ValorVar);

}

/* Função "esperar()"

  Função que retorna verdadeiro caso se tenha passado o tempo estabelecido desde a última
  vez que foi retornado. Uma forma de se evitar o delay() e não travar o código.
  ---------------------------------------------------------------------
  unsigned long tmp = tempo a ser esperado para retornar Verdadeiro
  unsigned long var = tempo a ser esperado para retornar Verdadeiro
*/

bool esperar(unsigned long tmp, char* var) {

  unsigned long ultimo_tempo;

  if (var == "MQTT") ultimo_tempo = ultimo_tempo_MQTT; // ultimo_tempo recebe valor de tempo atual
  else ultimo_tempo = ultimo_tempo_display; // ultimo_tempo recebe valor de tempo atual

  /* Se tiver passado o tempo requesitado desde a última vez que foi passado,
    armazenar o tempo atual na variável ultimo_tempo e retornar Verdadeiro, caso
    contrário retornar Falso*/
  if ((millis() - ultimo_tempo) >= tmp) {

    if (var == "MQTT") ultimo_tempo_MQTT = millis(); // ultimo_tempo recebe valor de tempo atual
    else ultimo_tempo = millis(); // ultimo_tempo recebe valor de tempo atual

    return 1; // Se sim, retorna Verdadeiro;

  } 
  
  return 0;
}