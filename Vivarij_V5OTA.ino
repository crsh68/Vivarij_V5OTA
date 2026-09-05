/*======================================
 ESP32  DISPLAY  DS18B20  PCF8574  AHT20
 3V3     Vcc      Vcc      Vcc     Vin
 GND     GND      GND      GND     GND
 D15     DC
 D4      RES
 D5      BLK
 D18     SCK
 D23     SDA
 D19               S
 D21                       SDA     SDA
 D22                       SCL     SCL

 D16 - erasing button
====================================== */


#define BLYNK_TEMPLATE_ID "YOUR_BLYNK_TEMPLATE_ID"     // ⚠️ zamijeni svojim - Blynk konzola → My Templates
#define BLYNK_TEMPLATE_NAME "YOUR_BLYNK_TEMPLATE_NAME" // ⚠️ mora točno odgovarati nazivu templatea u Blynk konzoli
#define BLYNK_DEVICE_NAME "Vivarij32"
#define BLYNK_AUTH_TOKEN "YOUR_BLYNK_AUTH_TOKEN"        // ⚠️ zamijeni svojim tokenom - NIKAD ga ne commitaj javno!

#include "fan2.h"
#include "alert.h"
#include "day.h"
#include "night.h"
#include "miniFont.h"

#include <TFT_eSPI.h> // Hardware-specific library
#include <SPI.h>

#include "OTABlynkCredentials.h"

#ifdef ESP8266
 #include <BlynkSimpleEsp8266.h>
#elif defined(ESP32)
 #include <BlynkSimpleEsp32.h>
#else
 #error "Board not found"
#endif

#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <TimeLib.h>
#include <WidgetRTC.h>
#include <OneWire.h>
#include <DallasTemperature.h> 
#include <SPI.h>
#include <Adafruit_PCF8574.h>


TFT_eSPI tft = TFT_eSPI();       // Invoke custom library
TFT_eSprite img = TFT_eSprite(&tft);
TFT_eSprite imgLoop = TFT_eSprite(&tft);

Adafruit_PCF8574 pcf;

credentials Credentials;


char auth_token[33];
bool connected_to_internet = 0;


//Provide credentials for your ESP server
char* esp_ssid = "Vivarij40";
char* esp_pass = "";

/*
 * Blynk server: blynk-cloud.com
 * port: 8080
 * 
 */

#ifdef ESP8266
  /*------------------ DEFINE DALLAS SENSOR PIN --------------*/
  #define DS18B20 13
  //#define SENSOR2_Type DHT11
  
  /*------------------ EPPROM ERASE PIN __-----------------*/
  #define Erasing_button 3

#elif defined(ESP32)
  /*------------------ DEFINE DALLAS SENSOR PIN --------------*/
   #define DS18B20 19
  /*------------------ DEFINE BACKGROUND LIGHT PIN -----------*/
   #define BLK_PIN 5
   #define ledChannel 0
   #define resolution 8
   #define freq 5000
   #define BLK_DAN 255
   #define BLK_NOC 50
   int BLK_DANnoc = BLK_DAN;    
/*------------------ EPPROM ERASE PIN -------------------*/
   #define Erasing_button 16
#else
  #error "Board not found"
#endif

/*------------------ DEFINE RELAY PINS ------------------*/
#define relejSvijetlo   0
#define relejUVB        1 
#define relejGrijac     2 
#define relejKamen      3
#define relejVlaga      4
#define relejVentilator 5  

/*------------------ DEFINE WIDGETS COLORS -------------*/
#define BLYNK_GREEN      "#23C48E"
#define BLYNK_BLUE       "#04C0F8"
#define BLYNK_YELLOW     "#ED9D00"
#define BLYNK_RED        "#D3435C"
#define BLYNK_DARK_BLUE  "#5F7CD8"
#define BLYNK_DARK_GREEN "#167A59"
#define BLYNK_GRAY       "#C0C0C0"
#define BLYNK_DARK_GRAY  "#808080"

/*------------------ DEFINE TFT DISPLAY ----------------*/
#define TFT_GREY 0x5AEB
#define LOOP_PERIOD 35 // Display updates every 35 ms

/*------------------ DEFINE WIDGETS --------------------*/
#define minVlagaVivarij 30   //30%
#define maxVlagaVivarij 60   //60%
#define minTempVivarij 21    //21°C
#define maxTempVivarij 35    //35°C
#define minTempKamen 38      //38°C
#define maxTempKamen 43      //43°C

