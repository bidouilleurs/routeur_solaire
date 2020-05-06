//**************************************************************/
// L'équipe de bidouilleurs discord reseautonome vous présente
// la réalisation d'un gradateur pour chauffe-eau en off grid
// gradateur synchronisé sur la production photovoltaique
// sans tirage sur les batteries pour utiliser le surplus dans
// un ballon d'eau chaude
//**************************************************************/

#include "settings.h"

// Mettre à true pour un lancement en mode serveur
bool SAP = false;

//**************************************************************/
// Initialisation du système
//**************************************************************/
const char *version_soft = "Version 1.1";

const int pinTriac = 27;         // GPIO27 triac
const int pinPince = 32;         // GPIO32   pince effet hall
const int zeroc = 33;            // GPIO33  passage par zéro de la sinusoide
const int pinPinceRef = 34;      // GPIO34   pince effet hall ref 2.5V
const int pinPotentiometre = 35; // GPIO35   potentiomètre
const int pinTension = 36;       // GPIO36   capteur tension
const int pinTemp = 23;          // GPIO23  capteurTempérature
const int pinSortie2 = 13;       // pin13 pour  2eme "gradateur
const int pinRelais = 19;        // Pin19 pour sortie relais
short int marcheForceePercentage = 25;
short int sortieActive = 1;
unsigned long temporisation = 60;
float intensiteBatterie = 0;
float capteurTension = 0;
int puissanceGradateur = 0;
float temperatureEauChaude = 0;
float puissanceDeChauffe = 0;
bool etatRelaisStatique = false;
struct param routeur;
int resetEsp = 0;
int testwifi = 0;
int choixSortie = 0;
int paramchange = 0;
bool MQTT = false;
bool serverOn = false;
bool modeparametrage = false;
bool marcheForcee = false;

#ifdef Pzem04t
float Pzem_i = 0;
float Pzem_U = 0;
float Pzem_P = 0;
float Pzem_W = 0;
#endif
/**********************************************/
/********** déclaration des librairiess *******/
/**********************************************/
#ifdef Bluetooth
#include "modeBT.h"
#endif
#ifdef WifiServer
#include "modeserveur.h"
#endif
#include "triac.h"
#ifdef EEprom
#include "prgEEprom.h"
#endif
#ifdef EcranOled
#include "afficheur.h"
#endif
#ifdef WifiMqtt
#include "modemqtt.h"
#endif
#ifdef Bluetooth
#include "modeBT.h"
#endif
#include "mesure.h"
#include "regulation.h"
#ifdef simulation
#include "simulation.h"
#endif
/***************************************/
/******** Programme principal   ********/
/***************************************/
void setup()
{

  Serial.begin(115200);

  pinMode(pinTriac, OUTPUT);
  digitalWrite(pinTriac, LOW); // mise à zéro du triac au démarrage
  pinMode(pinSortie2, OUTPUT);
  digitalWrite(pinSortie2, LOW); // mise à zéro du triac de la sortie 2
  pinMode(pinRelais, OUTPUT);
  digitalWrite(pinRelais, HIGH); // mise à zéro du relais statique
  pinMode(pinTemp, INPUT);
  pinMode(pinTension, INPUT);
  pinMode(pinPotentiometre, INPUT);
  pinMode(pinPince, INPUT);
  pinMode(pinPinceRef, INPUT);
  Serial.println();
  Serial.println(F("definition ok "));
  Serial.println(version_soft);
  Serial.println();

#ifdef EEprom
  RAPrgEEprom.setup();
#endif

  delay(500);
  marcheForcee = false; // mode forcé retirer au démarrage
  marcheForceePercentage = false;
  temporisation = 0;
#ifdef WifiMqtt
  RAMQTT.setup();
#endif

#ifdef WifiServer
  RAServer.setup(); // activation de la page Web de l'esp
#endif

#ifdef Bluetooth // bluetooth non autorise avec serveur web ou MQTT
  RABluetooth.setup();
#endif

#ifdef EcranOled
  RAAfficheur.setup();
#endif

#ifdef MesureTemperature
  RAMesure.setup();
#endif

#ifdef Pzem04t
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2); // redefinition des broches du serial2
#endif

