/********************************************/
/******** Mesure du courant  tension  ********/
/********************************************/
#include "mesure.h"
#include "settings.h"
#include "triac.h"
#include <Arduino.h>

#ifdef Pzem04t
#include "PZEM004Tv30.h"
PZEM004Tv30 pzem(&Serial2);
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
#ifdef MesureTemperature
#include <DallasTemperature.h>
#include <OneWire.h>
const int oneWireBus = pinTemp;
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);
#endif


void RAMesureClass::setup()
{
#ifdef MesureTemperature
  sensors.begin(); // demarrage de la sonde de temperature
#endif
}

void RAMesureClass::mesurePinceTension(int jmax, int imax)
{
 int inttension=0; // mesure de tension
 float floattension=0;
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
 //  floattension=inttension;
 //  if (inttension<1500) capteurTension=inttension*routeur.coeffTension;  // applique le coeff
 //    else capteurTension=(0.0383-1.85e-5*floattension+4.26e-9*floattension*floattension)*floattension;
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
  #define Pballon 1000 // 1000W puissance du ballon
  affpzem++;
  if (affpzem < 5)
    return;
  else
    affpzem = 0;
  puissanceDeChauffe = 0;
  float theta=PI*(1000-puissanceGradateur)/1000;
  puissanceDeChauffe = Pballon*(1-theta/PI+(sin(2*theta)/2)/PI);
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


float RAMesureClass::mesurePinceAC(int pinPinceAC, float coeff, bool avecsigne){
   #define ECH 200
   #define bclMesure 3 // faire 3 mesures pour garantir le signe surtout avec des faibles courants
   
  int signe=0;
  int valuesA[ECH];
  int valuesB[ECH];
  float ret=0;
  float I1valeff=0;  // sinusoide sur une composante continue
  float I1valmoy=0;  // composante continue
  int Tmax=20000; //us periode 50hz = 20ms = 20 000 us
    unsigned long start_times;
    unsigned long stop_times;
    int pause=0;
       int i;
        byte min=255;
        float puissanceAC=0;
        // calcul de la duree de mesure //
        start_times = micros();
        for(i=0;i<ECH;i+=1) { valuesA[i] = analogRead(pinPinceAC); valuesB[i] = 0; }  
        stop_times = micros();
        // calcule de l'interval entre les mesure
        pause=Tmax-(stop_times - start_times);
        pause=pause/ECH; 
        int nbfois=1;
        if (avecsigne) nbfois=bclMesure;
      for (int u=0;u<nbfois;u++)
      {
        
        for(i=0;i<ECH;i+=1) {
            valuesA[i] = analogRead(pinPinceAC); valuesB[i] = 0;
        delayMicroseconds(pause);
        } 
 
        float a=0;I1valeff=0;  I1valmoy=0;  

       for(int i=0;i<ECH;i++) I1valmoy+=valuesA[i]; I1valmoy=I1valmoy/ECH;
       for(int i=0;i<ECH;i++) {    a=valuesA[i];    a-=I1valmoy; a=a*a;    I1valeff+=a; }    

        
        I1valeff=coeff*(sqrt(I1valeff)/ECH); 

       int i_zc=0; 
       float p=0;  
       // mise en creneaux
        for(int i=0;i<ECH;i++) if (valuesB[i]>3000) valuesB[i]=4500; else valuesB[i]=0;
       // detection zc
        for(int i=1;i<ECH;i++) {  if ((valuesB[i-1]>4000) && valuesB[i]<1000) { i_zc=i; i=ECH; }  }
      // calcul de p
    for(int i=0;i<ECH;i++) p+=(7.5*(valuesA[i]-I1valmoy)*coeff*310*sin((i-i_zc)*2*PI/ECH))/ECH;
    
  /*   Serial.println(1000);
       for(int i=0;i<ECH;i++) {   
                              //Serial.print(5*(valuesA[i]-I1valmoy));
                              Serial.print(','); Serial.print(valuesB[i]/10);
                              Serial.print(','); Serial.println(230*sin((i-i_zc)*2*PI/ECH));
                              //Serial.print(','); Serial.print(5*(valuesA[i]-I1valmoy)*sin((i-i_zc)*2*PI/ECH));
                              //Serial.print(','); Serial.println(p);
                              }    

                      
     for(int i=0;i<ECH;i++) {Serial.print(1);Serial.print(','); Serial.println(1);}*/
  
      if (p>=0) signe+=1; else  signe-=1;    // teste 3 fois le signe
       ret+=abs(I1valeff);
       puissanceAC+=abs(p);
      }
     if (signe>=0) signe=1; else  signe=-1;    
     if (!avecsigne) signe=1;
     puissanceAC=signe*puissanceAC/bclMesure;
       return(signe*ret/bclMesure);
}





RAMesureClass RAMesure;
