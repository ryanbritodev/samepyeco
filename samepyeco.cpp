/*
------------------ FIAP --------------------
GLOBAL SOLUTION GREEN ENERGY
EDGE COMPUTING & COMPUTER SYSTEMS
Participantes:
Prof. Paulo Marcotti PF2150
Diogo Leles Franciulli RM558487
Felipe Sousa de Oliveira RM559085
Ryan Brito Pereira Ramos RM554497
--------------------------------------------

SAMEPYECO: GERADOR TERMOELÉTRICO SUSTENTÁVEL
 
Descrição:
O SamepyEco é um gerador de energia termoelétrico sustentável, projetado
para aproveitar a diferença de temperatura entre dois reservatórios de água
(um quente e um frio) e gerar energia elétrica de forma limpa e eficiente.
Este projeto visa contribuir para o uso consciente dos recursos energéticos,
oferecendo uma alternativa sustentável e escalável para a geração de energia
em regiões remotas, indústrias ou contextos domésticos. O sistema é equipado
com sensores e utiliza um ESP32 para coletar dados, monitorar a eficiência e
enviar informações em tempo real para a nuvem através do protocolo HTTP (via
ThingSpeak), permitindo o acompanhamento remoto do desempenho.

Funcionamento:
• Placas Peltier: 
Convertem a diferença de temperatura entre os dois reservatórios
(água quente e fria) em energia elétrica.
• Reservatório de Água Quente:
Pode ser aquecido pelo sol (através de silos pintados de preto para aumentar
a absorção térmica). Pode aproveitar calor residual de processos industriais.
• Reservatório de Água Fria:
Mantém água em temperatura ambiente ou resfriada para garantir o gradiente 
térmico.
• Sensores:
Dois sensores de temperatura DS18B20 monitoram a temperatura dos reservatórios.
Um potenciômetro simula a leitura da tensão gerada pelas placas Peltier no 
ambiente de simulação.
• ESP32:
Coleta os dados dos sensores.
Envia informações para a nuvem via Wi-Fi.
Integração com ThingSpeak para monitoramento remoto.
Utiliza a API da https://ipgeolocation.io/ para obter a localização da rede Wi-Fi
em que o ESP32 está conectado (insira sua chave de API na variável apiKey).

Fields Utilizados no ThingSpeak:
• Field 1: Latitude
• Field 2: Longitude
• Field 3: Cidade
• Field 4: País
• Field 5: Temperatura (Água Fria)
• Field 6: Temperatura (Água Quente)
• Field 7: Tensão

Dependências:
WiFi (<WiFi.h>)
HTTPClient (<HTTPClient.h>)
ArduinoJson (<ArduinoJson.h>)
OneWire (<OneWire.h>)
DallasTemperature (<DallasTemperature.h>)
Wire (<Wire.h>)
LiquidCrystal I2C (<LiquidCrystal_I2C.h>)

Referências:
https://github.com/ryanbritodev/samepyeco
https://youtu.be/6PZ9-6zow2M?si=0dqdfydpMLmBOOF0
https://youtu.be/0HuZBaha4aI?si=aWUscF2hDZLagFlF
https://youtu.be/wLrXYMJs-q8?si=RZjFwoTbGtvumNiv
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const char* ssid = "Wokwi-GUEST";
const char* password = "";

String apiKey = "6b3c4fe6ffa348d7958c0fa2ac0acfd4";
String serverName = "http://api.ipgeolocation.io/ipgeo?apiKey=" + apiKey;

const char* thingspeakURL = "http://api.thingspeak.com/update";
String thingspeakApiKey = "SSIVNUFSSLRC9EZQ";

const int tempPinFria = 2;
const int tempPinQuente = 4;
OneWire oneWireFria(tempPinFria);
OneWire oneWireQuente(tempPinQuente);
DallasTemperature sensorFria(&oneWireFria);
DallasTemperature sensorQuente(&oneWireQuente);

#define sinalSensor 19 

LiquidCrystal_I2C lcd(0x27, 16, 2);

byte fria[8] = { 0b01110, 0b01010, 0b01010, 0b01010, 0b10001, 0b11111, 0b11111, 0b01110 };
byte quente[8] = { 0b01110, 0b01010, 0b01110, 0b01110, 0b11111, 0b11111, 0b11111, 0b01110 };
uint8_t tensao[] = { 0x0e, 0x1b, 0x11, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f };
byte raio[8] = { 0b00010, 0b00110, 0b01100, 0b11111, 0b11111, 0b00110, 0b01100, 0b01000 };
byte pin[8] = { 0b01110, 0b11111, 0b11111, 0b01110, 0b00100, 0b00100, 0b00100, 0b00100 };

void mostrarLCD(String titulo, float valor, int tipo);
void mostrarLocalizacao(String country, String city);
void enviarParaThingSpeak(float latitude, float longitude, String city, String country, float tempFria, float tempQuente, float voltage);

void setup() {
  Serial.begin(115200);

  sensorFria.begin();
  sensorQuente.begin();

  lcd.init();
  lcd.backlight();
  lcd.createChar(0, fria);
  lcd.createChar(1, quente);
  lcd.createChar(2, tensao);
  lcd.createChar(3, raio);
  lcd.createChar(4, pin);
  lcd.setCursor(0, 0);
  lcd.print("Global  Solution");
  lcd.setCursor(0, 1);
  lcd.write((byte)3);
  lcd.print(" Green Energy ");
  lcd.write((byte)3);
  delay(3000);
  lcd.clear();

  Serial.print("Conectando-se ao Wi-Fi ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado ao Wi-Fi!");
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    http.begin(serverName);
    int httpResponseCode = http.GET();
    if (httpResponseCode > 0) {
      String payload = http.getString();
      StaticJsonDocument<1024> doc;
      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {
        String country = doc["country_name"].as<String>();
        String city = doc["city"].as<String>();
        float latitude = doc["latitude"];
        float longitude = doc["longitude"];

        Serial.println("Dados obtidos com sucesso!");
        Serial.print("País: "); Serial.println(country);
        Serial.print("Cidade: "); Serial.println(city);
        Serial.print("Latitude: "); Serial.println(latitude, 6);
        Serial.print("Longitude: "); Serial.println(longitude, 6);

        sensorFria.requestTemperatures();
        float temperaturaFria = sensorFria.getTempCByIndex(0);
        Serial.print("Temp. Água Fria: "); Serial.println(temperaturaFria);

        sensorQuente.requestTemperatures();
        float temperaturaQuente = sensorQuente.getTempCByIndex(0);
        Serial.print("Temp. Água Quente: "); Serial.println(temperaturaQuente);

        float leituraADC = analogRead(sinalSensor);
        // Cálculo utilizado na vida real para capturar a tensão DC gerada pelas Placas Peltiers
        // float voltage = (float)analogRead(sinalSensor) / 4096 * 15 * 28205 * 1.725 / 27000;
        float voltage = leituraADC / 4095.0 * 3.3;
        Serial.print("Tensão: "); Serial.println(voltage, 2);

        mostrarLCD(" Agua Fria", temperaturaFria, 0);
        mostrarLCD(" Agua Quente", temperaturaQuente, 1);
        mostrarLCD(" Tensao", voltage, 2);
        mostrarLocalizacao(country, city);
        
        enviarParaThingSpeak(latitude, longitude, city, country, temperaturaFria, temperaturaQuente, voltage);
      } else {
        Serial.print("Erro ao processar JSON: ");
        Serial.println(error.f_str());
      }
    } else {
      Serial.print("Erro na requisição HTTP: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("Erro na conexão Wi-Fi");
  }
  delay(15000); 
}

void mostrarLCD(String titulo, float valor, int tipo) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write((byte)tipo);  
  lcd.print(titulo);
  lcd.setCursor(0, 1);
  lcd.print("Valor: ");
  lcd.print(valor, 2);
  lcd.print(tipo == 2 ? "V" : "C");
  delay(5000);  
}

void mostrarLocalizacao(String country, String city) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write((byte)4);  
  lcd.print(" ");      
  lcd.print(country);  
  lcd.setCursor(0, 1);
  lcd.print(city);     
  delay(5000);         
}

void enviarParaThingSpeak(float latitude, float longitude, String city, String country, float tempFria, float tempQuente, float voltage) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    city.replace(" ", "%20");
    country.replace(" ", "%20");

    String url = String(thingspeakURL) + "?api_key=" + thingspeakApiKey +
                 "&field1=" + String(latitude, 6) +
                 "&field2=" + String(longitude, 6) +
                 "&field3=" + city +
                 "&field4=" + country +
                 "&field5=" + String(tempFria, 1) +
                 "&field6=" + String(tempQuente, 1) +
                 "&field7=" + String(voltage, 2);

    http.begin(url);
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      Serial.print("Dados enviados para o ThingSpeak com sucesso! Código: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("Erro ao enviar dados para o ThingSpeak! Código: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  }
}