#ifdef Parametrage
  modeparametrage = true;
#endif

  delay(500);
  RATriac.watchdog(0);       // initialise le watchdog
  RATriac.start_interrupt(); // demarre l'interruption de zerocrossing et du timer pour la gestion du triac
}

int iloop = 0; // pour le parametrage par niveau

void loop()
{
  RATriac.watchdog(1);                  //chien de garde à 4secondes dans timer0
  RAMesure.mesurePinceTension(700, 20); // mesure le courant et la tension avec 2 boucles de filtrage (700,20)

#ifdef simulation
  if (!modeparametrage)
  {
    RASimulation.imageMesure(0); // permet de faire des essais sans matériel
  }
#endif

  if (!modeparametrage)
  {
    RAMesure.mesureTemperature(); // mesure la temperature sur le ds18b20
    RAMesure.mesure_puissance();  // mesure la puissance sur le pzem004t

    int dev = RARegulation.mesureDerive(intensiteBatterie, 0.2); // donne 1 si ça dépasse l'encadrement haut et -1 si c'est en dessous de l'encadrement (Pince,0.2)
    if (marcheForcee)
    {
      RARegulation.pilotage();
    }
    else
    {                                                   // pilotage à distance
      puissanceGradateur = RARegulation.regulGrad(dev); // calcule l'augmentation ou diminution du courant dans le ballon en fonction de la deviation
    }
  }

  if (modeparametrage)
  {
    //int potar = map(analogRead(pinPotentiometre), 0, 4095, 0, 1000); // controle provisoire avec pot
    iloop++;
    if (iloop < 10)
      puissanceGradateur = 1;
    else if (iloop < 20)
      puissanceGradateur = 100;
    else if (iloop < 30)
      puissanceGradateur = 200;
    else if (iloop < 40)
      puissanceGradateur = 400;
    else if (iloop < 50)
      puissanceGradateur = 600;
    else if (iloop < 60)
      puissanceGradateur = 1000;
    else
      iloop = 0;
    /*   if (potar>10) PuisGrad=potar;
       else  if (potar<2) PuisGrad=0;
                    else PuisGrad=1; // priorité au potensiometre                    // priorité au potensiometre
    */
    Serial.println();
    Serial.print("Courant ");
    Serial.println(intensiteBatterie);
    Serial.print("tension ");
    Serial.println(capteurTension);
    Serial.print("Gradateur ");
    Serial.println(puissanceGradateur);
    Serial.println();
    delay(1000);
  }

  // affichage traceur serie
  float c = puissanceGradateur;
  Serial.print(" ");
  Serial.print(c / 100);
  Serial.print(',');
  Serial.print(devlente * 2);
  Serial.print(',');
  Serial.print(intensiteBatterie);
  Serial.print(',');
  Serial.print(56 / 5);
  Serial.print(',');
  Serial.println(capteurTension / 5);

#ifdef EcranOled
  RAAfficheur.affichage_oled(); // affichage de lcd
#endif

#ifdef WifiMqtt
  RAMQTT.loop();
#endif

#ifdef WifiServer
  RAServer.loop();
#endif

#ifdef Bluetooth
  read_bluetooth();
#endif

#ifdef RSTEEprom // mettre 1 pour reinitialiser l'eeprom
  resetEsp = 1;
#endif

#ifdef EEprom
  if (resetEsp == 1)
  {
    RATriac.stop_interrupt();
    RAPrgEEprom.close_param();
    delay(5000);
  }
#endif

  if (resetEsp == 1)
  {
    Serial.println("Restart !");
    ESP.restart(); // redemarrage
  }
}
