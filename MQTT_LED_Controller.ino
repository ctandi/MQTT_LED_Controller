#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>
#include <Scheduler.h>
#include <ArduinoJson.h>


#define PIN 0
#define NUMPIXELS 12

String address;
char address_str[5];
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

//Ergänzen Sie hier die Daten Ihres WLANs und MQTT-Servers
const char* ssid = "WIFI-SSID";
const char* password = "PASSWORD";

const char* mqtt_server = "192.168.1.1";
//Den MQTT-Topic können Sie in der Reconnect-Funktion ändern

//Wird Benutzerauthentifizierung verwendet, muss in der Reconnect-Funktion die andere Connect-Zeile aktiviert werden
//const char* mqtt_user = "MQTT-USER";
//const char* mqtt_password = "MQTT-PASS";

WiFiClient espClient;
PubSubClient client(espClient);

//Sollten Grün und Rot vertauscht sein, ändern Sie NEO_RGB auf NEO_GRB
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
    if (address == "all") {
      r = tv.toInt();
      g = fov.toInt();
      b = fiv.toInt();

      for (int i = 0; i < NUMPIXELS; i++){
        sprintf(address_str,"%d", i);        
        if (sv == "blink") {
            JsonObject& led = blinkj.createNestedObject(address_str);
            led["r"] = tv;
            led["g"] = fov;
            led["b"] = fiv;
        }
        else if (sv == "fastblink") {
            JsonObject& led = fastblinkj.createNestedObject(address_str);
            led["r"] = tv;
            led["g"] = fov;
            led["b"] = fiv;   
        }
        else if (sv == "stop") {
          blinkj.remove(address_str);
          fastblinkj.remove(address_str);
          pixels.setPixelColor(i, pixels.Color(0, 0, 0));
          pixels.show();
        }
        pixels.setPixelColor(i, pixels.Color(r, g, b));
        pixels.show();
      }
    } else { 
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
}
  else {
    int r = sv.toInt();
    int g = tv.toInt();
    int b = fov.toInt();  
  
    if (address == "all") {
      for (int i = 0; i < NUMPIXELS; i++){
        pixels.setPixelColor(i, pixels.Color(r, g, b));
        }
      pixels.show();
  
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
//    if (client.connect("ESP8266 Client", mqtt_user, mqtt_password)) {
//  diese Connect-Zeile ist fuer Verbindungen mit Benutzerauthentifizierung
      //Der Default-Topic lautet ledbar/lights. Sie können Ihn beliebig ändern
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
    Serial.printf("Blinktask startet - %d\n",state);
    for (int i = 0; i < NUMPIXELS; i++) {
        String index = String(i);
        // Fastblink bei jedem 2. Durchgang einschalten
        if (fastblinkj[index]) {
          if ((state % 2) == 1) {
            Serial.printf("f%d ",i);
            r = fastblinkj[index]["r"].as<int>();
            g = fastblinkj[index]["g"].as<int>();
            b = fastblinkj[index]["b"].as<int>();
            pixels.setPixelColor(i, pixels.Color(r, g, b));
          } else {
            pixels.setPixelColor(i, pixels.Color(0, 0, 0));
          }
        }
        // Normales Blink bei der oberen Haelfte der Durchgaenge einschalten
        if (blinkj[index]) {
          if ((state / 8) == 1) {
            Serial.printf("b%d ",i);
            r = blinkj[index]["r"].as<int>();
            g = blinkj[index]["g"].as<int>();
            b = blinkj[index]["b"].as<int>();
            pixels.setPixelColor(i, pixels.Color(r, g, b));
          } else {
            pixels.setPixelColor(i, pixels.Color(0, 0, 0));
          }
        }
     }
     Serial.print(".\n");
     if (state < 15) {
       state = state + 1;
     } else {
       state = 0;
     }
     pixels.show();
     delay(150);
     Serial.printf("Blinktask fertig - %d\n", state);
  }
private:
  uint8_t state;
} blink_task;

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
  Serial.begin(9600);
  pixels.begin();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WIFI");

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
    Serial.print(".");
  }
  Serial.print("Connected\n");
  
  delay(500);
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  Scheduler.start(&mqtt_task);
  Scheduler.start(&blink_task);

  Scheduler.begin();
  
}

void loop() {}
