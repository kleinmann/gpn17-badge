#define VERSION 2
#define DEBUG

#include <GPNBadge.h>
#include <ArduinoJson.h>

#define WEB_SERVER_BUFFER_SIZE 20

#include <FS.h>
#include <BadgeUI.h>



#include "rboot.h"
#include "rboot-api.h"


const char* ssid = "entropia"; // Put your SSID here
const char* password = "pw"; // Put your PASSWORD here

// Color definitions
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

Badge badge;
WindowSystem* ui = new WindowSystem(&tft);


void setup() {
  badge.init();

  if (!SPIFFS.begin()) { //initialize the spiffs
    // TODO
    Serial.print("Error init SPIFFS!");
  }

  badge.setBacklight(true);
  ui->root->setBmp("/system/load.bmp", 40, 12);
  ui->root->setSub("");
  ui->draw();

  Serial.print("Battery Voltage:  ");
  Serial.println(badge.getBatVoltage());
  Serial.print("Light LDR Level:  ");
  Serial.println(badge.getLDRLvl());

  pixels.setPixelColor(0, pixels.Color(30, 0, 0));
  pixels.setPixelColor(1, pixels.Color(30, 0, 0));
  pixels.setPixelColor(2, pixels.Color(30, 0, 0));
  pixels.setPixelColor(3, pixels.Color(30, 0, 0));
  pixels.show();


  //setGPIO(VIBRATOR, HIGH);
  //delay(500);
  //setGPIO(VIBRATOR, LOW);

  badge.setGPIO(IR_EN, LOW);

  badge.setAnalogMUX(MUX_JOY);

  if (SPIFFS.exists("/config.json")) {
    Serial.print("Found config.json");
    connectBadge();
  } else {
    initialConfig();
  }

}

void loop() {
  badge.setAnalogMUX(MUX_JOY);
  delay(10);
  uint16_t adc = analogRead(A0);
  Serial.println(adc);

  if (adc < UP + OFFSET && adc > UP - OFFSET) {
    pixels.setPixelColor(0, pixels.Color(0, 50, 50));
  }

  else if (adc < DOWN + OFFSET && adc > DOWN - OFFSET) {
    pixels.setPixelColor(3, pixels.Color(0, 50, 50));
  }

  else if (adc < RIGHT + OFFSET && adc > RIGHT - OFFSET) {
    pixels.setPixelColor(2, pixels.Color(0, 50, 50));
  }

  else if (adc < LEFT + OFFSET && adc > LEFT - OFFSET) {
    pixels.setPixelColor(1, pixels.Color(0, 50, 50));
  }

  else if (digitalRead(GPIO_BOOT) == HIGH) {
    badge.setGPIO(VIBRATOR, HIGH);
  }

  else {
    badge.setGPIO(VIBRATOR, LOW);
    pixels.clear();
  }
  pixels.show();
  ui->draw();
  delay(100);
}

void connectBadge() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  tft.setCursor(2, 22);
  tft.print("Connecting");
  tft.setCursor(63, 22);
  uint8_t cnt = 0;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    tft.print(".");
    cnt++;
    if (cnt % 4 == 0) {
      tft.fillRect(63, 22, 30, 10, BLACK);
      tft.setCursor(63, 22);
    }
    delay(250);
  }
  Serial.println("WiFi connected");
  Serial.println("");
  Serial.println(WiFi.localIP());

  tft.setTextColor(GREEN);
  tft.setCursor(2, 42);
  tft.print("WiFi connected!");
  tft.setCursor(2, 52);
  tft.print(WiFi.localIP());
}
StaticJsonBuffer<200> jsonBuffer;
void initialConfig() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(IPAddress (10, 0, 0, 1), IPAddress(10, 0, 0, 1), IPAddress(255, 255, 255, 0));
  char pw[20];
  char ssid[20];
  sprintf(pw, "%d", random(10000000, 100000000)); // TODO richtig machen
  sprintf(ssid, "ESP%d", ESP.getChipId());
#ifdef DEBUG
  sprintf(pw, "%s", "hallohallo");
