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

1. **Blynk Template ID/Name** — u `Vivarij_V5OTA.ino` zamijeni:
   ```cpp
   #define BLYNK_TEMPLATE_ID "YOUR_BLYNK_TEMPLATE_ID"
   #define BLYNK_TEMPLATE_NAME "YOUR_BLYNK_TEMPLATE_NAME"
   ```
   svojim vrijednostima iz Blynk konzole (My Templates → tvoj template). Ovi podaci nisu tajni (ne daju sami po sebi pristup uređaju) pa mogu ostati u kodu.

   **Auth Token se NE stavlja u kod** — unosi se preko web sučelja pri prvom postavljanju i sprema se u EEPROM (vidi "Prvo pokretanje" ispod). Ovo je namjerno: EEPROM particija je odvojena od programske particije koju OTA/auto-update prepisuje, pa tvoj stvarni token ostaje netaknut čak i kad firmware preuzmeš s (javnog) GitHub repozitorija gdje token ne postoji.

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

Pri prvom pokretanju (ili nakon brisanja EEPROM-a držanjem tipke na D16 tijekom odbrojavanja) uređaj otvara vlastitu WiFi pristupnu točku (`Vivarij40`) — spoji se na nju mobitelom/računalom i unesi na web stranici koja se automatski otvori:
- SSID i lozinku svoje kućne WiFi mreže
- **Blynk Auth Token** (Blynk.Console → Devices → tvoj uređaj → Device Info)

Sve troje se sprema u EEPROM i preživljava svaki budući OTA/auto-update.

> ⚠️ **Ako nadograđuješ postojeći uređaj** koji je već radio sa starijom verzijom ovog projekta (prije nego je token premješten u EEPROM): tvoj EEPROM još nema spremljen auth token. Nakon uploada ove verzije, **jednom** drži tipku na D16 tijekom odbrojavanja da otvoriš AP portal i ponovno unesi WiFi + Blynk podatke — nakon toga se sve dalje pamti automatski kroz sve buduće nadogradnje.

## Nadogradnja firmvera preko OTA

Nakon što je uređaj spojen na WiFi:
1. Arduino IDE → **Sketch → Export Compiled Binary**
2. Otvori `http://<IP adresa uređaja>/update` u pregledniku (IP se ispisuje na TFT ekranu nakon spajanja i u Serial Monitoru)
3. Odaberi izvezenu `.bin` datoteku i uploadaj

## Automatski GitHub OTA update

Uređaj sam provjerava GitHub pri svakom spajanju na WiFi i zatim periodički (zadano svaka 24h, podesivo preko `GITHUB_CHECK_INTERVAL_MS`). Uspoređuje svoju kompajliranu verziju (`FW_VERSION`) sa sadržajem `firmware/version.txt` u repozitoriju — ako je različita, sam preuzima `firmware/Vivarij_V5OTA.ino.bin` i flasha se, pa se restarta.

**Prije prve upotrebe ove značajke:**
- U `Vivarij_V5OTA.ino` zamijeni `YOUR_GITHUB_USERNAME` u `GITHUB_VERSION_URL` i `GITHUB_FIRMWARE_URL` svojim GitHub korisničkim imenom
- Ako ne želiš automatski update, postavi `#define GITHUB_AUTOUPDATE_ENABLED false`

**Objavljivanje nove verzije** (svaki put kad želiš da se svi uređaji sami nadograde):
1. Poveća `#define FW_VERSION "1.0.0"` u `.ino` (npr. na `"1.0.1"`)
2. Arduino IDE → **Sketch → Export Compiled Binary**
3. Kopiraj generirani `.bin` u `firmware/Vivarij_V5OTA.ino.bin` (prepiši postojeći)
4. Upiši **istu** novu vrijednost u `firmware/version.txt` (npr. `1.0.1`, bez navodnika i bez novog reda na kraju)
5. `git add firmware/ Vivarij_V5OTA.ino && git commit -m "v1.0.1" && git push`

Svi uređaji koji su spojeni na WiFi preuzet će novu verziju sami, u roku od maksimalno `GITHUB_CHECK_INTERVAL_MS` (zadano 24h), ili odmah pri sljedećem spajanju/restartu.

**Sigurnosna napomena:** provjera verzije i preuzimanje koriste `client.setInsecure()` (bez provjere TLS certifikata) radi jednostavnosti — GitHub certifikati se s vremena na vrijeme mijenjaju pa bi "pinani" certifikat s vremenom prestao raditi bez održavanja. Kompromis je da netko na istoj WiFi mreži teoretski može pokušati podmetnuti lažni odgovor tijekom provjere; `Update.h` svejedno odbija skraćenu/oštećenu datoteku prije nego je flash, ali ne provjerava je li sadržaj *legitiman* firmware. Za veću sigurnost razmisli o pinanju GitHub root CA certifikata u `GithubOTA.cpp`.

## Blynk konfiguracija (datastreams / kontrole)

Blynk **nema** ugrađenu opciju za izvoz predloška u datoteku, pa se konfiguracija ne može commitati kao gotov fajl. Ako želiš iskoristiti gotov predložak (npr. za drugi uređaj), u Blynk.Console → **Templates** → tvoj template → izbornik → **Duplicate** napravi kopiju unutar samog Blynk sučelja.

Za rekreiranje predloška od nule (npr. ako netko drugi kloniraj ovaj repo), tablica ispod pokazuje koji virtualni pinovi (Datastreams) trebaju postojati, na temelju onoga što kod čita/piše:

| Pin | Smjer | Tip / widget | Namjena |
|---|---|---|---|
| V1 | app → uređaj | Switch | Dan/noć način rada |
| V4 | uređaj → app | Value display | Temperatura "kamena" (DS18B20) |
| V8 | uređaj → app | Value display | Temperatura vivarija (AHT20) |
| V9 | uređaj → app | Value display | Vlažnost vivarija (AHT20) |
| V10 | app → uređaj | Time Input | Raspored hranjenja (po danu u tjednu) |
| V11 | app ↔ uređaj | Slider (int) | Ciljana temperatura vivarija — kontrolira grijač |
| V12 | app ↔ uređaj | Slider (int) | Ciljana temperatura kamena — kontrolira grijač kamena |
| V13 | app ↔ uređaj | Slider (int) | Ciljana vlažnost vivarija — kontrolira maglicu |
| V20 | app → uređaj | Switch | UVB rasvjeta uklj/isklj |
| V23 | app ↔ uređaj | Switch | Ventilator (ručno / automatski način) |
| V25 | app → uređaj | Switch | Rasvjeta vivarija uklj/isklj |

Dodatno, `min`/`max`/`color`/`label` postavke za V11, V12, V13 i V23 se automatski postavljaju iz koda (`Blynk.setProperty(...)`) pri spajanju — u Blynk konzoli ih je dovoljno grubo postaviti (widget tip + raspon), kod će ih dalje sam finije podešavati.

Preporuka: nakon što ručno postaviš dashboard u Blynk.Console (Web Dashboard i/ili Mobile App), snimi screenshotove i spremi ih u `docs/blynk/` unutar ovog repozitorija (npr. `docs/blynk/web-dashboard.png`, `docs/blynk/mobile-dashboard.png`) — tako novi korisnik repozitorija odmah vizualno vidi kako bi trebalo izgledati, umjesto da nagađa iz same tablice.



Vidi `LICENSE` (MIT).
