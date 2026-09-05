/*!
 * file OTABlynkCredentials.h
 *
 * This code is made for entering WiFi and Blynk Credentials
 * Over the Air using your web browser.
 * 
 *
 * Written by Sachin Soni for techiesms YouTube channel, with
 * contributions from the open source community.
 *
 *Visit our channel for more interesting projects
 *https://www.youtube.com/techiesms
 *
 */



#ifdef ESP8266
 #include <ESP8266WiFi.h>
#elif defined(ESP32)
 #include <WiFi.h>
#else
 #error "Board not found"
#endif

//
//
//  Install these all libraries to make the project work.
//
//
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

//extern Adafruit_SSD1306 display;
extern TFT_eSPI tft;
extern TFT_eSprite img;
extern TFT_eSprite imgWiFi;

class credentials {
  public:
  bool credentials_get();
  void setupAP(char* softap_ssid, char* softap_pass);
  void server_loops();
  void EEPROM_Config();
  void Erase_eeprom();
  String ssid = "";
  String pass = "";
  void fillArc(int x, int y, int start_angle, int seg_count, int rx, int ry, int w, unsigned int colour);
  void beginOTA(const char* otaUser, const char* otaPass); // web OTA nadogradnja firmvera (http://<IP>/update)
  void otaLoop();                                          // pozvati u glavnoj loop() petlji
  private:
  bool _testWifi(void);
  void _launchWeb(void);
  void _createWebServer(void);
  void drawProgressbar(int x, int y, int width, int height, int progress, int full);
  void WiFiLogo(int x, int y, int frame);
  
};
