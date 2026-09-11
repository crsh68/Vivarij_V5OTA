
/*!
   file OTABlynkCredentials.h

   This code is made for entering WiFi and Blynk Credentials
   Over the Air using your web browser.


   Written by Sachin Soni for techiesms YouTube channel, with
   contributions from the open source community.

  Visit our channel for more interesting projects
  https://www.youtube.com/techiesms

*/

#include "Arduino.h"
#include <TFT_eSPI.h>

#include "OTABlynkCredentials.h"

#define ELEGANTOTA_USE_ASYNC_WEBSERVER 1
#include <ElegantOTA.h>

#define DEG2RAD 0.0174532925


AsyncWebServer server(80); // Creating WebServer at port 80
WebSocketsServer webSocket = WebSocketsServer(81); // Creating WebSocket Server at port81



//
// This is the webpage which is hosted on your ESP board 
// and you can access this webpage by going to this address
//               
//                    192.168.4.1
//




char _webpage[] PROGMEM = R"=====(
<html>
  <head>
    <script>
var connection = new WebSocket('ws://'+location.hostname+':81/');

connection.onopen = function () 
{  
connection.send('Connect ' + new Date()); };
 connection.onerror = function (error) 
 {   
  console.log('WebSocket Error ', error);
 };
 connection.onmessage = function (e) 
  {  
    console.log('Server: ', e.data);
  };
  
  function credentials_rec()
{

  var ssid = document.getElementById('ssid_cred').value;
  var pass = document.getElementById('pass_cred').value;
  var auth = document.getElementById('auth_cred').value;
  var full_command = '#{"ssid":"'+ ssid +'", "pass":"' +pass +'", "auth":"' +auth +'"}';
  console.log(full_command);
  connection.send(full_command);
  location.replace('http://'+location.hostname+'/submit');
}


  </script

  </head>
  <body style="background-color:#99bc70"; width:500px>
  <p style="font-size: 24; font-family: verdana; text-align: center;">Vivarij WiFi</p>
  <p style="font-family: verdana; text-align: center;"><label for="ssid_cred">SSID:</label></p>
  <p style="font-family: verdana; text-align: center;"><input type="text" id="ssid_cred"></p>
  <p style="font-family: verdana; text-align: center;"><label for="pass_cred">Lozinka:</label></p>
  <p style="font-family: verdana; text-align: center;"><input type="text" id="pass_cred"></p>
  <p style="font-family: verdana; text-align: center;"><label for="auth_cred">Blynk Auth Token:</label></p>
  <p style="font-family: verdana; text-align: center;"><input type="text" id="auth_cred"></p>

  <p style="font-family: verdana; text-align: center;">
     <button type="button" onclick=credentials_rec();>Zapamti</button></p>

</body>

</html>


)=====";


void _webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{

  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

        // send message to client
        webSocket.sendTXT(num, "Connected");
      }
      break;
    case WStype_TEXT:
      Serial.printf("[%u] get Text: %s\n", num, payload);

      if (payload[0] == '#') 
      {
        String message = String((char*)( payload));
        message = message.substring(1);
        Serial.println(message);

        //JSON part
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, message);

        String ssid = doc["ssid"];
        String pass = doc["pass"];
        String auth = doc["auth"];
        Serial.println(ssid); Serial.println(pass);


        // Clearing EEPROM
        if (ssid.length() > 0 && pass.length() > 0) {
          Serial.println("clearing eeprom");
          for (int i = 0; i < 100; ++i) {
            EEPROM.write(i, 0);
          }


          // Storing in EEPROM
          Serial.println("writing eeprom ssid:");
          for (int i = 0; i < ssid.length(); ++i)
          {
            EEPROM.write(i, ssid[i]);
            Serial.print("Wrote: ");
            Serial.println(ssid[i]);
          }
          Serial.println("writing eeprom pass:");
          for (int i = 0; i < pass.length(); ++i)
          {
            EEPROM.write(32 + i, pass[i]);
            Serial.print("Wrote: ");
            Serial.println(pass[i]);
          }
          Serial.println("writing eeprom auth token:");
          for (int i = 0; i < auth.length(); ++i)
          {
            EEPROM.write(64 + i, auth[i]);
            Serial.print("Wrote: ");
            Serial.println(auth[i]);
          }

          EEPROM.commit();
          {
            // millis umjesto delay - u međuvremenu i dalje servisiramo websocket
            unsigned long waitStart = millis();
            while (millis() - waitStart < 2000) {
              webSocket.loop();
            }
          }

          //Restarting ESP board
          ESP.restart();
          break;
        }
     }
  }
}



void credentials::Erase_eeprom()
{
  EEPROM.begin(512); //Initialasing EEPROM
  Serial.println("Erasing...");
  Serial.println("clearing eeprom");
    for (int i = 0; i < 100; ++i) 
    {
      EEPROM.write(i, 0);
    }
   EEPROM.commit();
}



