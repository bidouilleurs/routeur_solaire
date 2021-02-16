//**************************************************************/
// L'équipe de bidouilleurs discord reseautonome vous présente
// la réalisation d'un gradateur pour chauffe-eau en off grid
// gradateur synchronisé sur la production photovoltaique
// sans tirage sur les batteries pour utiliser le surplus dans
// un ballon d'eau chaude
//**************************************************************/

const char *version_soft = "Version 1.5";

//**************************************************************/
// Initialisation
//**************************************************************/
#include "settings.h"
struct param routeur;
const int pinTriac = 27;         // GPIO27 triac
const int pinPince = 32;         // GPIO32   pince effet hall
const int zeroc = 33;            // GPIO33  passage par zéro de la sinusoide
const int pinPinceRef = 34;      // GPIO34   pince effet hall ref 2.5V
const int pinPotentiometre = 35; // GPIO35   potentiomètre
const int pinTension = 36;       // GPIO36   capteur tension
const int pinTemp = 23;          // GPIO23  capteurTempérature
const int pinSortie2 = 13;       // pin13 pour  2eme "gradateur
const int pinRelais = 19;        // Pin19 pour sortie relais
const int pinPinceAC = 39;       // GPIO39 pince AC
bool marcheForcee = false;
short int marcheForceePercentage = 25;
short int sortieActive = 1;
unsigned long temporisation = 60;
float intensiteBatterie = 0;
float capteurTension = 0;
int puissanceGradateur = 0;
float temperatureEauChaude = 0;
float puissanceDeChauffe = 0;
bool etatRelaisStatique = false;
bool modeparametrage = false;

int resetEsp = 0;
int testwifi = 0;
int choixSortie = 0;
int paramchange = 0;
bool SAP = false;
bool MQTT = false;
char traceurserie[50];
#ifdef Pzem04t
float Pzem_i = 0;

float Pzem_U = 0;
float Pzem_P = 0;
float Pzem_W = 0;
#endif
/**********************************************/
/********** déclaration des librairiess *******/
/**********************************************/
#include "triac.h"

#include "communication.h"

#ifdef OTA
#include "ota.h"
#endif
#ifdef EEprom
#include "prgEEprom.h"
#endif

#ifdef EcranOled
#include "afficheur.h"
#endif

//#define simulation // utiliser pour faire les essais sans les accessoires
#ifdef simulation
#include "simulation.h"
#endif

#include "mesure.h"
#include "regulation.h"
int iloop = 0; // pour le parametrage par niveau
extern int calPuis;
float mesureAc = 0;
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
  digitalWrite(pinRelais, LOW); // mise à zéro du relais statique
  pinMode(pinTemp, INPUT);
  pinMode(pinTension, INPUT);
  pinMode(pinPotentiometre, INPUT);
  pinMode(pinPince, INPUT);
  pinMode(pinPinceRef, INPUT);
  RACommunication.print(1, "", true);
  RACommunication.print(1, "definition ok ", true);
  RACommunication.print(1, version_soft, true);
  RACommunication.print(1, "", true);

#ifdef EcranOled
  RAAfficheur.setup();
#endif

#ifdef EEprom
  RAPrgEEprom.setup();
#endif

  delay(500);
  marcheForcee = false; // mode forcé retirer au démarrage
  marcheForceePercentage = 0;
  temporisation = 0;

#if defined WifiMqtt || defined WifiServer
  RACommunication.setup(version_soft);
#endif

#ifdef OTA
  RAOTA.begin();
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

#ifdef parametrage
  modeparametrage = true;
#endif

  delay(500);
  RATriac.watchdog(0);       // initialise le watchdog
  RATriac.start_interrupt(); // demarre l'interruption de zerocrossing et du timer pour la gestion du triac
}

void loop()
{
#ifdef OTA
  RAOTA.loop();
#endif

  RATriac.watchdog(1);                  //chien de garde à 4secondes dans timer0
  RAMesure.mesurePinceTension(700, 20); // mesure le courant et la tension avec 2 boucles de filtrage (700,20)
  if (routeur.utilisationPinceAC)
  {
    mesureAc = RAMesure.mesurePinceAC(pinPinceAC, routeur.coeffMesureAc, false);
  }

#ifdef simulation
  if (!modeparametrage)
    RASimulation.imageMesure(0); // permet de faire des essais sans matériel
  RACommunication.print(1, "Mode simulation", true);
#endif

  if (!modeparametrage)
  {
    RAMesure.mesureTemperature(); // mesure la temperature sur le ds18b20
    RAMesure.mesure_puissance();  // mesure la puissance sur le pzem004t

    int dev = RARegulation.mesureDerive(intensiteBatterie, 0.2); // donne 1 si ça dépasse l'encadrement haut et -1 si c'est en dessous de l'encadrement (Pince,0.2)

    if (mesureAc < routeur.seuilCoupureAC)
    {

      if (routeur.actif)
      {
        puissanceGradateur = RARegulation.regulGrad(dev); // calcule l'augmentation ou diminution du courant dans le ballon en fonction de la deviation
      }
      else
      {
        puissanceGradateur = 0;
      }
      RARegulation.pilotage(); // pilotage à distance
    }

    else
    {
      calPuis = 0;
      puissanceGradateur = 0;
    }
  }

  if (modeparametrage)
  {
    //    int potar = map(analogRead(pinPotentiometre), 0, 4095, 0, 1000); // controle provisoire avec pot
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
    /*   if (potar>10) puissanceGradateur=potar;
         else  if (potar<2) puissanceGradateur=0;
                      else puissanceGradateur=1; // priorité au potensiometre                    // priorité au potensiometre
      */
    char paramLog[50];
    sprintf(paramLog, "Courant %2.2f, Tension %2.2f, Gradateur %d", intensiteBatterie, capteurTension, puissanceGradateur);
    RACommunication.print(1, paramLog, true);

    delay(200);
  }

  // affichage traceur serie
  float c = puissanceGradateur;

  sprintf(traceurserie, "%2.2f,%d,%2.2f,%2.2f,%2.2f,%2.2f,%2.2f",
          c / 100,
          devlente * 2,
          intensiteBatterie,
          routeur.seuilDemarrageBatterie / 5,
          -routeur.toleranceNegative,
          mesureAc,
          capteurTension / 5);
  RACommunication.print(1, traceurserie, true);
  //  Serial.print("zero");  Serial.println(routeur.zeropince);

#ifdef EcranOled
  RAAfficheur.affichage_oled(); // affichage de lcd
#endif

#if defined WifiMqtt || defined WifiServer
  RACommunication.loop();
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

    RACommunication.print(1, "Restart !", true);
    ESP.restart(); // redemarrage"
  }
}
