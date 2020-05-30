/***************************************/
/******** pilotage et regulation   ********/
/***************************************/
#include "regulation.h"
//#include "mesure.h"
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
  devlente = 0;
  if (x > xmax)
  {
    dev = 1;
    xmax = x;
    xmin = xmax - seuil;

  } // si dépasse le seuil haut déplacement du seuil haut et bas

  if (x < xmin)
  {
    dev = -1;
    xmin = x;
    xmax = xmin + seuil;
   // mesure récente = 0  et ensuite ranger vers la plus ancienne
     for (int i = 0; i < 9; i++)
          {
            tabxmin[9 - i] = tabxmin[8 - i];
          }
    tabxmin[0] = xmin; // range les valeurs mini  
    float deltaxmin = 0;
     for (int i = 1; i < 10; i++)
        {
          deltaxmin += tabxmin[i] - tabxmin[0];
        } // calcule la pente négative
        
    if (itab < 10)  itab++; // retire les 10 premiers points avant de calculer
    if ((itab >= 10) && (deltaxmin > 10 * seuil))  // avec réglage de la pente de descente par le seuil
        {
          devlente = 1; // le courant décroit sur les 10 derniers points
        }
     else
        {
          devlente = 0;  // le courant croit ou reste constant sur les 10 derniers points
        }
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
int chargecomp = 0;
#define variation_lente 1    // config 5
#define variation_normale 10 // config 10
#define variation_rapide 20  // config 20
#define bridePuissance 900   // sur 1000


int testpmax=0;

int RARegulationClass::regulGrad(int dev)
{
  calPuisav = calPuis;
    // décalage du courant pour maintenir la regulation proche de zéro
  /*
    if (intensiteBatterie < 0)   devcount = 0;
   if (devprevious == dev)
          {
            if (devcount < 100)  devcount++; // si deviation dans le même sens
          } else  devcount = 0;

 //  courant constant
  if (dev == 0)
  {
    calPuis += variation_lente;  // demarrage
    if (devcount > 3)  calPuis += variation_normale; // accélération
     if ((devcount > 7) && (calPuis < puisGradmax - 50)) // accélération jusque la puissance maxi
            calPuis += variation_rapide;
  }
 
 // courant croissant 
   if (dev > 0)
  {
    calPuis += variation_lente;
    if (devcount > 3)  calPuis += variation_normale;
    if ((devcount > 7) && (calPuis < puisGradmax)) calPuis += variation_rapide;
  }
  
 // courant décroissant
  if (dev < 0) 
  {
    // fin de charge 
    if (intensiteBatterie < 2)
        {
          calPuis -= variation_normale + variation_lente;
        }
    else
        {
          calPuis -= variation_rapide + variation_lente;
        }
     
    if (devcount > 2) calPuis -= variation_normale; // accélération de la descente
    if ((devcount > 5) && (intensiteBatterie > 2)) calPuis -= variation_rapide;
  }*/

   // descente réguliere lente 
    if ((devlente == 1) && (intensiteBatterie > 0))
    {
    if (intensiteBatterie > 2)
        {
          calPuis -= 2* variation_rapide; //descente brutale
        }
        else
        {
          calPuis -= variation_lente;  // fin de charge descente lente
        }
     } else calPuis += variation_lente; // autorise la montée si la pente n'est pas descendante

  // courant varie mais reste globalement constant permet le décollage
  if (devlente == 0) {
            if (intensiteBatterie >= 2)  
               { 
                  chargecomp=0;  // charge en cours
                  calPuis += variation_normale;
                }
            if ((intensiteBatterie < 2) && (intensiteBatterie >= 0) ) 
                { 
                  chargecomp=1;  // fin de charge
                  calPuis += variation_normale;
                }
          if ((intensiteBatterie < 0) && (intensiteBatterie >= -routeur.toleranceNegative) ) 
                { 
                  chargecomp=1;  // fin de charge
                  
                }
            }
 
   if ((intensiteBatterie < 0) && (intensiteBatterie >= -routeur.toleranceNegative) )  {
                   calPuis += variation_lente;
            }
 
  // mesure de la puissance maximum pour le remonter en puissance
  if (calPuis<(8*puisGradmax/10) )  {
                   calPuis += variation_normale;
            } puisGradmax = calPuis;
 
   
 // courant devient négatif
   if (intensiteBatterie < -routeur.toleranceNegative)
        {
         if (chargecomp==0) calPuis = calPuis/2; else calPuis -= variation_normale;
         if ((ineg < 1000) && (intensiteBatterie < 0))
            {
              ineg++;
              //puisGradmax = calPuis-2*variation_rapide;
              if (intensiteBatterie < -routeur.toleranceNegative) calPuis  -= variation_normale;
            } 
        if (ineg > 5) // autorisation de 5 mesures négatives avec baisse régulière
            {
              puisGradmax = 0;
              calPuis -=  2*variation_rapide;
            } // tolerance pic négatif
       }
  else
      {
        ineg = 0;
      }
 
  
  devprevious = dev;

  // seuil de démarrage 
  if (capteurTension > routeur.seuilDemarrageBatterie + 0.2)   tesTension = 1;
  // seuil bas de batterie
  if (capteurTension < routeur.seuilDemarrageBatterie - 0.5) tesTension = 0;
  if (tesTension == 0) calPuis = 0;
 
  calPuis = min(max(0, calPuis), bridePuissance);
  return (calPuis);
}

/**************************************/
/******** Pilotage exterieur*******/
/**************************************/
unsigned long tempdepart;
int tempo = 0;

int RARegulationClass::pilotage()
{
  // pilotage du 2eme triac
#ifndef MesureTemperature
#ifdef Sortie2
#ifdef Pzem04t
  if (routeur.utilisation2Sorties)
  {
    if ((puissanceDeChauffe == 0) && (puissanceGradateur > 100))
      tempo2 = ++;
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