/*------------------ OSTALO ----------------------------*/
#define BLYNK_PRINT Serial
#define odstupanje 1    // odstupanje +- u *C od ciljane temperature kod uklj/iskl releja
#define uklj LOW        // logika releja
#define isklj HIGH      // logika releja
  

float vlagaS1 = 0.0, tempS1 = 0.0, vlagaS2 = 0.0, prethodnaTempS1 = 0.0, prethodnaVlagaS1 = 0.0;
int tempS2 = 0, prethodnaTempS2 = 0;

int tempVivarij;
int tempKamen;
int vlagaVivarij;
boolean porukaVlagaVivarijPoslana = false;
boolean porukaTempVivarijPoslana = false;
boolean ventilatorNonStop = false;
boolean DANnoc = true;   // dan = true, noć = false
boolean samoPrviput = true;

unsigned long vrijemePorukeTemp=millis();
unsigned long vrijemePorukeKamen=millis();
unsigned long vrijemePorukeVlaga=millis();
unsigned long vrijemeSad=millis();
unsigned long vrijemePoruke=1000*60*15;   // 15 minuta

/*------------- STATUS SENZORA / MODULA -----------------
   Ako neki senzor ili modul otkaže, program se NE zaustavlja.
   Umjesto toga se postavlja odgovarajuća "OK" zastavica na false,
   pripadajući relej (ako je sigurnosno relevantan) se isključuje,
   te se periodički pokušava ponovno inicijalizirati modul.        */
bool pcfOK = false;   // PCF8574 (relejni modul)
bool ahtOK = false;   // AHT20 (temp/vlaga vivarija)
bool dsOK  = false;   // DS18B20 (temp kamena)

unsigned long lastPcfRetry = 0;
unsigned long lastAhtRetry = 0;
const unsigned long SENSOR_RETRY_INTERVAL = 30000UL; // pokušaj ponovno svakih 30 s


//------------- MINI FONT ---------------------
int colors[2]={TFT_DARKGREEN, TFT_YELLOW};  //first colour is color of background , second is color of digit
int boja_tempS1[2]={TFT_DARKGREEN, TFT_YELLOW};
int boja_vlagaS1[2]={TFT_DARKGREEN, TFT_YELLOW};
int boja_tempS2[2]={TFT_DARKGREEN, TFT_YELLOW};
//---------- BOJE ZA MINI FONT ----------------
int TFTbojaUredu[2]={TFT_DARKGREEN, TFT_YELLOW};
int TFTbojaUklj[2]={TFT_DARKGREEN, TFT_RED};
int TFTbojaIsklj[2]={TFT_DARKGREEN, TFT_SILVER};

//---------- BOJE ZA MALI FONT ----------------
int TFT_tempS1 = TFT_YELLOW;
int TFT_vlagaS1 = TFT_YELLOW;
int TFT_tempS2 = TFT_YELLOW;

//---------- ANIMACIJE  IKONE NA DISPLEJU -----
int aniFrame=0;
unsigned long frameTime=millis();
bool ventAni = false;     //animacija ventilatora
bool vlagaAlarm = false;  //ikona alarma vlage
bool tempAlarmS1 = false; //ikona alarma temp vivarija



        
Adafruit_AHTX0 aht;

SimpleTimer timer;
WidgetRTC rtc;

OneWire oneWire(DS18B20);
DallasTemperature sensors(&oneWire);

/*---------- SIGURNI ZAPIS NA RELEJ (PCF8574) ----------
   Ako PCF8574 nije dostupan, poziv se jednostavno ignorira
   umjesto da program blokira ili se sruši.                */
void relejWrite(uint8_t pin, uint8_t state)
{
  if (pcfOK) {
    pcf.digitalWrite(pin, state);
  }
}

/*---------- SYNC ALL SETTINGS ON BOOT UP ----------*/
//bool isFirstConnect = true;
BLYNK_CONNECTED() {
  rtc.begin();
  //if (isFirstConnect) {
    Blynk.syncAll();
    //isFirstConnect = false;
  //}
}

/*---------- DAN / NOĆ - način rada----------*/
BLYNK_WRITE(V1)
{
    DANnoc=param.asInt();
}

/*---------- UVB LIGHT CONTROL ----------*/
BLYNK_WRITE(V20)
{
  if (param.asInt()) {
    relejWrite(relejUVB, uklj);
  } else {
    relejWrite(relejUVB, isklj);
  }
}

/*---------- VIVARIUM LIGHT CONTROL ----------*/
BLYNK_WRITE(V25)
{
  if (param.asInt()) {
    relejWrite(relejSvijetlo, uklj);
  } else {
    relejWrite(relejSvijetlo, isklj);
  }
}


