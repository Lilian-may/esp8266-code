//is working for bmp280 and/or aht20
#include <FastBot.h>
#include <Wire.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>

#define WIFI_SSID "SSID ur WI-FI"
#define WIFI_PASS "password of ur WI-FI"
#define BOT_TOKEN "ur bot token"

FastBot bot(BOT_TOKEN);
Adafruit_AHTX0 aht;
Adafruit_BMP280 bmp280;

#define CHAT_ID "your chat ID"
#define SEALEVELPRESSURE_HPA (1013.25)

const int ledPin = D4;
const int ledPinTwo = D3;
bool ledState = HIGH;
bool ledStateTwo = HIGH;

String AHT20BALCONY() {
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);
  String message = "Температура: " + String(temp.temperature) + " ºC \n";
  message += "Влажность: " + String (humidity.relative_humidity) + " % \n";
  return message;
}

String BMP280ROOM() {
  String message = "Температура: " + String(bmp280.readTemperature()) + " ºC \n";
  message += "Давление: " + String (bmp280.readPressure() / 133.3) + " мм. рт. ст. \n";
  message += "Приблизительная Высота: " + String(bmp280.readAltitude(SEALEVELPRESSURE_HPA)) + " м \n";
  return message;
}

String STATE() {
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);
  
  float pressure = 0;
  
  for (int i = 0; i < 7; i++) {
    pressure += bmp280.readTemperature();
    pressure += temp.temperature;
  }
  pressure /= 14.0f;
  String message = "Температура: " + String(pressure) + " ºC \n";
  message += "Влажность: " + String (humidity.relative_humidity) + " % \n";
  message += "Давление: " + String (bmp280.readPressure() / 133.3) + " мм. рт. ст. \n";
  message += "Приблизительная Высота: " + String(bmp280.readAltitude(SEALEVELPRESSURE_HPA)) + " м \n";
  return message;
}

void setup() {
  connectWiFi();
  bot.attach(newMsg);
  aht.begin();
  while (!bmp280.begin(BMP280_ADDRESS - 1)) {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring!"));
    delay(2000);
  }
bot.setChatID(CHAT_ID);
bot.attach(newMsg);
bot.sendMessage("I'm started!");
  
  if (! aht.begin()) {
    Serial.println("Could not find AHT? Check wiring");
    while (1) delay(10);
  }
  else {
    Serial.println("AHT10 or AHT20 found");
  }
  
  bot.setChatID(CHAT_ID);

  pinMode(ledPin, OUTPUT);
  pinMode(ledPinTwo, OUTPUT);
  digitalWrite(ledPin, ledState);
  digitalWrite(ledPinTwo, ledStateTwo);

}
// обработчик сообщений
void newMsg(FB_msg& msg) {
  
  Serial.println(msg.toString());

    if (msg.text == "/led_on_one" || msg.text == "/led_on_all") {
      bot.sendMessage("Светодиод 1 включен!", msg.chatID);
      ledState = HIGH;
      digitalWrite(ledPin, ledState);
    }

    if (msg.text == "/led_off_one" || msg.text == "/led_off_all") {
      bot.sendMessage("Светодиод 1 выключен!", msg.chatID);
      ledState = LOW;
    }

    if (msg.text == "/led_on_two" || msg.text == "/led_on_all") {
      bot.sendMessage("Светодиод 2 включен!", msg.chatID);
      ledStateTwo = HIGH;
      digitalWrite(ledPinTwo, ledStateTwo);
    }

    if (msg.text == "/led_off_two" || msg.text == "/led_off_all") {
      bot.sendMessage("Светодиод 2 выключен!", msg.chatID);
      ledStateTwo = LOW;
      digitalWrite(ledPinTwo, ledStateTwo);
    }

    if (msg.text == "/state_one" || msg.text == "/state_all") {
      if (digitalRead(ledPin)) {
        bot.sendMessage("Состояние светодиода 1: включен.", msg.chatID);
      }
      else {
        bot.sendMessage("Состояние светодиода 1: выключен.", msg.chatID);
      }
    }

    if (msg.text == "/state_two" || msg.text == "/state_all") {
      if (digitalRead(ledPinTwo)) {
        bot.sendMessage("Состояние светодиода 2: включен.", msg.chatID);
      }
      else {
        bot.sendMessage("Состояние светодиода 2: выключен.", msg.chatID);
      }
    }

    if (msg.text == "/weather_aht") {
      bot.sendMessage(AHT20BALCONY(), msg.chatID);
    }

    if (msg.text == "/weather_bmp") {
      bot.sendMessage(BMP280ROOM(), msg.chatID);
    }

    if (msg.text == "/state_all" || msg.text == "/weather_all") {
      bot.sendMessage(STATE(), msg.chatID);
    }
}
void loop() {
  bot.tick();
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
