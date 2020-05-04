/***************************************/
/******** pilotage et regulation   ********/
/***************************************/
#include "regulation.h"
#include "settings.h"
#include <Arduino.h>
float xmax = 0;
float xmin = 0;
int devlente = 0;
int devdecro = 0;
float tabxmin[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int itab = 0;

int RARegulationClass::mesureDerive(float x, float seuil)
{
  int dev = 0;

  if (x > xmax)
  {
    dev = 1;
    xmax = x;
    xmin = xmax - seuil;

  } // si dépasse le seuil haut déplacement du seuil haut et bas

  if (x < xmin)
  {
    if ((xmin - x) > 10 * seuil)
    {
      devdecro = 1;
    }
    else
    {
      devdecro = 0; // gestion du décrochage chute brutale du courant
    }
    dev = -1;
    xmin = x;
    xmax = xmin + seuil;

    for (int i = 0; i < 9; i++)
    {
      tabxmin[9 - i] = tabxmin[8 - i];
    }
    tabxmin[0] = xmin; // range les valeurs mini
    float deltaxmin = 0;
    float xminmoy = tabxmin[0];

    for (int i = 1; i < 10; i++)
    {
      deltaxmin += tabxmin[i] - tabxmin[0];
    } // calcule la pente négative
    //           for (int i=0;i<9;i++) { ondulxmin+=abs(tabxmin[i]-tabxmin[i+1]); }
    if (itab < 10)
    {
      itab++; // retire les 10 premiers points avant de calculer
    }
    if ((itab >= 10) && (deltaxmin > 10 * seuil))
    {
      devlente = 1;
    }
    else
    {
      devlente = 0;
    }
    //          if ((itab>=10) && (ondulxmin>40*seuil)) devforte=1; else devforte=0;
  } // si descend en dessous du seuil bas déplacement du seuil haut et bas
  return dev;
}

int calPuis = 0;
int calPuisav = 0;
int ineg = 0;
int devprevious = 0;
int devcount = 0;
int puisGradmax = 0;
int tesTension = 0;
#define variation_lente 1    // config 5
#define variation_normale 10 // config 10
#define variation_rapide 20  // config 20
#define bridePuissance 900   // sur 1000

int RARegulationClass::regulGrad(int dev)
{
  calPuisav = calPuis;
  if (puisGradmax < calPuis)
  {
    puisGradmax = calPuis; // puissance maximeum de fonctionnement
  }
  if ((intensiteBatterie < 0) && (intensiteBatterie > -routeur.toleranceNegative))
  {
    intensiteBatterie = intensiteBatterie + routeur.toleranceNegative; //correction des mesures proche de zéro
  }
  if (intensiteBatterie < 0)
  {
    devcount = 0;
  }
  if (devprevious == dev)
  {
    if (devcount < 100)
      devcount++;
  }
  else
  {
    devcount = 0;
  }
  if (dev == 0)
  {
    calPuis += variation_lente;
    if (devcount > 3)
    {
      calPuis += variation_normale;
    }
    if ((devcount > 7) && (calPuis < puisGradmax - 50))
    {
      calPuis += variation_rapide;
    }
  }
  if (dev > 0)
  {
    calPuis += variation_lente;
    if (devcount > 5)
    {
      calPuis += variation_normale;
    }
    if ((devcount > 7) && (calPuis < puisGradmax))
    {
      calPuis += variation_rapide;
    }
  }
  if ((dev < 0) || (intensiteBatterie < 0))
  {
    if (intensiteBatterie < 2)
    {
      calPuis -= variation_normale + variation_lente;
    }
    else
    {
      calPuis -= variation_rapide + variation_lente;
    }
    if (devcount > 2)
    {
      calPuis -= variation_normale;
    }
    if ((devcount > 5) && (intensiteBatterie > 2))
    {
      calPuis -= variation_rapide;
    }

    if (devlente == 1)
    {
      calPuis -= variation_normale;
    }
    //     if(devforte == 1) calPuis -= variation_normale;
    if (devdecro == 1)
    {
      calPuis -= variation_normale;
    }
  }
  if ((intensiteBatterie > 2) && (devlente == 0))
  {
    calPuis += variation_rapide;
  } // si la batterie commence à se décharger
  if ((intensiteBatterie < 2) && (intensiteBatterie > 0.2) && (devlente == 0))
  {
    calPuis += variation_normale;
  } // si la batterie commence à se décharger

  if (intensiteBatterie < 0)
  {
    calPuis = calPuisav - variation_rapide;
  } // si la batterie est déchargée complètement
  if (intensiteBatterie < 0)
  { // si le courant est trop longtemps négatif
    if (ineg < 100)
    {
      ineg++;
      puisGradmax -= variation_normale;
    }
    if (ineg > 5)
    {
      puisGradmax = 0;
      calPuis = 0;
    } // tolerance pic négatif
  }
  else
  {
    ineg = 0;
  }
  //   if (Pince<-1)  { puisGradmax=3*calPuis/4; calPuis=3*calPuis/4;}// autorisation de tirer 500mA max sur les batteries
  //  if (Pince<-1)  { puisGradmax=3*calPuis/4; calPuis=0;}// autorisation de tirer 500mA max sur les batteries

  devprevious = dev;
  if (capteurTension > routeur.seuilDemarrageBatterie + 0.2)
  {
    tesTension = 1;
  }
  if (capteurTension < routeur.seuilDemarrageBatterie - 0.5)
  {
    tesTension = 0;
  }
  if (tesTension == 0)
  {
    calPuis = 0;
  }
  calPuis = min(max(0, calPuis), bridePuissance);
  return (calPuis);
}

/**************************************/
/******** Pilotage exterieur*******/
/**************************************/
unsigned long tempdepart;
int tempo = 0;
int tempo2 = 0;

void RARegulationClass::pilotage()
{
  // pilotage du 2eme triac
#ifndef MesureTemperature
#ifdef Sortie2
#ifdef Pzem04t
  if (routeur.utilisation2Sorties)
  {
    if ((puissanceDeChauffe == 0) && (puissanceGradateur > 100))
      tempo2++;
    else
      tempo2 = 0; // demarre la tempo chauffe-eau temp atteinte
    if (tempo2 > 10)
    {
      choixSortie = 1;
      sortieActive = 2;
      tempo2++;
    } // après 2s avec i=0 bascule sur triac2
    if (tempo2 > 200)
    {
      choixSortie = 0;
      sortieActive = 1;
      tempo2 = 0;
    } // après qques minutes bascule sur 1er triac
  }
#endif
#endif
#endif

#ifdef MesureTemperature
#ifdef Sortie2
  if (routeur.utilisation2Sorties)
  {
    if ((temperatureEauChaude > routeur.temperatureBasculementSortie2) && (choixSortie == 0))
    {
      choixSortie = 1;
      sortieActive = 2;
    }
    // commande du gradateur2
    if ((temperatureEauChaude < routeur.temperatureRetourSortie1) && (choixSortie == 1))
    {
      choixSortie = 0;
      sortieActive = 1;
    }
    // commande du gradateur1
  }
  else
  {
    choixSortie = 0;
    sortieActive = 1;
  }
#endif

  if (routeur.relaisStatique && strcmp(routeur.tensionOuTemperature, "D") == 0)
  {
    if (temperatureEauChaude > routeur.seuilMarche)
    {
      digitalWrite(pinRelais, LOW); // mise à un du relais statique
      etatRelaisStatique = true;
    }
    if (temperatureEauChaude < routeur.seuilArret)
    {
      digitalWrite(pinRelais, HIGH); // mise à zéro du relais statique
      etatRelaisStatique = false;
    }
  }
#endif

  if (routeur.relaisStatique && strcmp(routeur.tensionOuTemperature, "V") == 0)
  {
    if (capteurTension > routeur.seuilMarche)
    {
      digitalWrite(pinRelais, LOW); // mise à un du relais statique
      etatRelaisStatique = true;
    }
    if (capteurTension < routeur.seuilArret)
    {
      digitalWrite(pinRelais, HIGH); // mise à zéro du relais statique
      etatRelaisStatique = false;
    }
  }
  if ((marcheForcee) && (tempo == 0))
  {
    tempdepart = millis(); //  memorisation de moment de depart
    tempo = 1;
  }
  if ((marcheForcee) && (tempo == 1))
  {
    puissanceGradateur = marcheForceePercentage * 9; //limitation 90%
    if (millis() > tempdepart + 60000)
    { // decremente toutes les minutes
      tempdepart = millis();
      temporisation--;
      paramchange = 1;
    }                       // durée de forcage en milliseconde
    if (temporisation == 0) // fin de temporisation
    {
      marcheForcee = false;
      tempo = 0;
    }
  }
}

RARegulationClass RARegulation;