/*---------- FAN CONTROL ----------*/
BLYNK_WRITE(V23)
{
  if (param.asInt()) {
    relejWrite(relejVentilator, uklj);
    ventilatorNonStop = true;
    Blynk.setProperty(V23, "onLabel", "Vent M");
    Blynk.setProperty(V23, "offLabel", "Vent M");
  } else {
    relejWrite(relejVentilator, isklj);
    ventilatorNonStop = false;
    Blynk.setProperty(V23, "onLabel", "Vent A");
    Blynk.setProperty(V23, "offLabel", "Vent A");
  }
}

/*---------- FOOD MENU ----------*/
BLYNK_WRITE(V10)
{
 TimeInputParam t(param);
  
  int d;
  String dan = "";
  d=weekday();
  switch (d){
    case 1:
      dan="povrće";
      break;
    case 2:
      dan="kukci";
      break;
    case 3:
      dan="povrće";
      break;
    case 4:
      dan="voće";
      break;
    case 5:
      dan="kukci";
      break;
    case 6:
      dan="povrće";
      break;
    case 7:
      dan="post";
      break;
 
  }
  Blynk.setProperty(V10, "onLabel", dan);
  //Blynk.virtualWrite(V10, 1);
 }
/*---------- HEAT VIVARIUM CONTROL ----------*/
BLYNK_WRITE(V11)
{
  tempVivarij=param.asInt();
}

/*---------- HEAT STONE CONTROL ----------*/
BLYNK_WRITE(V12)
{
  tempKamen=param.asInt();
}

/*---------- HUMIDITY VIVARIUM CONTROL ----------*/
BLYNK_WRITE(V13)
{
  vlagaVivarij=param.asInt();
}

