/***************************************/
/******** Programme simulation   ********/
/***************************************/
#include "settings.h"
#ifdef simulation
#include "simulation.h"
#include <Arduino.h>

int temps = 0;
int itemps = 0;
#define ond 300

void RASimulationClass::imageMesure(int tempo)
{
  if (temps < 50)
    intensiteBatterie = (-10 + 1.0 * random(ond / 2) / 10) / 10;
  if ((temps >= 50) && (temps < 150))
  {
    itemps++;
    intensiteBatterie = (100 - itemps + 1.0 * random(ond) / 10) / 10;
  }
  if ((temps >= 150) && (temps < 250))
  {
    itemps++;
    intensiteBatterie = (itemps + 1.0 * random(ond) / 10) / 10;
  }
  if ((temps >= 250) && (temps < 350))
    intensiteBatterie = (100 + 1.0 * random(ond / 2) / 10) / 10;
  if ((temps >= 350) && (temps < 450))
    intensiteBatterie = (50 + 1.0 * random(ond / 2) / 10) / 10;
  if ((temps >= 450) && (temps < 550))
    intensiteBatterie = (-100 + 1.0 * random(ond / 2) / 10) / 10;
  temps++;
  if (temps > 550)
  {
    temps = 0;
    itemps = 0;
  }
  delay(tempo);
  capteurTension = (57.0 * 100 + random(100)) / 100;
  temperatureEauChaude = (20.0 * 100 + random(100)) / 100;
  puissanceDeChauffe = (10 * puissanceDeChauffe + random(1000)) / 11;

  //  capteurTemp=25;
}

RASimulationClass RASimulation;
#endif