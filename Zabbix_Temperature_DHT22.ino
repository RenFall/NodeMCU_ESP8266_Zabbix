#include "DHT.h"                                        // Подключаем библиотеку DHT
#define DHTPIN 0                                        // Пин к которому подключен датчик
#include <ESP8266ZabbixSender.h>
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);                               // Инициализируем датчик

/* DHT variable*/
float temperature = 0;     // запрос на считывание температуры


const uint8_t averageFactor = 5;  // коэффициент сглаживания показаний (0 = не сглаживать)

// ----------------------------------------------------------------------------
//Первоначальная инициализация сенсора
void dht_setup() {

  dht.begin();                                          // инициализация DHT
  temperature = dht.readTemperature();                  // считывание температуры
  

  return;
}


// ----------------------------------------------------------------------------
//  Чтение показаний сенсора
void dht_read() {

  if (temperature != dht.readTemperature()) {
    // усреднение показаний для устранения "скачков"
    if (averageFactor > 0) {
      // <новое среднее> = (<старое среднее>*4 + <текущее значение>) / фактор сглаживания
      temperature = (temperature * (averageFactor - 1) + dht.readTemperature()) / averageFactor;
    } else {
      temperature = dht.readTemperature();  // не делаем усреднений, что прочитали то и считаем выводом
    }
  }



  return;
}


ESP8266ZabbixSender zSender;

/* WiFi settings */
String ssid = "_______";
String pass = "_______";

/* Zabbix host setting */
IPAddress ip(192, 168, 37, 25);      // host IP
IPAddress gateway(192, 168, 12, 1);  // gateway IP
IPAddress subnet(255, 255, 255, 0);  // network mask

/* Zabbix server setting */

#define SERVERADDR 192, 168, 37, 25  // Zabbix server Address
#define ZABBIXPORT 10051            // Zabbix server Port
#define ZABBIXAGHOST "IOTBOARD_01"  // Zabbix item's host name


// ----------------------------------------------------------------------------
//  Инициализация esp8266
void esp8266_setup() {

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  WiFi.begin(ssid.c_str(), pass.c_str());
  WiFi.config(ip, gateway, subnet);
  while (!checkConnection()) {
  }
  zSender.Init(IPAddress(SERVERADDR), ZABBIXPORT, ZABBIXAGHOST); // Init zabbix server information

  return;
}

// ----------------------------------------------------------------------------
//  Передача данных в Zabbix
void esp8266_sendMessage () {

  checkConnection();                // Check wifi connection
  zSender.ClearItem();              // Clear ZabbixSender's item list
  zSender.AddItem("temperature", (float)temperature); // Exmaple value of zabbix trapper item
  //  zSender.AddItem("humidity", (float)humidity); // Exmaple value of zabbix trapper item
  if (zSender.Send() == EXIT_SUCCESS) {     // Send zabbix items
    Serial.println("ZABBIX SEND: OK");
  } else {
    Serial.println("ZABBIX SEND: NG");
  }

  return;
}


// ----------------------------------------------------------------------------
//  Проверка соединения
boolean checkConnection() {
  int count = 0;
  
  Serial.print("Waiting for Wi-Fi connection");
  while (count < 300) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println();
      Serial.println("Connected!");
      return (true);
    }
    delay(500);
    Serial.print(".");
    count++;
  }
  Serial.println("Timed out.");
  return false;
}

// ----------------------------------------------------------------------------
//  Таймер потока 01
uint8_t avrTthread01(uint16_t span) {
  static uint32_t future = 0;
  if (millis() < future) return 0;
  future += span;
  return 1;
}

// ----------------------------------------------------------------------------
//  Таймер потока 02
uint8_t avrTthread02(uint16_t span) {
  static uint32_t future = 0;
  if (millis() < future) return 0;
  future += span;
  return 1;
}


void setup() {

  Serial.begin(115200);  // инициализация последовательного порта
  esp8266_setup();       // инициализация ESP8266
  dht_setup();           // инициализация DHT

  return;
}


void loop() {

  // опрос датчика dht-11
  if (avrTthread01(15000)) {
    // Serial.println("thread 1 ");
    dht_read();  // Запрос на считывание температуры
  }

  // отправка данных в Zabbix
  if (avrTthread02(60000)) {
    // Serial.println("thread 2 ");
    esp8266_sendMessage();
  }

  return;
}