void credentials::EEPROM_Config()
{
  EEPROM.begin(512); //Initialasing EEPROM
  Serial.println();
  Serial.println();

  //---------------------------------------- Read eeprom for ssid and pass
  Serial.println("Reading EEPROM");

  ssid="";
  for (int i = 0; i < 32; ++i)
  {
    ssid += char(EEPROM.read(i));
  }
  Serial.print("SSID: ");
  Serial.println(ssid);

  pass="";
  for (int i = 32; i < 64; ++i)
  {
    pass += char(EEPROM.read(i));
  }
  Serial.print("Password: ");
  Serial.println(pass);

  authToken = "";
  for (int i = 64; i < 100; ++i)
  {
    authToken += char(EEPROM.read(i));
  }
  Serial.print("Blynk Auth Token: ");
  Serial.println(authToken);
}



bool credentials::credentials_get()
{
  if (_testWifi())
  {
    Serial.println("Succesfully Connected!!!");
    return true;
  }
  else
  {
    Serial.println("Turning the HotSpot On");
    return false;
  }
}



void credentials::setupAP(char* softap_ssid, char* softap_pass)
{
  
  WiFi.disconnect();
  {
    unsigned long waitStart = millis();
    while (millis() - waitStart < 100) {
      // millis umjesto delay - kratka pauza da se WiFi stogu da vremena
    }
  }
  WiFi.softAP(softap_ssid,softap_pass);
  Serial.println("softap");
  _launchWeb();
  Serial.println("Server Started");
  webSocket.begin();
  webSocket.onEvent(_webSocketEvent);
}



bool credentials::_testWifi()
{
  Serial.println("Waiting for Wifi to connect");
  char* my_ssid = &ssid[0];
  char* my_pass = &pass[0];

  WiFi.begin(my_ssid, my_pass);

  const unsigned long secs = 60;
  const unsigned long totalMs = secs * 1000UL;
  const unsigned long stepMs = 500UL; // koliko često se osvježava prikaz na ekranu

  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  unsigned long startTime = millis();
  unsigned long lastStep = startTime;
  int c = 0;

  while (millis() - startTime < totalMs) {
    // WiFi status se provjerava kontinuirano (bez čekanja na delay),
    // pa se spajanje detektira odmah čim uspije
    if (WiFi.status() == WL_CONNECTED)
    {
      return true;
    }

    if (millis() - lastStep >= stepMs) {
      lastStep = millis();

      // sekunde do isteka - računaju se izravno iz proteklog vremena i
      // ograničene su na 0 prema dolje, tako da ne mogu "otići u minus"
      // i uzrokovati underflow (koji bi ispisao ogroman broj poput 2147483647)
      long secondsLeft = (long)secs - (long)((millis() - startTime) / 1000UL);
      if (secondsLeft < 0) secondsLeft = 0;
      Serial.println(secondsLeft);

      WiFiLogo(74, 115, c);
      drawProgressbar(1, 200, 238, 20, secs*2 - c, secs*2);
      c++;
    }
  }
  Serial.println("");
  Serial.println("Connect timed out, opening AP");
  return false;
}



// This is the function which will be called when an invalid page is called from client
void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}


void credentials::_createWebServer()
{
server.on("/", [](AsyncWebServerRequest * request) {
  request->send_P(200, "text/html", _webpage);
});

// Send a GET request to <IP>/get?message=<message>
server.on("/submit", HTTP_GET, [] (AsyncWebServerRequest * request) {
  String message;
    message = "Credentials received by ESP board!!!";
   request->send(200, "text/plain", message);
});

server.onNotFound(notFound);
server.begin();
  
}

void credentials::_launchWeb()
{
  Serial.println("");
  Serial.print("SoftAP IP: ");
  Serial.println(WiFi.softAPIP());
  _createWebServer();
  // Start the server

}

void credentials::server_loops()
{ 
     webSocket.loop();
}

/*======================================================
   WEB OTA - nadogradnja firmvera preko preglednika
   Stranica dostupna na: http://<IP adresa ESP-a>/update
   Zaštićena korisničkim imenom i lozinkom (Basic Auth).
======================================================*/
void credentials::beginOTA(const char* otaUser, const char* otaPass)
{
  ElegantOTA.begin(&server, otaUser, otaPass);

  // VAŽNO: ovi callbackovi se izvršavaju iz AsyncTCP FreeRTOS taska,
  // NE iz glavnog loop() taska. TFT_eSPI/SPI pozivi napravljeni izravno
  // ovdje uzrokuju krah ("assert failed: xTaskPriorityDisinherit").
  // Zato se ovdje SAMO postavljaju zastavice - stvarno iscrtavanje
  // radi otaLoop(), koja se poziva iz glavnog loop() i time je sigurna.
  ElegantOTA.onStart([this]() {
    Serial.println("OTA: nadogradnja firmvera započela");
    _otaStartPending = true;
  });

  ElegantOTA.onProgress([this](size_t current, size_t final) {
    _otaCurrent = current;
    _otaFinal = final;
    _otaProgressPending = true;
  });

  ElegantOTA.onEnd([this](bool success) {
    Serial.println(success ? "OTA: uspjeh, restartam se" : "OTA: neuspjeh");
    _otaSuccess = success;
    _otaEndPending = true;
  });

  server.begin();
  Serial.println("OTA web poslužitelj aktivan na /update");
}