/*----------READ AHT SENSOR ----------*/
void sensorRead() {

//===== periodički pokušaj oporavka neispravnih modula =====
  if (!pcfOK && millis() - lastPcfRetry > SENSOR_RETRY_INTERVAL) {
    lastPcfRetry = millis();
    pcfOK = pcf.begin(0x20, &Wire);
    if (pcfOK) {
      for (uint8_t p = 0; p < 8; p++) {
        pcf.pinMode(p, OUTPUT);
        pcf.digitalWrite(p, isklj);
      }
      Serial.println("PCF8574 ponovno dostupan");
    }
  }

//===== AHT20 (temperatura + vlažnost vivarija) =====
  if (ahtOK) {
    sensors_event_t humidity, temp;
    bool citanjeUspjelo = aht.getEvent(&humidity, &temp);
    float t = temp.temperature;
    float h = humidity.relative_humidity;

    if (citanjeUspjelo && t > -40 && t < 85 && h >= 0 && h <= 100) {
      tempS1 = t;
      vlagaS1 = h;
      prethodnaTempS1 = tempS1;
      prethodnaVlagaS1 = vlagaS1;
    } else {
      // neispravno/nevjerodostojno očitanje - zadrži zadnju poznatu vrijednost
      // i označi senzor kao neispravan kako bi se relej isključio
      tempS1 = prethodnaTempS1;
      vlagaS1 = prethodnaVlagaS1;
      ahtOK = false;
      Serial.println("AHT20: neispravno očitanje - senzor označen kao neispravan");
    }
  }
  if (!ahtOK && millis() - lastAhtRetry > SENSOR_RETRY_INTERVAL) {
    lastAhtRetry = millis();
    ahtOK = aht.begin();
    Serial.println(ahtOK ? "AHT20 ponovno dostupan" : "AHT20 i dalje nedostupan");
  }

//===== DS18B20 (temperatura kamena) =====
  sensors.requestTemperatures();
  float t2 = sensors.getTempCByIndex(0);
  if (t2 != DEVICE_DISCONNECTED_C && t2 > -40 && t2 < 125) {
    tempS2 = t2;
    prethodnaTempS2 = tempS2;
    dsOK = true;
  } else {
    // senzor ne odgovara - zadrži zadnju poznatu vrijednost, isključi grijač kamena
    tempS2 = prethodnaTempS2;
    dsOK = false;
    Serial.println("DS18B20: senzor ne odgovara");
  }

  Blynk.virtualWrite(V8, tempS1);
  Blynk.virtualWrite(V9, vlagaS1);
  Blynk.virtualWrite(V4, tempS2);
 
//===== kontrola temperature vivarija ======
  if (!ahtOK) {
     // senzor neispravan - iz sigurnosnih razloga grijač je uvijek isključen
     Blynk.setProperty(V11, "color", BLYNK_GRAY);
     boja_tS1(TFTbojaIsklj);
     TFT_tempS1 = TFT_SILVER;
     relejWrite(relejGrijac, isklj);
  }
  else if(tempVivarij>minTempVivarij){
      if(tempS1<(tempVivarij-odstupanje)){
         Blynk.setProperty(V11, "color", BLYNK_RED);
         TFT_tempS1 = TFT_RED;
         boja_tS1(TFTbojaUklj);
         relejWrite(relejGrijac, uklj);
      }
      else if(tempS1>(tempVivarij+odstupanje)){
         Blynk.setProperty(V11, "color", BLYNK_DARK_GREEN);
         boja_tS1(TFTbojaUredu);
         TFT_tempS1 = TFT_YELLOW;
         relejWrite(relejGrijac, isklj);
      }
  }
  else{
     Blynk.setProperty(V11, "color", BLYNK_DARK_GRAY); //ako je klizač na minimalnoj vrijednosti grijač je isključen
     boja_tS1(TFTbojaIsklj);
     TFT_tempS1 = TFT_SILVER;
     relejWrite(relejGrijac, isklj);
  }
  


//==========================================

//===== kontrola temperature kamena ========    kamen je uključen samo po danu
  
  if (!dsOK) {
     // senzor neispravan - iz sigurnosnih razloga grijač kamena je uvijek isključen
     Blynk.setProperty(V12, "color", BLYNK_GRAY);
     TFT_tempS2 = TFT_SILVER;
     boja_tS2(TFTbojaIsklj);
     relejWrite(relejKamen, isklj);
  }
  else if(tempKamen>minTempKamen){
    if(DANnoc){
      if(tempS2<(tempKamen-odstupanje)){
         Blynk.setProperty(V12, "color", BLYNK_RED);
         TFT_tempS2 = TFT_RED;
         boja_tS2(TFTbojaUklj);
         relejWrite(relejKamen, uklj);
      }
      else if(tempS2>(tempKamen+odstupanje)){
         Blynk.setProperty(V12, "color", BLYNK_DARK_GREEN);
         TFT_tempS2 = TFT_YELLOW;
         boja_tS2(TFTbojaUredu);
         relejWrite(relejKamen, isklj);
      }
    }
    else{
       Blynk.setProperty(V12, "color", BLYNK_DARK_GRAY);
       TFT_tempS2 = TFT_SILVER;
       boja_tS2(TFTbojaIsklj);
       relejWrite(relejKamen, isklj);
    }
  }
  else{
     Blynk.setProperty(V12, "color", BLYNK_DARK_GRAY); //ako je klizač na minimalnoj vrijednosti grijač kamena je isključen
     TFT_tempS2 = TFT_SILVER;
     boja_tS2(TFTbojaIsklj);
     relejWrite(relejKamen, isklj);
  }
  
  

//==========================================

//===== kontrola vlage vivarja =============
  if (!ahtOK) {
     // senzor neispravan - iz sigurnosnih razloga maglica je uvijek isključena
     Blynk.setProperty(V13, "color", BLYNK_GRAY);
     TFT_vlagaS1 = TFT_SILVER;
     boja_vS1(TFTbojaIsklj);
     relejWrite(relejVlaga, isklj);
  }
  else if(vlagaVivarij>minVlagaVivarij){
      if(vlagaS1<(vlagaVivarij-odstupanje)){
        Blynk.setProperty(V13, "color", BLYNK_RED);
        TFT_vlagaS1 = TFT_RED;
        boja_vS1(TFTbojaUklj);
        relejWrite(relejVlaga, uklj);
      }
      else if(vlagaS1>(vlagaVivarij+odstupanje)){
        Blynk.setProperty(V13, "color", BLYNK_DARK_GREEN);
        TFT_vlagaS1 = TFT_YELLOW;
        boja_vS1(TFTbojaUredu);
        relejWrite(relejVlaga, isklj);
      }
  }
  else{
     Blynk.setProperty(V13, "color", BLYNK_DARK_GRAY);//ako je klizač na minimalnoj vrijednosti maglica je isključena
     TFT_vlagaS1 = TFT_SILVER;
     boja_vS1(TFTbojaIsklj);
     relejWrite(relejVlaga, isklj);
  }
  

//==========================================

//===== kontrola ventilatora ===============
  ventAni = false;        // <---------------------------
  tempAlarmS1 = false;    // <   sve bi trebalo biti OK |
  vlagaAlarm = false;     // <---------------------------
  
  if(tempS1>(maxTempVivarij+odstupanje)){
    
    ventAni = true;
    tempAlarmS1 = true; 
    relejWrite(relejVentilator, uklj);
    Blynk.virtualWrite(V23, 1);
    if(!porukaTempVivarijPoslana){
      vrijemeSad=millis();
      if(vrijemeSad>(vrijemePorukeTemp+vrijemePoruke)){
         //Blynk.notify("Vivarij: temperatura veća od maksimalne");
         Blynk.logEvent("vivarij_temperatura", "Vivarij: temperatura veća od maksimalne");
         porukaTempVivarijPoslana = true;
         vrijemePorukeTemp=vrijemeSad;
      }
    }
  }
  else if(tempS1<(minTempVivarij-odstupanje) && (tempVivarij>minTempVivarij)){
    tempAlarmS1 = true;
    if(!porukaTempVivarijPoslana){
      vrijemeSad=millis();
      if(vrijemeSad>(vrijemePorukeTemp+vrijemePoruke)){
        //Blynk.notify("Vivarij: temperatura manja od minimalne. Provjeri grijač!");
        Blynk.logEvent("vivarij_temperatura", "Vivarij: temperatura manja od minimalne. Provjeri grijač!");
        porukaTempVivarijPoslana = true;
        vrijemePorukeTemp=vrijemeSad;
      }
    }
  }
  else if(vlagaS1>(maxVlagaVivarij+odstupanje)){
    ventAni = true;
    vlagaAlarm = true;
    relejWrite(relejVentilator, uklj);
    Blynk.virtualWrite(V23, 1);
    if(!porukaVlagaVivarijPoslana){
      vrijemeSad=millis();
      if(vrijemeSad>(vrijemePorukeVlaga+vrijemePoruke)){
        //Blynk.notify("Vivarij: vlažnost veća od maksimalne");
        Blynk.logEvent("vivarij_vlaga", "Vivarij: vlažnost veća od maksimalne");
        porukaVlagaVivarijPoslana = true;
        vrijemePorukeVlaga=vrijemeSad;
      }
    }
  }
  else if(vlagaS1<(minVlagaVivarij-odstupanje) && (vlagaVivarij>minVlagaVivarij)){
    vlagaAlarm = true;
    if(!porukaVlagaVivarijPoslana){
      vrijemeSad=millis();
      if(vrijemeSad>(vrijemePorukeVlaga+vrijemePoruke)){
        //Blynk.notify("Vivarij: vlažnost manja od minimalne. Provjeri vodu!");
        Blynk.logEvent("vivarij_vlaga", "Vivarij: vlažnost manja od minimalne. Provjeri vodu!");
        porukaVlagaVivarijPoslana = true;
        vrijemePorukeVlaga=vrijemeSad;
      } 
    }
  }
  else if(ventilatorNonStop){
    ventAni = true;
    relejWrite(relejVentilator, uklj);
    Blynk.virtualWrite(V23, 1);
  }
  else{
    relejWrite(relejVentilator, isklj);
    Blynk.virtualWrite(V23, 0);
    porukaVlagaVivarijPoslana = false;
    porukaTempVivarijPoslana = false;
  }
 
//======== ISPIS NA DISPLEJ ==================================
    if(samoPrviput)
      {
        tft.fillRoundRect(0, 0, 240, 25, 10, TFT_BROWN);
        tft.drawRoundRect(0, 0, 240, 25, 10, TFT_RED);
        tft.fillRoundRect(0, 26, 240, 214, 12, TFT_DARKGREEN);
        tft.drawRoundRect(0, 26, 240, 214, 12, TFT_GREEN);

        samoPrviput = false;
      }
    ledcWrite(BLK_PIN, BLK_DANnoc);
    tft.setTextDatum(TR_DATUM); // osiguraj ispravno sidrište teksta (top-right) -
                                 // prijašnji ekrani za spajanje na WiFi ostave
                                 // datum na C_BASELINE pa ga ovdje resetiramo
    tft.setFreeFont(&FreeSans9pt7b);
    tft.setTextColor(TFT_YELLOW, TFT_BROWN);
    tft.setTextSize(1);
    tft.drawString("Parina kucica", 170, 5, 1);

    // ---- indikator statusa senzora (crveno = kvar, prazno = OK) ----
    tft.fillRect(0, 2, 65, 20, TFT_BROWN); // očisti prethodni tekst
    if (!ahtOK || !dsOK || !pcfOK) {
      String greska = "";
      if (!ahtOK) greska += "AHT ";
      if (!dsOK)  greska += "DS ";
      if (!pcfOK) greska += "REL";
      tft.setTextColor(TFT_RED, TFT_BROWN);
      tft.drawString(greska, 65, 5, 1);
    }
    
    int t=tempS1+0.5;
    tft.setTextSize(2);
    tft.setTextColor(TFT_GREEN, TFT_DARKGREEN);
    tft.drawString(String(t), 145, 45, 7);
    tft.setTextSize(1);
    tft.drawString("`C", 170, 45, 4 );
    tft.setTextColor(TFT_tempS1, TFT_DARKGREEN);
    //tft.drawString(String(tempVivarij), 165, 100, 4);
    tft.drawString("`C", 190, 115, 2);
    img.createSprite(32, 32);
    img.fillRect(0, 0, 32, 32, TFT_DARKGREEN);
    img.pushSprite(145, 115);
    img.deleteSprite();
    miniFont(145, 120, boja_tempS1, tempVivarij); 
    
    int v=vlagaS1+0.5;
    tft.setTextColor(TFT_SKYBLUE, TFT_DARKGREEN);
    tft.drawString(String(v), 70, 175, 7);
    tft.drawString("%", 90, 175, 4);
    tft.setTextColor(TFT_vlagaS1, TFT_DARKGREEN);
    //tft.drawString(String(vlagaVivarij), 100, 157, 4);
    img.createSprite(32, 32);
    img.fillRect(0, 0, 32, 32, TFT_DARKGREEN);
    img.pushSprite(73, 199);
    img.deleteSprite();
    miniFont(76, 201, boja_vlagaS1, vlagaVivarij);
    tft.drawString("%", 115, 200, 2);
    
    int tk=tempS2+0.5;
    tft.setTextColor(TFT_ORANGE, TFT_DARKGREEN);
    tft.drawString(String(tk), 188, 175, 7);
    tft.drawString("`C", 216, 175, 4 );
    tft.setTextColor(TFT_tempS2, TFT_DARKGREEN);
    //tft.drawString(String(tempKamen), 100, 213, 4);
    miniFont(194, 201, boja_tempS2, tempKamen);
    tft.drawString("`C", 237, 200, 2);

    if(tempAlarmS1) tft.pushImage(145, 115, alertWidth, alertHeight, alert); // tempS1 alarm
    if(vlagaAlarm) tft.pushImage(73, 199, alertWidth, alertHeight, alert); // vlaga alarm

    
}


