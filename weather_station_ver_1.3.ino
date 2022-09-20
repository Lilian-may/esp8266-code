//using this code on esp8266 board with sensors: bmp280, aht20, ds18b20, mq135 and TFT Display 240*240 st7789
#include <Wire.h>              // Wire library (required for I2C devices)
#include <Adafruit_GFX.h>      // Adafruit core graphics library
#include <Adafruit_ST7789.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_AHTX0.h>
#include <microDS18B20.h>
#include <TimerMs.h>
#include <FastBot.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <MQ135.h>
#include "NTPClient.h"
#include "ESP8266WiFi.h"
#include "WiFiUdp.h"

// ST7789 TFT module connections
#define TFT_DC    D3
#define TFT_RST   D0
#define TFT_CS    D7

#define WIFI_SSID "/*your_wi-fi_ssid*/"
#define WIFI_PASS "/*your_password*/"
#define BOT_TOKEN "/*your_bot_token(for_telegram_bot)*/"
#define CHAT_ID "/*your_chat_id(for_telegram_bot)*/"
#define SEALEVELPRESSURE_HPA (1013.25)
#define PIN_MQ135 A0

// initialize ST7789 TFT library with hardware SPI module
// SCK (CLK) ---> NodeMCU pin D5 (GPIO14)
// MOSI(DIN) ---> NodeMCU pin D7 (GPIO13)
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// define device I2C address: 0x76 or 0x77 (0x77 is library default address)
#define BMP280_I2C_ADDRESS  0x76
#define DS_PIN D4

const long utcOffsetInSeconds = 4*60*60; //edit first number for your timezone (utc+num)
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

Adafruit_BMP280 bmp280;
Adafruit_AHTX0 aht;
FastBot bot(BOT_TOKEN);
TimerMs tmr(1000*60*5, 1, 1);
MicroDS18B20<D4> ds18b20;
MQ135 mq135_sensor(PIN_MQ135);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

String STATE() {
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);
  ds18b20.requestTemp();
  
  float tempahtbmp = 0;
  float tempahtbmpds18b20 = 0;
  float humi = 0;
  
  for (int i = 0; i < 5; i++) {
    tempahtbmp += bmp280.readTemperature();
    tempahtbmp += temp.temperature;
    tempahtbmpds18b20 += ds18b20.getTemp();
    humi += humidity.relative_humidity;
  }
  tempahtbmp /= 10.0f;
  tempahtbmpds18b20 /= 5.0f;
  tempahtbmpds18b20 += tempahtbmp;
  tempahtbmpds18b20 /= 2.0f;
  humi /= 5.0f;
  String message = "Temperature:\nBMP280&&AHT20: " + String(tempahtbmp) + " ºC \n";
  message += "BMP280&&AHT20&&DS18B20: " + String(tempahtbmpds18b20) + " ºC \n";
  message += "BMP280: " + String(bmp280.readTemperature()) + " ºC \n";
  message += "AHT20: " + String(temp.temperature) + " ºC \n";
  message += "DS18B20: " + String(ds18b20.getTemp()) + " ºC \n";
  message += "Pressure:\nBMP280: " + String (bmp280.readPressure() / 133.3) + " mm Hg \n";
  message += "Humidity:\nAHT20: " + String (humidity.relative_humidity) + "% rH \n";
  message += "Approximate Altitube:\nBMP280: " + String(bmp280.readAltitude(SEALEVELPRESSURE_HPA)) + " m \n";
  message += "MQ135:\nCO2: " + String(mq135_sensor.getCorrectedPPM(tempahtbmp, humi)) + " ppm";
  return message;
}