void credentials::otaLoop()
{
  ElegantOTA.loop();

  // Stvarno iscrtavanje po TFT-u radi se ovdje (glavni loop() task),
  // nikad direktno unutar ElegantOTA callbackova iznad.
  if (_otaStartPending) {
    _otaStartPending = false;
    tft.fillRoundRect(10, 90, 220, 60, 8, TFT_BLACK);
    tft.drawRoundRect(10, 90, 220, 60, 8, TFT_WHITE);
    tft.setTextDatum(C_BASELINE);
    tft.setFreeFont(&FreeSans9pt7b);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setTextSize(1);
    tft.drawString("OTA nadogradnja...", 120, 120, 1);
  }

  static unsigned long lastDraw = 0;
  if (_otaProgressPending && millis() - lastDraw > 200) {
    _otaProgressPending = false;
    lastDraw = millis();
    if (_otaFinal > 0) {
      drawProgressbar(15, 130, 210, 14, _otaCurrent, _otaFinal);
    }
  }

  if (_otaEndPending) {
    _otaEndPending = false;
    tft.fillRoundRect(10, 90, 220, 60, 8, TFT_BLACK);
    tft.drawRoundRect(10, 90, 220, 60, 8, TFT_WHITE);
    tft.setTextColor(_otaSuccess ? TFT_GREEN : TFT_RED, TFT_BLACK);
    tft.drawString(_otaSuccess ? "OTA uspjela, restart..." : "OTA neuspjela!", 120, 120, 1);
  }
}


void credentials::drawProgressbar(int x, int y, int width, int height, int progress, int full)
{
  int radius=10;
  float bar = ((float)(width-4) / full) * progress;
  img.createSprite(238, 20);
  img.fillRect(0,0,238,20, TFT_SKYBLUE);
  img.fillRoundRect(2, 2, width-4, height-4, radius-2, TFT_BLACK);
  img.drawRoundRect(0, 0, width, height, radius, TFT_WHITE);
  img.fillRoundRect(2, 2, bar, height-4, radius-2, TFT_ORANGE);

  img.pushSprite(x, y);
  img.deleteSprite();
}

void credentials::fillArc(int x, int y, int start_angle, int seg_count, int rx, int ry, int w, unsigned int colour)
{
  byte seg = 1; // Segments are 3 degrees wide = 120 segments for 360 degrees
  byte inc = 6; // Draw segments every 3 degrees, increase to 6 for segmented ring

  // Calculate first pair of coordinates for segment start
  float sx = cos((start_angle - 90) * DEG2RAD);
  float sy = sin((start_angle - 90) * DEG2RAD);
  uint16_t x0 = sx * (rx - w) + x;
  uint16_t y0 = sy * (ry - w) + y;
  uint16_t x1 = sx * rx + x;
  uint16_t y1 = sy * ry + y;

  // Draw colour blocks every inc degrees
  for (int i = start_angle; i < start_angle + seg * seg_count; i += inc) {
    // Calculate pair of coordinates for segment end
    float sx2 = cos((i + seg - 90) * DEG2RAD);
    float sy2 = sin((i + seg - 90) * DEG2RAD);
    int x2 = sx2 * (rx - w) + x;
    int y2 = sy2 * (ry - w) + y;
    int x3 = sx2 * rx + x;
    int y3 = sy2 * ry + y;

    img.fillTriangle(x0, y0, x1, y1, x2, y2, colour);
    img.fillTriangle(x1, y1, x2, y2, x3, y3, colour);

    // Copy segment end to sgement start for next segment
    x0 = x2;
    y0 = y2;
    x1 = x3;
    y1 = y3;
  }
}

void credentials::WiFiLogo(int x, int y, int frame)
{
  img.createSprite(92, 65);
  img.fillRect(0,0,92,65, TFT_SKYBLUE);
  img.fillCircle(46, 55, 10, TFT_WHITE);
  frame = frame % 4;
  switch(frame){
     case 3:
      fillArc(46, 55, 305, 110, 55, 55, 7, TFT_WHITE);
     case 2:
      fillArc(46, 55, 305, 110, 40, 40, 7, TFT_WHITE);
     case 1:
      fillArc(46, 55, 305, 110, 25, 25, 7, TFT_WHITE);
     case 0:
      break;
  }
  img.pushSprite(x, y);
  img.deleteSprite();
}
