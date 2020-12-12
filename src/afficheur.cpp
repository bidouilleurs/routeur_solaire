/***************************************/
/******** gestion afficheur   ********/
/***************************************/
#include "settings.h"
#ifdef EcranOled
#include "Arduino.h"
#include "afficheur.h"
#include <Wire.h> // Only needed for Arduino 1.6.5 and earlier
#include <WiFi.h> // legacy: #include "SSD1306.h"

int teAff = 0;

#ifndef HELTEC
#include "SSD1306Wire.h" // legacy: #include "SSD1306.h"
SSD1306Wire display(0x3c, SDA, SCL);

void RAAfficheurClass::setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  // Initialising the UI will init the display too.
  display.init();

  //  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  cls();
  affiche(13, "bidouilleurs");
  affiche(23, "Reseau");
  affiche(31, "   -tonome");
#ifdef WifiServer
  if (!SAP)
    affiche(39, WiFi.localIP().toString());
  else
    affiche(39, WiFi.softAPIP().toString());
#endif

#ifdef Bluetooth
  affiche(50, "Bluetooth");
#endif
  delay(2000);
}

void RAAfficheurClass::cls()
{

  display.clear();
  display.setTextAlignment(TEXT_ALIGN_RIGHT);
  delay(500);
}

void RAAfficheurClass::affiche(int li, String a)
{
  Serial.println(a);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  //  display.drawString(35, li, a);
  display.drawString(32, li - 13, a);
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

    display.drawProgressBar(32, 50 - 13, 60, 10, abs(puissanceGradateur) / 10);
    // draw the percentage as String
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    display.drawString(32, 13 - 13, "I = " + String(intensiteBatterie) + " A  ");
    display.drawString(32, 24 - 13, "U = " + String(capteurTension) + " V  ");
#ifdef MesureTemperature
    display.drawString(32, 35 - 13, "T = " + String(temperatureEauChaude) + " °C  ");
#endif
    /*
    // affichage à l'envers
    display.drawProgressBar(35, 50, 60, 10, abs(puissanceGradateur) / 10);
    // draw the percentage as String
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    display.drawString(35, 13, "I = " + String(intensiteBatterie) + " A  ");
    display.drawString(35, 24, "U = " + String(capteurTension) + " V  ");
    display.drawString(35, 35, "T = " + String(temperatureEauChaude) + " °C  ");
    // display.drawString(1, 39, "Puis = " + String(puissanceMono) + " W  ");
    // write the buffer to the display
 */
    display.display();
    teAff = 0;
  }
  teAff++;
}
#endif

/************************************************/
/*********** pour la heltec *********************/
/************************************************/
#ifdef HELTEC
#include "heltec.h"

void RAAfficheurClass::setup()
{
  Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Disable*/, true /*Serial Enable*/);
  Heltec.display->setContrast(125);
  cls();
  affiche(1, "Les bidouilleurs");
  affiche(15, "Reseautonome");
#ifdef WifiServer
  if (!SAP)
    affiche(32, WiFi.localIP().toString());
  else
    affiche(32, WiFi.softAPIP().toString());
#endif

#ifdef Bluetooth
  affiche(44, "Bluetooth");
#endif
}

void RAAfficheurClass::cls()
{

  for (int16_t i = 0; i < DISPLAY_HEIGHT / 2; i += 2)
  {
    Heltec.display->drawRect(i, i, DISPLAY_WIDTH - 2 * i, DISPLAY_HEIGHT - 2 * i);
    Heltec.display->display();
    //    delay(10);
  }
  Heltec.display->clear();
  Heltec.display->display();
  delay(500);
  Heltec.display->clear();
  Heltec.display->display();
}

void RAAfficheurClass::affiche(int li, String a)
{
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->setFont(ArialMT_Plain_16);
  Heltec.display->drawString(0, li, a);
  // write the buffer to the display
  Heltec.display->display();
}

void RAAfficheurClass::affichage_oled()
{
  if (teAff >= 5)
  {
    Heltec.display->clear();
    Heltec.display->drawProgressBar(0, 53, 120, 10, abs(puissanceGradateur) / 10);
    // draw the percentage as String
    Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
    Heltec.display->setFont(ArialMT_Plain_10);
    Heltec.display->drawString(4, 0, "I_bat = " + String(intensiteBatterie) + " A  ");
    Heltec.display->drawString(4, 13, "U_bat = " + String(capteurTension) + " V  ");
    Heltec.display->drawString(4, 27, "T°   = " + String(temperatureEauChaude) + " °C  ");
    //  Heltec.display->drawString(4, 39, "Puis = " + String(puissanceMono) + " W  ");
    // write the buffer to the display
    Heltec.display->display();
    teAff = 0;
  }
  teAff++;
}

#endif // heltec

RAAfficheurClass RAAfficheur;
#endif
