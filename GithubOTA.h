#ifndef GITHUB_OTA_H
#define GITHUB_OTA_H

#include <Arduino.h>

/*
  Provjerava sadržaj datoteke "version.txt" na GitHubu i uspoređuje je
  s trenutnom (kompajliranom) verzijom firmvera. Ako je dostupna
  novija verzija, preuzima .bin datoteku i flasha je preko ugrađenog
  ESP32 HTTPUpdate mehanizma (isti sigurnosni mehanizam kao ElegantOTA -
  ako preuzimanje/flash ne uspije, stari firmver ostaje netaknut).

  Poziva se SINKRONO (blokira dok traje) iz glavnog loop()/timer
  konteksta - NE iz async callbacka - pa je crtanje po TFT-u unutar
  ove funkcije sigurno (za razliku od ElegantOTA async callbackova).

  Vraća true samo ako je nadogradnja pronađena i POKRENUTA (u tom
  slučaju se ESP32 automatski restarta nakon uspjeha i ova funkcija
  se praktički nikad ne vrati). Vraća false ako nema novije verzije,
  nema WiFi veze, ili je provjera/preuzimanje neuspjelo.
*/
bool githubCheckAndUpdate(const char* currentVersion,
                           const char* versionUrl,
                           const char* firmwareUrl);

#endif
