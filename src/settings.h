#ifndef RA_SETTINGS_H
#define RA_SETTINGS_H
/**********************************************/
/********** Liste des options possibles ******/
/********************************************/
//#define parametrage // utiliser pour faire les essais sans les accessoires

//**** options MINI ****
#define EcranOled         // si pas d'écran oled      installer librairie heltec dev board
#define MesureTemperature // capteur DS18B20       installer librairie dallas temperature
#define WifiServer        // affiche les mesures dans une page html crée un point d'accès si pas de reseau
#define WifiMqtt          // mettre en commentaire si pas de réseau       installer librairie ArduinoJson et PubSubClient
#define OTA               // permet la mise à jour par OTA (over the air)
// #define Ecran_inverse

//**** options MAXI  ****
#define Pzem04t // utilise un pzem004 pour la mesure de puissance dans le ballon  inclure  https://github.com/mandulaj/PZEM-004T-v30
#define Sortie2 // utilise un 2eme triac
#define F50HZ   // Frequence du reseau 50Hz ou 60Hz non testé

const int VERBOSE = 1; // 0 pour rien , 1 pour info traceur serie, 2 pour error, 3 pour debug
struct param
{
  float zeropince = -20.84;                 // valeur mesurer à zéro (2) 2819 2818.5
  float coeffPince = 0.02475;               // Calculer le coefficient (4)0.14 0.210
  float coeffTension = 0.02008;             // diviseur de tension
  float seuilDemarrageBatterie = 57.20;     // seuil de mise en marche de la regulation dans le ballon
  float toleranceNegative = 0.8;            // autorisation de 300mA négative au moment de la charge complète
  bool utilisation2Sorties = false;         // validation de la sortie 2eme gradateur
  float temperatureBasculementSortie2 = 60; // température pour démarrer la regul sur la sortie 2
  float temperatureRetourSortie1 = 45;      // température pour rebasculer sur le premier gradateur
  bool relaisStatique = false;              // Indique si un relais statique est utilisé
  float seuilMarche = 50;                   // température ou tension de déclenchement du relais
  float seuilArret = 45;                    // température ou tension de déclenchement du relais
  char tensionOuTemperature[2] = "V";       // Indique si le seuil est en Volts ou en Degrés
  char ssid[30] = "Bbox-xxxxxxx";           // ssid de la box internet
  char password[50] = "6557D4EFxxxxxxxx";     // mot de passe
                                            // en mode serveur l'ip est 192.168.4.1'
                                            // ssid , "routeur_esp32"
                                            // password , "adminesp32"
  char mqttServer[30] = "192.168.1.18";
  short int mqttPort = 1883;
  char mqttUser[30] = "mosquitto";
  char mqttPassword[50] = "!*mosquitto*!";
  char mqttopic[30] = "sensor/solar";
  char mqttopicInput[30] = "output/solar";
  char mqttopicParam1[30] = "param/solar1";
  char mqttopicParam2[30] = "param/solar2";
  char mqttopicParam3[30] = "param/solar3";
  char mqttopicPzem1[30] = "sensor/Pzem1";
  float correctionTemperature = -2.3;
  char basculementMode[2] = "T"; // Choix du mode de basculement : T->température, P-> Puissance zero
  bool actif = true;
  float seuilCoupureAC = 0.3;      // Seuil de coupure pour la pince AC
  float coeffMesureAc = 0.321;     // Coeff de mesure de la pince AC
  bool utilisationPinceAC = false; // Utilisation d'une Pince pour la mesure AC
  bool utilisationSAP = false;     // Utilisation du SAP uniquement
};
extern struct param routeur; //  regroupe tous les parametres de configuration
extern float mesureAc;
extern bool marcheForcee; // validation de la sortie marche forcée
extern short int marcheForceePercentage;
extern short int sortieActive;      // affichage triac actif par défaut le 1er
extern unsigned long temporisation; // temporisation de la marche forcée par défaut 1h
extern float intensiteBatterie;     // mesure de la pince à effet hall
extern float capteurTension;        // mesure de la tension batterie en volts
extern int puissanceGradateur;      // armorcage du triac
extern float temperatureEauChaude;  // mesure de la tension batterie en volts
extern float puissanceDeChauffe;    // mesure avec le pzem
extern bool etatRelaisStatique;
extern int resetEsp;    // reset l'esp
extern int testwifi;    // perte de signal wifi ou mqtt
extern int choixSortie; // choix du triac1 ou 2
extern int paramchange; // si =1 alors l'envoie des parametres en mqtt
extern bool SAP;        // mode serveur initialisé en point d'acces
extern bool MQTT;       // verification du broker mqtt
extern bool modeparametrage;
extern int devlente;
/***************************************/
/********** déclaration des Pins *******/
/***************************************/
extern const int pinTriac;
extern const int pinPince;
extern const int zeroc;
extern const int pinPinceRef;
extern const int pinPotentiometre;
extern const int pinTension;
extern const int pinTemp;
extern const int pinSortie2;
extern const int pinRelais;
extern const int pinPinceAC;
extern bool wifiSAP;

#define RXD2 18 // Pin pour le Pzem v3.0 //18
#define TXD2 17 //17"

#ifdef WifiMqtt
#define WifiServer
#endif
#ifdef WifiServer
#define EEprom // sauvegarde des configurations dans eeprom passage de parametre par Wifi Mqtt
#endif
#ifdef Pzem04t
extern float Pzem_i;
extern float Pzem_U;
extern float Pzem_P;
extern float Pzem_W;
#endif

#endif
