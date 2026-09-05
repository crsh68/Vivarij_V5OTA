# Vivarij_V5OTA

ESP32 kontroler za vivarij — mjeri i regulira temperaturu i vlažnost, upravlja grijačima/maglicom/ventilatorom preko relejnog modula, prikazuje status na TFT displeju, te se upravlja i nadograđuje na daljinu preko [Blynk](https://blynk.io/) aplikacije i web OTA nadogradnje firmvera.

## Značajke

- Očitanje temperature/vlažnosti vivarija (AHT20) i temperature "kamena" (DS18B20)
- Automatska regulacija grijača, maglice i ventilatora preko relejnog modula (PCF8574)
- Prikaz stanja na TFT displeju (ST7789 240×240)
- Upravljanje i nadzor preko Blynk aplikacije
- **Otporno na kvar senzora** — ako neki senzor ili relejni modul otkaže, uređaj ne staje: prelazi u sigurno stanje (isključi grijače/maglicu vezane za taj senzor), prikazuje grešku na ekranu i periodički pokušava oporavak
- Unos WiFi/Blynk podataka preko preglednika (bez potrebe za USB kabelom pri prvom postavljanju)
- **Web OTA nadogradnja firmvera** preko preglednika (`http://<IP uređaja>/update`), bez potrebe za USB kabelom

## Hardver

| ESP32 pin | Spojeno na |
|---|---|
| 3V3 | Vcc (displej, DS18B20, PCF8574, AHT20) |
| GND | GND (svi moduli) |
| D15 | DC (displej) |
| D4  | RES (displej) |
| D5  | BLK (pozadinsko svjetlo displeja) |
| D18 | SCK (displej) |
| D23 | SDA/MOSI (displej) |
| D19 | CS (displej) |
| D21 | SDA (I2C — PCF8574, AHT20) |
| D22 | SCL (I2C — PCF8574, AHT20) |
| D16 | Tipka za brisanje EEPROM-a (pull-up) |

Displej: **GMT130**, ST7789 driver, 240×240, SPI.

## Potrebne biblioteke (Arduino Library Manager)

| Biblioteka | Autor |
|---|---|
| Blynk | Blynk (klasična biblioteka, koristi `BlynkSimpleEsp32.h` — ne "Blynk IoT/Edgent" predložak) |
| TFT_eSPI | Bodmer |
| Adafruit AHTX0 | Adafruit |
| Adafruit PCF8574 | Adafruit |
| OneWire | Paul Stoffregen |
| DallasTemperature | Miles Burton |
| ESPAsyncWebServer | mathieucarbou / ESP32Async (**ne** originalna me-no-dev verzija — nekompatibilna s ESP32 core 3.x) |
| AsyncTCP | mathieucarbou / ESP32Async (mora odgovarati gornjem forku) |
| WebSockets | Markus Sattler (Links2004) |
| ArduinoJson | Benoit Blanchon |
| ElegantOTA | ayushsharma82 (v3.1.0+) |

## Postavljanje prije prvog uploada

1. **Blynk podaci** — u `Vivarij_V5OTA.ino` zamijeni:
   ```cpp
   #define BLYNK_TEMPLATE_ID "YOUR_BLYNK_TEMPLATE_ID"
   #define BLYNK_TEMPLATE_NAME "YOUR_BLYNK_TEMPLATE_NAME"
   #define BLYNK_AUTH_TOKEN "YOUR_BLYNK_AUTH_TOKEN"
   ```
   svojim vrijednostima iz Blynk konzole (My Templates → tvoj template). **Ove vrijednosti nikad ne commitaj u javni repozitorij** — ako repo dijeliš javno, drži ih lokalno ili u `.gitignore`-anoj datoteci.

2. **TFT_eSPI konfiguracija** — kopiraj `extras/TFT_eSPI_User_Setup/Setup300_Vivarij32_GMT130.h` u:
   ```
   Documents\Arduino\libraries\TFT_eSPI\User_Setups\
   ```
   pa u `User_Setup_Select.h` (u istom library folderu) zakomentiraj zadani `#include <User_Setup.h>` i dodaj:
   ```cpp
   #include <User_Setups/Setup300_Vivarij32_GMT130.h>
   ```
   (Detalji i razlog svake postavke su komentirani unutar te datoteke.)

3. **ElegantOTA async mode** — u `Documents\Arduino\libraries\ElegantOTA\src\ElegantOTA.h` promijeni:
   ```cpp
   #define ELEGANTOTA_USE_ASYNC_WEBSERVER 1   // zadano je 0
   ```
   (Arduino IDE ne podržava globalne build-flagove kao PlatformIO, pa se ovo mora ručno postaviti unutar same biblioteke — ponovi nakon svakog ažuriranja ElegantOTA-e.)

4. **OTA lozinka** — u `setup()` promijeni zadanu lozinku prije prve upotrebe:
   ```cpp
   Credentials.beginOTA("admin", "vivarij123");
   ```

## Prvo pokretanje

Pri prvom pokretanju (ili nakon brisanja EEPROM-a držanjem tipke na D16 tijekom odbrojavanja) uređaj otvara vlastitu WiFi pristupnu točku (`Vivarij40`) — spoji se na nju mobitelom/računalom i unesi podatke svoje kućne WiFi mreže preko web stranice koja se automatski otvori.

## Nadogradnja firmvera preko OTA

Nakon što je uređaj spojen na WiFi:
1. Arduino IDE → **Sketch → Export Compiled Binary**
2. Otvori `http://<IP adresa uređaja>/update` u pregledniku (IP se ispisuje na TFT ekranu nakon spajanja i u Serial Monitoru)
3. Odaberi izvezenu `.bin` datoteku i uploadaj

## Licenca

Vidi `LICENSE` (MIT).