void miniFont(int fromLeft, int fromTop, int colors[], int number)
{
  int sizee=4;   //size of each box
  byte space=0 ; // space between boxes
  int Round=0;
  String n=String(number);
  
  for(int z=0;z<n.length();z++)
   for(int i=0;i<5;i++)
     for(int j=0;j<3;j++)
      {
        String c=n.substring(z, z+1);
        int b=c.toInt();
        tft.fillRoundRect((z*(sizee*4))+fromLeft+(j*sizee)+(j*space),fromTop+(i*sizee)+(i*space),sizee,sizee,Round,colors[arr[b][i][j]]);
      }
}

void boja_tS1(int Colour[])
{
  for(int i=0; i<2; i++)
    {
      boja_tempS1[i]=Colour[i];
    }
  
}

void boja_vS1(int Colour[])
{
  for(int i=0; i<2; i++)
    {
      boja_vlagaS1[i]=Colour[i];
    }
  
}

void boja_tS2(int Colour[])
{
  for(int i=0; i<2; i++)
    {
      boja_tempS2[i]=Colour[i];
    }
  
}
//====================================================================================
//====================================================================================
//====================================================================================

void setup()
{
  Serial.begin(115200);
  Serial.println("=== BOOT: setup() start ===");
  pinMode(Erasing_button, INPUT_PULLUP);

//------------- TFT init ---------------------------

  #ifdef ESP8266
    // još ništa
  #else if ESP32
    // configure LED PWM functionality (novi ESP32 core 3.x API)
    bool ledcOK = ledcAttach(BLK_PIN, freq, resolution);
    Serial.print("ledcAttach (pozadinsko svjetlo): ");
    Serial.println(ledcOK ? "OK" : "NEUSPJELO");
  #endif
  
  Serial.println("Pokrećem tft.init()...");
  tft.init();
  Serial.println("tft.init() gotovo");
  tft.setRotation(3);
  tft.setSwapBytes(true);
  ledcWrite(BLK_PIN, BLK_DANnoc);
  Serial.println("Pozadinsko svjetlo postavljeno, crtam test ekran");
  tft.fillScreen(TFT_RED);
  Serial.println("=== Ako vidiš CRVEN ekran, TFT+pozadinsko svjetlo rade ===");
  
//------------- PCF init ---------------------------
  pcfOK = pcf.begin(0x20, &Wire);
  if (!pcfOK) {
    Serial.println("Greška na PCF8574 - nastavljam bez relejnog modula, pokušat ću ponovno kasnije");
  } else {
    for (uint8_t p=0; p<8; p++) {
      pcf.pinMode(p, OUTPUT);
      pcf.digitalWrite(p, isklj);
    }
  }

//------------- AHT init ---------------------------
  ahtOK = aht.begin();
  if (!ahtOK) {
    Serial.println("Greška na AHT20 - nastavljam bez tog senzora, pokušat ću ponovno kasnije");
  }

//-------------------------------------------------- 

//------- brisanje vjerodajnica iz EEPROMa ---------
    
  tft.fillRect(0, 0, 240, 240, TFT_BLACK);
  tft.setTextDatum(C_BASELINE);
  tft.setFreeFont(&FreeSans12pt7b);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.setTextSize(1);
  tft.drawString("Brisanje EEPROMa?", 120, 30, 1);

  // Non-blocking odbrojavanje (millis umjesto delay) - tipka se
  // provjerava kontinuirano tijekom cijelog odbrojavanja, ne samo na kraju
  bool eraseRequested = false;
  const unsigned long COUNTDOWN_MS = 6000;
  unsigned long countdownStart = millis();
  int lastShown = -1;

  while (millis() - countdownStart < COUNTDOWN_MS) {
    int secondsLeft = 5 - (int)((millis() - countdownStart) / 1000);
    if (secondsLeft != lastShown) {
      lastShown = secondsLeft;
      Serial.println(secondsLeft);
      tft.setTextSize(2);
      tft.drawString(String(secondsLeft), 120, 180, 7);
    }
    if (digitalRead(Erasing_button) == LOW) {
      eraseRequested = true;
      break;
    }
  }

  // Press and hold the button to erase all the credentials
  if (eraseRequested)
  {
    tft.fillScreen(TFT_RED);
    tft.setTextColor(TFT_YELLOW, TFT_RED);
    tft.setTextSize(1);
    tft.drawString("BRISANJE", 120, 50, 1);
    tft.drawString("EEPROMa", 120, 100, 1);
    
    Credentials.Erase_eeprom();

  }
  tft.fillScreen(TFT_BLACK);
  

  //String auth_string = Credentials.EEPROM_Config();
  //auth_string.toCharArray(auth_token, 33);
  Credentials.EEPROM_Config();
  tft.fillRoundRect(0, 0, 240, 25, 10, TFT_SKYBLUE);
  tft.drawRoundRect(0, 0, 240, 25, 10, TFT_WHITE);
  tft.fillRoundRect(0, 26, 240, 214, 12, TFT_SKYBLUE);
  tft.drawRoundRect(0, 26, 240, 214, 12, TFT_WHITE);
  tft.setTextDatum(C_BASELINE);
  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextColor(TFT_WHITE, TFT_SKYBLUE);
  tft.setTextSize(1);
  tft.drawString("Spajam se na:", 120, 17, 1);
  tft.setTextSize(2);
  tft.drawString(Credentials.ssid, 120, 80, 1);
  

  if (Credentials.credentials_get())
  {
    Blynk.config(BLYNK_AUTH_TOKEN);
    connected_to_internet = 1;
  }
  else
  {
    tft.fillRoundRect(0,0, 240, 240, 12, TFT_RED);
    tft.setTextColor(TFT_YELLOW, TFT_RED);
    tft.setTextSize(1);
    tft.drawString("Spajanje na ", 120, 17, 1);
    tft.setTextSize(2);
    tft.drawString(Credentials.ssid, 120, 120, 1);
    tft.setTextSize(1);
    tft.drawString(" nije uspjelo!", 120, 150, 1);
    {
      unsigned long waitStart = millis();
      while (millis() - waitStart < 3000) {
        // pauza prije prikaza hotspot podataka (millis umjesto delay)
      }
    }
    tft.fillRect(0, 0, 240, 240, TFT_BLACK);
    tft.fillRoundRect(0, 0, 240, 25, 10, TFT_SKYBLUE);
    tft.drawRoundRect(0, 0, 240, 25, 10, TFT_WHITE);
    tft.fillRoundRect(0, 26, 240, 214, 12, TFT_SKYBLUE);
    tft.drawRoundRect(0, 26, 240, 214, 12, TFT_WHITE);
    tft.setTextDatum(C_BASELINE);
    tft.setTextColor(TFT_WHITE, TFT_SKYBLUE);
    tft.setTextSize(1);
    tft.drawString("Pokrecem hotspot:", 120, 17, 1);
    tft.setTextSize(2);
    tft.drawString(esp_ssid, 120, 120, 2);
    tft.setTextSize(2);
    String WiFiIP = WiFi.softAPIP().toString().c_str();
    tft.drawString(WiFiIP, 120, 160, 2);
    
    Credentials.setupAP(esp_ssid, esp_pass);
    connected_to_internet = 0;
  }

  if (connected_to_internet)
  {
    //Blynk.notify("Vivarij kontroler je aktivan!");
    Blynk.logEvent("vivarij_aktivan", "Vivarij: kontroler je aktivan!");
    tft.fillRoundRect(0, 0, 240, 25, 10, TFT_SKYBLUE);
    tft.drawRoundRect(0, 0, 240, 25, 10, TFT_WHITE);
    tft.fillRoundRect(0, 26, 240, 214, 12, TFT_SKYBLUE);
    tft.drawRoundRect(0, 26, 240, 214, 12, TFT_WHITE);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_SKYBLUE);
    tft.drawString("Spojen na:", 120, 17, 1);
    tft.setTextSize(2);
    tft.drawString(Credentials.ssid, 120, 80, 1);
    img.createSprite(110, 110);
    img.fillRect(0, 0, 110, 110, TFT_SKYBLUE);
    img.fillCircle(55, 55, 55, TFT_BLACK);
    img.fillCircle(55, 55, 50, TFT_YELLOW);
    img.fillEllipse(35, 45, 10, 20, TFT_BLACK);
    img.fillEllipse(75, 45, 10, 20, TFT_BLACK);
    Credentials.fillArc(55, 55, 100, 160, 40, 35, 5, TFT_BLACK);
    img.pushSprite(65, 95);
    img.deleteSprite();

    // ---- IP adresa na dnu ekrana ----
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_SKYBLUE);
    tft.drawString(WiFi.localIP().toString(), 120, 230, 2);
  

