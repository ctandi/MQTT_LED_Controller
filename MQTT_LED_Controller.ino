#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>
#include <Scheduler.h>
#include <ArduinoJson.h>


#define PIN 0
#define NUMPIXELS 12

String address;
int r;
int g;
int b;
String fv;
String sv;
String tv;
String fov;
String fiv;

StaticJsonBuffer<8192> jsonBuffer;
JsonObject& blinkj = jsonBuffer.createObject();
JsonObject& fastblinkj = jsonBuffer.createObject();

// Connect to the WiFi
const char* ssid = "WIFI-SSID";
const char* password = "PASSWORD";
const char* mqtt_server = "192.168.1.1";

WiFiClient espClient;
PubSubClient client(espClient);

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_RGB + NEO_KHZ800);

void callback(char* topic, byte* payload, unsigned int length) {
  String spayload;
  for (int i = 0; i < length; i++) {
    spayload += (char)payload[i];
  }
  Serial.print("Nachricht empfangen: " + spayload + "\n");
  int ci = spayload.indexOf(',');
  int sci = spayload.indexOf(',', ci + 1);
  int tci = spayload.indexOf(',', sci + 1);
  int foci = spayload.indexOf(',', tci + 1);

  fv = spayload.substring(0, ci);
  sv = spayload.substring(ci + 1, sci);
  tv = spayload.substring(sci + 1, tci);
  fov = spayload.substring(tci + 1, foci);
  fiv = spayload.substring(foci + 1, spayload.length());
  address = fv;


  if (sv == "blink" || sv == "fastblink" || sv == "stop"){
    int firstaddress;
    int iaddress = address.toInt();
    
    r = tv.toInt();
    g = fov.toInt();
    b = fiv.toInt();
    
    if (sv == "blink") {
        JsonObject& led = blinkj.createNestedObject(address);
        led["r"] = tv;
        led["g"] = fov;
        led["b"] = fiv;
    }
    else if (sv == "fastblink") {
        JsonObject& led = fastblinkj.createNestedObject(address);
        led["r"] = tv;
        led["g"] = fov;
        led["b"] = fiv;   
    }
    else if (sv == "stop") {
      blinkj.remove(address);
      fastblinkj.remove(address);
      pixels.setPixelColor(iaddress, pixels.Color(0, 0, 0));
      pixels.show();
      }
      
}
  else {
    int r = sv.toInt();
    int g = tv.toInt();
    int b = fov.toInt();  
  
    if (address == "all") {
      for (int i = 0; i < NUMPIXELS; i++){
        pixels.setPixelColor(i, pixels.Color(r, g, b));
        pixels.show();
        }
  
    }
    else {
      int lednum = address.toInt();
      pixels.setPixelColor(lednum, pixels.Color(r, g, b));
      pixels.show();
      }
  }
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect("ESP8266 Client")) {
      client.subscribe("ledbar/lights");
      pixels.setPixelColor(0, pixels.Color(0, 50, 0));
      pixels.show();
      delay(2000);
      pixels.setPixelColor(0, pixels.Color(0, 0, 0));
      pixels.show();
      } 
      else {
      pixels.setPixelColor(0, pixels.Color(0, 0, 50));
      pixels.show();
      delay(2000);
    }
  }
}

class blinktask : public Task {
protected:
  void loop() {
    for (int i = 0; i < NUMPIXELS; i++) {
        String index = String(i);
        if (blinkj[index]) {
          r = blinkj[index]["r"].as<int>();
          g = blinkj[index]["g"].as<int>();
          b = blinkj[index]["b"].as<int>();
          pixels.setPixelColor(i, pixels.Color(r, g, b));
        }
        else {
          delay(50);
        }
     }
     pixels.show();
     delay(750);
     for (int i = 0; i < NUMPIXELS; i++) {
        String index = String(i);
        if (blinkj[index]) {
          pixels.setPixelColor(i, pixels.Color(0, 0, 0));
        }
        else {
          delay(50);
        }
     }
     pixels.show();
     delay(750);
  }
private:
  uint8_t state;
} blink_task;

class fastblinktask : public Task {
protected:
  void loop() {
    for (int i = 0; i < NUMPIXELS; i++) {
        String index = String(i);
        if (fastblinkj[index]) {
          r = fastblinkj[index]["r"].as<int>();
          g = fastblinkj[index]["g"].as<int>();
          b = fastblinkj[index]["b"].as<int>();
          pixels.setPixelColor(i, pixels.Color(r, g, b));
        }
        else {
          delay(50);
        }
     }
     pixels.show();
     delay(100);
     for (int i = 0; i < NUMPIXELS; i++) {
        String index = String(i);
        if (fastblinkj[index]) {
          pixels.setPixelColor(i, pixels.Color(0, 0, 0));
        }
        else {
          delay(50);
        }
     }
     pixels.show();
     delay(100);
  }
private:
  uint8_t state;
} fastblink_task;

class MQTTtask : public Task {
protected:
  void loop() {
       if (!client.connected()) {
          reconnect();}   
       client.loop();
  }
private:
  uint8_t state;
} mqtt_task;

void setup()
{
  pixels.begin();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  for (int i = 0; i < NUMPIXELS; i++)
      pixels.setPixelColor(i, pixels.Color(0, 0, 0));
      pixels.show();

  while (WiFi.status() != WL_CONNECTED) {
    pixels.setPixelColor(0, pixels.Color(50, 0, 0));
    pixels.show();
    delay(50);
    pixels.setPixelColor(0, pixels.Color(0, 0, 0));
    pixels.show();
    delay(50);
  }

  
  
  Serial.begin(9600);
  delay(500);
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  Scheduler.start(&mqtt_task);
  Scheduler.start(&blink_task);
  Scheduler.start(&fastblink_task);

  Scheduler.begin();
  
}

void loop() {}
