/***************************************/
/******** gestion afficheur   ********/
/***************************************/
#include "settings.h"
#ifdef EcranOled
#include "afficheur.h"
#include <Wire.h>        // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306Wire.h" // legacy: #include "SSD1306.h"
#include <WiFi.h> // legacy: #include "SSD1306.h"

int teAff = 0;
SSD1306Wire display(0x3c, SDA, SCL);

void RAAfficheurClass::setup()
{
  // Initialising the UI will init the display too.
  display.init();

  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  cls();
  affiche(13, "bidouilleurs");
  affiche(23, "Reseau");
  affiche(31, "   -tonome");
#ifdef WifiServer
  if (!SAP)
    affiche(42, WiFi.localIP().toString());
  else
    affiche(42, WiFi.softAPIP().toString());
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
  display.drawString(35, li, a);
  // write the buffer to the display
  display.display();
}

void RAAfficheurClass::affichage_oled()
{

  if (teAff >= 5)
  {
    display.clear();
    display.drawProgressBar(35, 50, 60, 10, abs(puissanceGradateur) / 10);
    // draw the percentage as String
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    display.drawString(35, 13, "I = " + String(intensiteBatterie) + " A  ");
    display.drawString(35, 24, "U = " + String(capteurTension) + " V  ");
    display.drawString(35, 35, "T = " + String(temperatureEauChaude) + " °C  ");
    // display.drawString(1, 39, "Puis = " + String(puissanceMono) + " W  ");
    // write the buffer to the display
    display.display();
    teAff = 0;
  }
  teAff++;
}

RAAfficheurClass RAAfficheur;
#endif