/***************************************/
/******** gestion afficheur   ********/
/***************************************/
#include "settings.h"
#ifdef EcranOled
#include "Arduino.h"
#include "afficheur.h"
#include <Wire.h> // Only needed for Arduino 1.6.5 and earlier
#include <WiFi.h>
#include "communication.h"
int teAff = 0;

#include "SSD1306Wire.h" // legacy: #include "SSD1306.h"
SSD1306Wire display(0x3c, SDA, SCL);

void RAAfficheurClass::setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  // Initialising the UI will init the display too.
  display.init();
  display.setBrightness(100);
#ifdef Ecran_inverse
  display.flipScreenVertically();
#endif
  display.setFont(ArialMT_Plain_10);
  cls();
  affiche(13, "bidouilleurs");
  affiche(23, "Reseau");
  affiche(31, "   -tonome");
#ifdef WifiServer
  if (!SAP)
    affiche(39, WiFi.localIP().toString().c_str());
  else
    affiche(39, WiFi.softAPIP().toString().c_str());
#endif
  delay(2000);
}

void RAAfficheurClass::cls()
{

  display.clear();
  display.setTextAlignment(TEXT_ALIGN_RIGHT);
  delay(500);
}

void RAAfficheurClass::affiche(int li, const char *a)
{
  RACommunication.print(3, a, true);

  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
#ifdef Ecran_inverse
  display.drawString(31, li, a);
#endif
#ifndef Ecran_inverse
  display.drawString(31, li - 13, a);
#endif
  // write the buffer to the display
  display.display();
}

int cliled = 0;
void RAAfficheurClass::affichage_oled()
{
  if (cliled == 1)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    cliled = 0;
  }
  else
  {
    digitalWrite(LED_BUILTIN, LOW);
    cliled = 1;
  }
  if (testwifi != 0)
  {
    digitalWrite(LED_BUILTIN, LOW);
  }

  if (teAff >= 5)
  {
    display.clear();
#ifndef Ecran_inverse

    display.drawProgressBar(32, 50 - 13, 60, 10, abs(puissanceGradateur) / 10);
    // draw the percentage as String
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    sprintf(intBattery, "I = %2.2f A", intensiteBatterie);
    sprintf(tension, "V = %2.2f V", capteurTension);
    sprintf(tempEau, "T = %2.2f °C", temperatureEauChaude);
    display.drawString(32, 13 - 13, intBattery);
    display.drawString(32, 24 - 13, tension);
    display.drawString(32, 35 - 13, tempEau);

#endif
#ifdef Ecran_inverse

    // affichage à l'envers
    display.drawProgressBar(35, 50, 60, 10, abs(puissanceGradateur) / 10);
    // draw the percentage as String
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    sprintf(intBattery, "I = %2.2f A", intensiteBatterie);
    sprintf(tension, "V = %2.2f V", capteurTension);
    sprintf(tempEau, "T = %2.2f °C", temperatureEauChaude);
    display.drawString(35, 13, intBattery);
    display.drawString(35, 24, tension);
    display.drawString(35, 35, tempEau);
    // display.drawString(1, 39, "Puis = " + String(puissanceMono) + " W  ");
    // write the buffer to the display
#endif

    display.display();
    teAff = 0;
  }
  teAff++;
}

RAAfficheurClass RAAfficheur;
#endif