void setup(void) {

  timeClient.begin();

  connectWiFi();
  bot.attach(newMsg);
  bot.setChatID(CHAT_ID);
  bot.attach(newMsg);
  bot.setChatID(CHAT_ID);
  bot.sendMessage("Weather Station ON!\nVersion 1.2");

  // initialize the ST7789 display (240x240 pixel)
  // if the display has CS pin try with SPI_MODE0
  tft.init(240, 240, SPI_MODE2);

  // if the screen is flipped, remove this command
  tft.setRotation(2);
  // fill the screen with black color
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextWrap(false);         // turn off text wrap option

  // initialize the BME280 sensor
  Wire.begin(D2, D1);  // set I2C pins [SDA = D2, SCL = D1], default clock is 100kHz
  if( bmp280.begin(BMP280_I2C_ADDRESS) == 0 )
  {  // connection error or device address wrong!
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);   // set text color to white and black background
    tft.setTextSize(4);      // text size = 4
    tft.setCursor(3, 88);    // move cursor to position (3, 88) pixel
    tft.print("Connection");
    tft.setCursor(63, 126);  // move cursor to position (63, 126) pixel
    tft.print("Error");
    while(1) delay(1000);  // stay here
  }

  if (! aht.begin()) {
    Serial.println("Could not find AHT? Check wiring");
    while (1) delay(10);
  }
  Serial.println("AHT10 or AHT20 found");


  tft.setTextColor(0xAFE5, ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setCursor(0, 0);
  tft.print("Temperature:");

  tft.setTextColor(0xF81F, ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setCursor(0, 72);
  tft.print("Pressure:");

  tft.setTextColor(0x07FF, ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setCursor(0, 108);
  tft.print("Humidity:");

  tft.setTextColor(0xC618, ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setCursor(0, 144);
  tft.print("MQ135:");

  tft.setTextColor(0xB33F, ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setCursor(0, 180);
  tft.print("Date && Time:");

//stringnum*18
}

void newMsg(FB_msg& msg) {
  
  Serial.println(msg.toString());

  if (msg.text == "/weather"){
    bot.sendMessage(STATE(), msg.chatID);
  }
}

// main loop
void loop() {

  timeClient.update();

  bot.tick();
  bot.setChatID(CHAT_ID);
  bot.attach(newMsg);

  if (tmr.tick()) {
    bot.sendMessage(STATE());
  }

  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);
  ds18b20.requestTemp();

  delay(100);

  float temperatureformq135 = 0;
  float humidityformq135 = 0;
  for (int i = 0; i < 5; i++) {
    temperatureformq135 += bmp280.readTemperature();
    temperatureformq135 += temp.temperature;
    humidityformq135 += humidity.relative_humidity;
  }
  temperatureformq135 /= 10.0f;
  humidityformq135 /= 5.0f;

  float co2mq135 = mq135_sensor.getCorrectedPPM(temperatureformq135, humidityformq135);

//длинна одного символа: 5x7 пикселей
//для tft.setTextSize(2); используется формула символы*пиксели*2 + 4(для оси х), символы*пиксели*2 + 6(для оси у)
//для tft.setTextSize(2); используется формула символы*пиксели*3 + 6(для оси х), символы*пиксели*2 + 8(для оси у)

  tft.setTextColor(0xAFE5, ST77XX_BLACK);
  tft.setCursor(0, 18);
  tft.print("BMP280: " + String(bmp280.readTemperature()) + " \'C");
  tft.setTextSize(2);
  
  tft.setTextColor(0xAFE5, ST77XX_BLACK);
  tft.setCursor(0, 54);
  if (ds18b20.readTemp()) {
    tft.print("DS18B20: " + String(ds18b20.getTemp()) + " \'C");
  }
  else tft.print("DS18B20: Error");
  tft.setTextSize(2);

  tft.setTextColor(0xAFE5, ST77XX_BLACK);
  tft.setCursor(0, 36);
  tft.print("AHT20: " + String(temp.temperature) + " \'C");
  tft.setTextSize(2);

  tft.setTextColor(0x07FF, ST77XX_BLACK);
  tft.setCursor(0, 126);
  tft.print("AHT20: " + String(humidity.relative_humidity) + "% rH");
  tft.setTextSize(2);
  
  tft.setTextColor(0xF81F, ST77XX_BLACK);
  tft.setCursor(0, 90);
  tft.print("BMP280: " + String(bmp280.readPressure() / 133.3) + " mm Hg");
  tft.setTextSize(2);

  tft.setTextColor(0xC618, ST77XX_BLACK);
  tft.setCursor(0, 162);
  tft.print("CO2: " + String(co2mq135) + " ppm");
  tft.setTextSize(2);

  tft.setTextColor(0xB33F, ST77XX_BLACK);
  tft.setCursor(0, 198);
  tft.printf("%s %02d:%02d:%02d", String(daysOfTheWeek[timeClient.getDay()]), timeClient.getHours(), timeClient.getMinutes(), timeClient.getSeconds());
  tft.setTextSize(2);

  String co2mq135CW = "You Can Close Window!\n" + String(co2mq135) + " ppm";
  String co2mq135OW = "Open Window!\n" + String(co2mq135) + " ppm";

  if (co2mq135 > 1250.0f && tmr.tick()) {
    bot.sendMessage(co2mq135OW);
  }
  
  if (co2mq135 < 1250.0f && tmr.tick()) {
    bot.sendMessage(co2mq135CW);
  }

}
void connectWiFi() {
  delay(2000);
  Serial.begin(115200);
  Serial.println();
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Соединение с WI-FI сетью не установленно!\n");
    if (millis() > 15000) ESP.restart();
  }
  Serial.println("Подключено к WI-FI сети!");
}
