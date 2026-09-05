// ============================================================
// Setup300_Vivarij32_GMT130.h
//
// Samostalna TFT_eSPI setup datoteka za projekt "Vivarij32"
// Displej: GMT130, ST7789 driver, 240x240, SPI
//
// KAKO KORISTITI:
// 1. Kopiraj ovu datoteku u:
//    Documents\Arduino\libraries\TFT_eSPI\User_Setups\
// 2. U datoteci User_Setup_Select.h (isti folder, jednu razinu
//    gore) zakomentiraj zadanu liniju:
//       //#include <User_Setup.h>
//    i dodaj/otkomentiraj:
//       #include <User_Setups/Setup300_Vivarij32_GMT130.h>
// 3. Sačuvaj i ovu datoteku i original zip projekta (ovaj fajl,
//    kao i sam .ino/.cpp, zajedno s izvornim kodom projekta),
//    jer se User_Setups folder gubi kad se TFT_eSPI biblioteka
//    ažurira/reinstalira preko Library Managera - onda samo
//    ponovno kopiraš ovu datoteku i ponovno urediš
//    User_Setup_Select.h kako je opisano gore.
// ============================================================

#define USER_SETUP_ID 300
#define USER_SETUP_INFO "Setup300_Vivarij32_GMT130"

// ------------------- DRIVER -------------------
#define ST7789_DRIVER

// ------------------- REZOLUCIJA -------------------
#define TFT_WIDTH  240
#define TFT_HEIGHT 240

// KLJUČNO za GMT130 / male 240x240 ST7789 panele - bez ovoga slika
// je pomaknuta izvan vidljivog područja i ekran ostaje prazan/crn.
#define CGRAM_OFFSET

// ------------------- PINOVI (prema shemi projekta Vivarij32) -------------------
#define TFT_MOSI 23   // D23 - SDA
#define TFT_SCLK 18   // D18 - SCK
#define TFT_CS   19   // D19 - CS
#define TFT_DC   15   // D15 - DC
#define TFT_RST   4   // D4  - RES

// TFT_BL namjerno NIJE definiran - pozadinsko svjetlo (D5) upravlja
// sam sketch preko ledcAttach()/ledcWrite(), pa TFT_eSPI ne smije
// dirati taj pin da ne dođe do sukoba dviju PWM konfiguracija.

// ------------------- REDOSLIJED BOJA -------------------
// GMT130 panel šalje boje u BGR redoslijedu - bez ovoga su
// crvena i plava zamijenjene.
#define TFT_RGB_ORDER TFT_BGR

// ------------------- FONTOVI -------------------
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF

#define SMOOTH_FONT

// ------------------- SPI BRZINA -------------------
#define SPI_FREQUENCY       40000000
#define SPI_READ_FREQUENCY  20000000