#endif
  Serial.println(WiFi.softAP(ssid, pw));
  FullScreenBMPStatus* webStatus = new FullScreenBMPStatus();
  connectWizard(ssid, pw, webStatus);
  WiFiServer server(80);
  server.begin();
  // Own Webserver. _slow_ but memory efficient
  while (true) {
    if (WiFi.softAPgetStationNum() == 0) {
      connectWizard(ssid, pw, webStatus);
    }
    WiFiClient currentClient = server.available();
    if (!currentClient)  {
      continue;
    }
    String headBuf;
    String getValue;
    while (currentClient.available()) {
      char r = char(currentClient.read());
      if (r == '\n') {
        if (headBuf.startsWith("GET")) {
          getValue = headBuf.substring(4, headBuf.length() - 10);//.strip();
          Serial.print("GET::");
          Serial.println(getValue);
          Serial.flush();
          break;
        }
      } else {
        headBuf += r;
      }
    }
    headBuf = String();
    if (getValue == "/api/scan") {
      pixels.setPixelColor(0, pixels.Color(0, 0, 60));
      pixels.setPixelColor(1, pixels.Color(0, 0, 60));
      pixels.setPixelColor(2, pixels.Color(0, 0, 60));
      pixels.setPixelColor(3, pixels.Color(0, 0, 60));
      pixels.show();
      int n = WiFi.scanNetworks();
      pixels.setPixelColor(0, pixels.Color(0, 0, 0));
      pixels.setPixelColor(1, pixels.Color(0, 0, 0));
      pixels.setPixelColor(2, pixels.Color(0, 0, 0));
      pixels.setPixelColor(3, pixels.Color(0, 0, 0));
      pixels.show();

      File scanDump = SPIFFS.open("/system/web/ls.js", "w+");
      //server.send(200, "application/json", "");
      //server.writeDirect("[");


      JsonArray& root = jsonBuffer.createArray();
      for (int i = 0; i < n; ++i)
      {
        JsonObject& ssid = root.createNestedObject();
        ssid["ssid"] = WiFi.SSID(i);
        ssid["rssi"] = WiFi.RSSI(i);
        ssid["encType"] = WiFi.encryptionType(i);
        //server.writeDirect("{ssid: \"");
        //server.writeDirect(WiFi.SSID(i));
        //server.writeDirect("\", rssi: ");
        //server.writeDirect(String(WiFi.RSSI(i)));
        //server.writeDirect(", encType: \"");
        //server.writeDirect(String(WiFi.encryptionType(i)));
        //server.writeDirect("}");
        //if(i-1 < n) {
        //  server.writeDirect(",");
        //}
      }
      currentClient.write("HTTP/1.1 200\r\n");
      currentClient.write("Content-Type: application/json");
      currentClient.write("\r\n\r\n");
      root.printTo(currentClient);
    } else {
      if (getValue == "/") {
        getValue = "/index.html";
      }
      String path = "/system/web" + getValue;
      if (SPIFFS.exists(path) || SPIFFS.exists(path + ".gz")) {
        currentClient.write("HTTP/1.1 200\r\n");
        currentClient.write("Content-Type: ");
        currentClient.write(getContentType(path).c_str());
        if (SPIFFS.exists(path + ".gz")) {
          path += ".gz";
          currentClient.write("\r\nContent-Encoding: gzip");
        }
        currentClient.write("\r\n\r\n");
        File file = SPIFFS.open(path, "r");
        path = String();
        char writeBuf[WEB_SERVER_BUFFER_SIZE];
        int pos = 0;
        while (file.available()) {
          writeBuf[pos++] = char(file.read());
          if (pos == WEB_SERVER_BUFFER_SIZE) {
            currentClient.write(&writeBuf[0], size_t(WEB_SERVER_BUFFER_SIZE));
            pos = 0;
          }
        }
        // Flush the remaining bytes
        currentClient.write(&writeBuf[0], size_t(pos));
      } else {
        currentClient.write("HTTP/1.1 200");
        currentClient.write("\r\n\r\n");
        currentClient.write("Hallo welt");
      }
    }
    currentClient.stop();

  }
  server.close();
  ui->closeCurrent();
}

void connectWizard(char* ssid, char* pw, FullScreenBMPStatus* webStatus) {
  webStatus->setBmp("/system/wifi.bmp", 13, 6);
  webStatus->setSub(ssid);
  ui->open(webStatus);
  ui->draw();
  int last_b = millis();
  bool b_state = false;
  while (true) {
    if (millis() - last_b > 1000) {
      b_state = !b_state;
      pixels.setPixelColor(0, pixels.Color(0, b_state ? 0 : 30, 0));
      pixels.setPixelColor(1, pixels.Color(0, b_state ? 0 : 30, 0));
      pixels.setPixelColor(2, pixels.Color(0, b_state ? 0 : 30, 0));
      pixels.setPixelColor(3, pixels.Color(0, b_state ? 0 : 30, 0));
      pixels.show();
      last_b = millis();
    }
    if (badge.getJoystickState() != NOTHING) {
      webStatus->setBmp("/system/lock.bmp", 35, 6);
      webStatus->setSub(pw);
      ui->draw();
    }
    if (WiFi.softAPgetStationNum() > 0) {
      webStatus->setBmp("/system/pc.bmp", 10, 9);
      webStatus->setSub("10.0.0.1", 30, 52);
      ui->draw();
      break;
    }
    delay(20);
  }
  pixels.setPixelColor(0, pixels.Color(0, b_state ? 0 : 30, 0));
  pixels.setPixelColor(1, pixels.Color(0, b_state ? 0 : 30, 0));
  pixels.setPixelColor(2, pixels.Color(0, b_state ? 0 : 30, 0));
  pixels.setPixelColor(3, pixels.Color(0, b_state ? 0 : 30, 0));
  pixels.show();
}

String getContentType(String filename) {
  if (filename.endsWith(".htm")) return "text/html";
  else if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".gif")) return "image/gif";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".xml")) return "text/xml";
  else if (filename.endsWith(".pdf")) return "application/x-pdf";
  else if (filename.endsWith(".zip")) return "application/x-zip";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  else if (filename.endsWith(".json")) return "application/json";
  return "text/plain";
}


