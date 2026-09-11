#include "GithubOTA.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>

bool githubCheckAndUpdate(const char* currentVersion,
                           const char* versionUrl,
                           const char* firmwareUrl)
{
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("GithubOTA: nema WiFi veze, preskačem provjeru");
    return false;
  }

  WiFiClientSecure client;
  // Preskače provjeru TLS certifikata (jednostavnije, GitHub certifikati
  // se rotiraju pa bi fiksni "pinani" certifikat s vremenom prestao raditi).
  // Kompromis: netko na istoj WiFi mreži teoretski može presresti/lažirati
  // odgovor GitHuba tijekom provjere verzije. Update.h svejedno provjerava
  // veličinu/MD5 preuzete datoteke prije nego je proglasi valjanom, pa
  // uređaj neće flashati skraćenu/oštećenu datoteku - ali ne štiti od
  // namjerno zlonamjernog (ali strukturno ispravnog) firmvera ubačenog
  // ovim putem. Za veću sigurnost, razmisli o "pinanju" GitHub root CA
  // certifikata umjesto setInsecure().
  client.setInsecure();

  HTTPClient http;
  Serial.println("GithubOTA: provjeravam najnoviju verziju na GitHubu...");
  if (!http.begin(client, versionUrl)) {
    Serial.println("GithubOTA: ne mogu otvoriti vezu za provjeru verzije");
    return false;
  }

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("GithubOTA: greška pri dohvaćanju verzije, HTTP kod %d\n", httpCode);
    http.end();
    return false;
  }

  String remoteVersion = http.getString();
  remoteVersion.trim();
  http.end();

  Serial.print("GithubOTA: trenutna verzija = ");
  Serial.println(currentVersion);
  Serial.print("GithubOTA: dostupna verzija = ");
  Serial.println(remoteVersion);

  if (remoteVersion.length() == 0 || remoteVersion == String(currentVersion)) {
    Serial.println("GithubOTA: već je najnovija verzija");
    return false;
  }

  Serial.println("GithubOTA: pronađena novija verzija, preuzimam i flasham...");

  // Ova funkcija se poziva SINKRONO iz glavnog loop()/timer konteksta
  // (ne iz async callbacka kao ElegantOTA), pa je blokira glavnu petlju
  // dok traje - sensorRead()/releji se u tom vremenu ne ažuriraju.
  httpUpdate.rebootOnUpdate(true); // automatski restart nakon uspješne nadogradnje

  t_httpUpdate_return ret = httpUpdate.update(client, firmwareUrl);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("GithubOTA: nadogradnja neuspjela (%d): %s\n",
                    httpUpdate.getLastError(),
                    httpUpdate.getLastErrorString().c_str());
      return false;
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("GithubOTA: server javlja da nema nadogradnje");
      return false;
    case HTTP_UPDATE_OK:
      Serial.println("GithubOTA: nadogradnja uspješna, restart...");
      return true; // u praksi se ovamo skoro nikad ne stigne - rebootOnUpdate
                    // uređaj već restarta prije nego se update() vrati
  }
  return false;
}
