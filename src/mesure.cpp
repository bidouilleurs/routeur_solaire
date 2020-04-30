/********************************************/
/******** Mesure du courant  tension  ********/
/********************************************/
#include "mesure.h"
#include "settings.h"
#include "triac.h"
#include <Arduino.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#ifdef Pzem04t
#include "PZEM004Tv30.h"
#endif
float Pinceav = 0;
int intPince = 0;
float pince2 = 0;
float pince1 = 0;
float tensionav = 0;
int cptimp = 0;
int afftemp = 500;
int affpzem = 5;

// GPIO where the DS18B20 is connected to
const int oneWireBus = pinTemp;
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);
#ifdef Pzem04t
PZEM004Tv30 pzem(&Serial2);
#endif
void RAMesureClass::setup()
{
  sensors.begin(); // demarrage de la sonde de temperature
}

void RAMesureClass::mesurePinceTension(int jmax, int imax)
{
  int inttension = analogRead(pinTension); // mesure de tension
  pince2 = 0;
  for (int j = 0; j < jmax; j++)
  { // boucle exterieure

    for (int i = 0; i < imax; i++)
    { // boucle interieur
      intPince = analogRead(pinPince) - analogRead(pinPinceRef);
      intensiteBatterie = intPince - routeur.zeropince;                     // decalage du zero
      intensiteBatterie = (intensiteBatterie)*routeur.coeffPince;           // applique le coeff
      intensiteBatterie = (intensiteBatterie + (imax - 1) * pince1) / imax; // intégration de la mesure
      pince1 = intensiteBatterie;
    }

    intensiteBatterie = ((jmax - 1) * pince2 + intensiteBatterie) / jmax;
    pince2 = intensiteBatterie;

    inttension = analogRead(pinTension);                // mesure de tension
    capteurTension = inttension * routeur.coeffTension; // applique le coeff
    tensionav = (capteurTension + (imax - 1) * tensionav) / imax;
  }

  capteurTension = tensionav;
  inttension = capteurTension * 100;
  capteurTension = inttension;
  capteurTension = capteurTension / 100; // arrondi à la 1er virgule
}

void RAMesureClass::mesureTemperature()
{
  afftemp++;
  if (afftemp < 500)
  {
    return;
  }
  else
  {
    afftemp = 0;
  }
#ifdef MesureTemperature

  RATriac.stop_interrupt();
  Serial.println("Mesure température");
  sensors.requestTemperatures();
  float temperatureC = sensors.getTempCByIndex(0);
  // float temperatureF = sensors.getTempFByIndex(0);
  if (temperatureC < 75)
  {
    temperatureEauChaude = temperatureC; // supprime les pics de mauvaise mesure
  }
  if (temperatureC == -127)
  {
    sensors.begin(); // redemarrage du dallas
  }
  RATriac.restart_interrupt();

#endif
}

void RAMesureClass::mesure_puissance()
{
  affpzem++;
  if (affpzem < 5)
    return;
  else
    affpzem = 0;
  puissanceDeChauffe = 0;
#ifdef Pzem04t
  Pzem_i = pzem.current();
  Pzem_U = pzem.voltage();
  Pzem_P = pzem.power();
  Pzem_W = pzem.energy();
  if (puissanceGradateur > 0)
  {
    puissanceDeChauffe = Pzem_P;
  }
  else
  {
    puissanceDeChauffe = 0;
  }
#endif
}
RAMesureClass RAMesure;