/*------------ SET WIDGETS VALUES --------------------*/
// temperatura vivarija
    Blynk.setProperty(V11, "label", "VIVARIJ °C");
    Blynk.setProperty(V11, "color", BLYNK_DARK_GREEN);
    Blynk.setProperty(V11, "min", minTempVivarij);
    Blynk.setProperty(V11, "max", maxTempVivarij);

// vlažnost vivarija
    Blynk.setProperty(V13, "label", "MAGLICA %");
    Blynk.setProperty(V13, "color", BLYNK_DARK_GREEN);
    Blynk.setProperty(V13, "min", minVlagaVivarij);
    Blynk.setProperty(V13, "max", maxVlagaVivarij);

//temperatura kamena
    Blynk.setProperty(V12, "label", "KAMEN °C");
    Blynk.setProperty(V12, "color", BLYNK_DARK_GREEN);
    Blynk.setProperty(V12, "min", minTempKamen);
    Blynk.setProperty(V12, "max", maxTempKamen);

//ventilator
    Blynk.setProperty(V23, "onLabel", "Vent A");

    sensors.begin();
    sensors.setResolution(10);
  
    timer.setInterval(3000L, sensorRead);

//------------- WEB OTA (nadogradnja firmvera) ---------
    // Stranica dostupna na http://<IP adresa ESP-a>/update
    // VAŽNO: promijeni zadanu lozinku prije korištenja!
    Credentials.beginOTA("admin", "vivarij123");
  }
}


//====================================================================================
//====================================================================================
//====================================================================================


void loop()
{
  Credentials.server_loops();

  if (connected_to_internet)
  {

    // .  Write the loop part of your code
    //
    // .             HERE

    Blynk.run();
    timer.run();
    Credentials.otaLoop();

    

    if(!samoPrviput)
      {
        //imgLoop.createSprite(55, 55);
        if(ventAni)
          {
            if(millis()-frameTime>100)
              {
                frameTime=millis();
                if(aniFrame>=frames) aniFrame=0;
                tft.pushImage(179, 42, animation_width, animation_height, fan2[aniFrame]);
                aniFrame++;
              }
          }
        else
          {
            if(DANnoc)
              {
                tft.pushImage(179, 42, dayWidth, dayHeight, sun);
                BLK_DANnoc = BLK_DAN; //intenzitet pozadinskog svijetla displeja po danu
              }
            else
              {
                tft.pushImage(179, 42, nightWidth, nightHeight, night);
                BLK_DANnoc = BLK_NOC; //intenzitet pozadinskog svijetla displeja po noći 
              }
          }
        //imgLoop.deleteSprite();
      }
    
  }
}
