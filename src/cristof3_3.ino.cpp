//  **************************************************************/
// L'équipe de bidouilleurs discord reseautonome vous présente
// la réalisation d'un gradateur pour chauffe-eau en off grid
// gradateur synchronisé sur la production photovoltaique
// sans tirage sur les batteries pour utiliser le surplus dans
// un ballon d'eau chaude
//  **************************************************************/
#include <Arduino.h>
//#define Parametrage  // utiliser pour faire les étalonnages

/**********************************************/
/********** Liste des options possibles ******/
/********************************************/

#ifndef Parametrage             // utiliser pour faire les étalonnages
#define EcranOled             // si pas d'écran oled      installer librairie heltec dev board
#define MesureTemperature     // capteur DS18B20       installer librairie dallas temperature
#define Pzem04t               // utilise un pzem004 pour la mesure de puissance dans le ballon  inclure  https://github.com/mandulaj/PZEM-004T-v30
#define WifiMqtt              // mettre en commentaire si pas de réseau       installer librairie ArduinoJson et PubSubClient
#define EEprom                // sauvegarde des configurations dans eeprom passage de parametre par Wifi Mqtt  
//#define Sortie2               // utilise un 2eme triac
//#define WifiServer            // affiche les mesures dans une page html ne pas mettre en même temps que wifimqtt
#endif
#define Frequence 50HZ          // Frequence du reseau 50Hz ou 60Hz non testé

/***************************************/
/********** déclaration des Pins *******/
/***************************************/
const int pinTriac         =        27;    // GPIO27 triac
const int pinPince         =        32;    // GPIO32   pince effet hall
const int zeroc            =        33;    // GPIO33  passage par zéro de la sinusoide
const int pinPinceRef      =        34;    // GPIO34   pince effet hall ref 2.5V
const int pinPotentiometre =        35;    // GPIO35   potentiomètre
const int pinTension       =        36;    // GPIO36   capteur tension
const int pinTemp          =        23;    // GPIO23  capteurTempérature
const int pinSortie2       =        13;    // pin13 pour  2eme gradateur
const int pinRelais        =        19;    // Pin19 pour sortie relais
#define     RXD2                    18     // Pin pour le Pzem v3.0 //18
#define     TXD2                    17                              //17


struct param
{
  float zeropince         = -100;           // valeur mesurer à zéro (2) 2819 2818.5
  float coeffPince        = 0.0294 ;         // Calculer le coefficient (4)0.14 0.210
  float coeffTension      = 0.0177533;       // diviseur de tension
  float seuilTension      = 56;              // seuil de mise en marche de la regulation dans le ballon
  float toleranceNeg      = 0.5;             // autorisation de 300mA négative au moment de la charge complète
 boolean sortie2          = false;           // validation de la sortie 2eme gradateur
  int sortie2_tempHaut    = 60;              // température pour démarrer la regul sur la sortie 2
  int sortie2_tempBas     = 45;              // température pour rebasculer sur le premier gradateur
 boolean sortieRelaisTemp = false;           // validation de la sortie relais température
  int relaisMax           = 50;              // température de déclenchement du relais
 boolean sortieRelaisTens = false;           // validation de la sortie relais tension
  int relaisMin           = 45;              // tension de déclenchement du relais
 boolean maForce          = false;           // validation de la sortie marche forcée
  int maForceval          = 25;              // valeur par defaut 25%
  byte sortieActive       = 1;               // affichage triac actif par défaut le 1er
unsigned long temporisation       = 60;              // temporisation de la marche forcée par défaut 1h
};

struct param routeur;     //  regroupe tous les parametres de configuration
float Pince=0;            // mesure de la pince à effet hall
float capteurTension=0;   // mesure de la tension batterie en volts
int   PuisGrad=0;         // armorcage du triac
float capteurTemp=0;      // mesure de la tension batterie en volts
float puissanceMono=0;    // mesure avec le pzem
 
int   bloc_i=0;           // bloc les interruptions zero crossing
int   resetEsp=0;         // reset l'esp
int   testwifi=0;         // perte de signal wifi ou mqtt
int   choixSortie=0;      // choix du triac1 ou 2
int   paramchange=0;      // si =1 alors l'envoie des parametres en mqtt


/**********************************************/
/********** déclaration des librairiess *******/
/**********************************************/
#ifdef WifiMqtt
#include "ArduinoJson.h"
#include "WiFi.h"
#include "PubSubClient.h"

#ifdef EEprom          // sauvegarde des configuration dans eeprom passage de parametre par Wifi Mqtt
#include "EEPROM.h"
#define EEPROM_SIZE 200
#endif

#endif // fin de wifimqtt


#ifdef EcranOled
#include "heltec.h"
#endif


#ifdef MesureTemperature
#include <OneWire.h>
#include <DallasTemperature.h>

// GPIO where the DS18B20 is connected to
const int oneWireBus = pinTemp;     
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);
#endif


#ifdef Pzem04t
#include <PZEM004Tv30.h>


PZEM004Tv30 pzem(&Serial2);
float Pzem_i=0;
float Pzem_U=0;
float Pzem_P=0;
float Pzem_W=0;

#endif


/************************* WiFi Access Point *********************************/
#ifdef WifiMqtt
#include "codewifi.h"

/************ WiFi & MQTT objects  ******************/
void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();
 if (String(topic) == mqqtopic_input) {
    if(messageTemp == "rst")
                          { 
                             Serial.println("reset");
                             resetEsp=1;
                           }
    
 /*   if(messageTemp == "off")
                          {
                              Serial.println("off");
                           }  */
    

    if (messageTemp.substring(0, 3).equals("bat")) // reception ex: "bat51" 51 est converti en seuil batterie
                          {
                              Serial.print("bat ");
                              int com=messageTemp.substring( 3).toInt();
                               routeur.seuilTension=com;
                            paramchange=1;
                           }  
 
    if (messageTemp.substring(0, 3).equals("neg")) // reception ex: "neg0.6" autorise un seuil de démarrage
                          {
                              Serial.print("neg ");
                              float com=messageTemp.substring( 3).toFloat();
                               routeur.toleranceNeg=com;
                            paramchange=1;
                           }  
 
   if (messageTemp.substring(0, 3).equals("sth")) // reception ex: "sth51" 51 seuil de temperature haute
                          {
                              Serial.print("sth ");
                              int com=messageTemp.substring( 3).toInt();
                               routeur.sortie2_tempHaut=com;
                            paramchange=1;
                           }  
 
   if (messageTemp.substring(0, 3).equals("stb")) // reception ex: "stb51" 51 seuil de temperature basse
                          {
                              Serial.print("stb ");
                              int com=messageTemp.substring( 3).toInt();
                               routeur.sortie2_tempBas=com;
                            paramchange=1;
                           }  

  if (messageTemp.substring(0, 3).equals("rth")) // reception ex: "rth51" 51 relais tension haut
                          {
                              Serial.print("rth ");
                              int com=messageTemp.substring( 3).toInt();
                               routeur.relaisMax=com;
                            paramchange=1;
                           }  

  if (messageTemp.substring(0, 3).equals("rtb")) // reception ex: "rtb51" 51 relais tension bas
                          {
                              Serial.print("rtb ");
                              int com=messageTemp.substring( 3).toInt();
                               routeur.relaisMin=com;
                            paramchange=1;
                           }  
 
#ifdef Sortie2                
  if (messageTemp.substring(0, 3).equals("sor")) // reception ex: "sor1"  ou "sor0"  pour la commande du 2eme gradateur 
                          {
                              Serial.print("sortie2 ");
                              int com=messageTemp.substring( 3).toInt();
                               if (com==1) routeur.sortie2=true; else routeur.sortie2=false;
                           paramchange=1;
                           }  
#endif                         
 
     if (messageTemp.substring(0, 3).equals("cmf")) // reception ex: "cmf1"  ou "cmf0"  pour la commande de la marche forcee 
                          {
                              Serial.print("marche forcée ");
                              int com=messageTemp.substring( 3).toInt();
                               if (com==1) routeur.maForce=true; else routeur.maForce=false;
                           paramchange=1;
                           }  
 
     if (messageTemp.substring(0, 3).equals("rmf")) // reception ex: "rmf25" 25% de charge
                          {
                              Serial.print("valeur marche forcée  ");
                              int com=messageTemp.substring( 3).toInt();
                              Serial.println(com);
                               routeur.maForceval=com;
                            paramchange=1;
                          }
                            
     if (messageTemp.substring(0, 3).equals("rel")) // reception ex: "rel0" le relais température et tension à zéro rel1 pour temp rel2 tension
                          {
                              Serial.print("sortie relais  ");
                              int com=messageTemp.substring( 3).toInt();
                              Serial.println(com);
                               if (com==0)  { routeur.sortieRelaisTemp=false; routeur.sortieRelaisTens=false; }
                               if (com==1)  {routeur.sortieRelaisTemp=true; routeur.sortieRelaisTens=false; }
                               if (com==2)  {routeur.sortieRelaisTemp=false; routeur.sortieRelaisTens=true; }
                            paramchange=1;
                           }  

     if (messageTemp.substring(0, 3).equals("tmp")) // reception ex: "tmp60" 60 minute de marche forcée
                          {
                              Serial.print("temporisation  ");
                              int com=messageTemp.substring( 3).toInt();
                              Serial.println(com);
                               routeur.temporisation=com;
                            paramchange=1;
                          }
 
  }
}


WiFiClient espClient;
PubSubClient client(espClient);

void Mqtt_subcrive(){
  if (client.connected()) {
     client.subscribe(mqqtopic_input);
  }
  client.loop();
}


int affpub=0;

void Mqtt_publish(int a){
    if (a>0) affpub=30; // forcage de l'envoie
    affpub++;
    if (affpub<20)return; else affpub=0;
    
    const int capacity = JSON_OBJECT_SIZE(5);   // 4 données dans json
   StaticJsonDocument<capacity> doc;
   char buffer[400];
  //Exportation des données en trame JSON via MQTT
  doc["Intensite"] = Pince;
  doc["Tension"] = capteurTension;
  doc["Gradateur"] = PuisGrad;
  doc["Temperature"] = capteurTemp;
  doc["puissanceMono"] = puissanceMono;

  size_t n = serializeJson(doc, buffer);   // calcul de la taille du json
/*
 String output;
   serializeJson(doc, output);
  Serial.println(output);
 */
   buffer[n-1]='}';
   buffer[n]=0;                   // fermeture de la chaine json
   Serial.println(buffer);
 if (client.connected()) {
  
  if (client.publish(mqqtopic, buffer, n) == true) {  
    Serial.println("Success sending message");
  } else {
    Serial.println("Error sending message");
  }


 }

  client.loop();
    if (a==0) return;
  
    const int capacityParam = JSON_OBJECT_SIZE(5);   // 4 données dans json
   StaticJsonDocument<capacityParam> doc2;
   StaticJsonDocument<capacityParam> doc3;
   StaticJsonDocument<capacityParam> doc4;
//   char buffer[150];
  //Exportation des données en trame JSON via MQTT
  doc2["zeropince"]      = routeur.zeropince;
  doc2["coeffPince"]     = routeur.coeffPince;
  doc2["coeffTension"]   = routeur.coeffTension;
  doc2["seuilTension"]   = routeur.seuilTension;
  doc2["toleranceNeg"]   = routeur.toleranceNeg;
  doc3["sortie2"]        = routeur.sortie2;
  doc3["sortie2_tempHaut"] = routeur.sortie2_tempHaut;
  doc3["sortie2_tempBas"]  = routeur.sortie2_tempBas;
  doc3["temporisation"]  = routeur.temporisation;
  doc3["sortieActive"]  = routeur.sortieActive;
  doc4["sortieRelaisTemp"] = routeur.sortieRelaisTemp;
  doc4["relaisMax"]       = routeur.relaisMax;
  doc4["sortieRelaisTens"] = routeur.sortieRelaisTens;
  doc4["relaisMin"]       = routeur.relaisMin;
  doc4["Forcage_1h"]      = routeur.maForce;

 
  n = serializeJson(doc2, buffer);   // calcul de la taille du json
  buffer[n-1]='}';
   buffer[n]=0;                   // fermeture de la chaine json
   Serial.println(buffer);
 if (client.connected()) {
  
  if (client.publish(mqqtopic_param1, buffer, n) == true) {  
    Serial.println("Success sending message param");
  } else {
    Serial.println("Error sending message param");
  }
 }


  n = serializeJson(doc3, buffer);   // calcul de la taille du json
  buffer[n-1]='}';
   buffer[n]=0;                   // fermeture de la chaine json
   Serial.println(buffer);
  if (client.connected()) {
  

 if (client.publish(mqqtopic_param2, buffer, n) == true) {  
    Serial.println("Success sending message param");
  } else {
    Serial.println("Error sending message param");
  }
 }
   n = serializeJson(doc4, buffer);   // calcul de la taille du json
  buffer[n-1]='}';
   buffer[n]=0;                   // fermeture de la chaine json
   Serial.println(buffer);
 if (client.connected()) {
  
  if (client.publish(mqqtopic_param3, buffer, n) == true) {  
    Serial.println("Success sending message param");
  } else {
    Serial.println("Error sending message param");
  }
 }

  client.loop();

 //  sortie du pzem
#ifdef Pzem04t

 //  capacity = JSON_OBJECT_SIZE(4);   // 4 données dans json
   StaticJsonDocument<capacity> doc5;
 //  char buffer[400];
  //Exportation des données en trame JSON via MQTT
  doc5["Intensite"] = Pzem_i;
  doc5["Tension"] = Pzem_U;
  doc5["Puissance"] = Pzem_P;
  doc5["Energie"] = Pzem_W;
  doc5["Cosf"] = 0;

   n = serializeJson(doc5, buffer);   // calcul de la taille du json
   buffer[n-1]='}';
   buffer[n]=0;                   // fermeture de la chaine json
   Serial.println(buffer);
 if (client.connected()) {
  
  if (client.publish(mqqtopic_Pzem1, buffer, n) == true) {  
    Serial.println("Success sending message");
  } else {
    Serial.println("Error sending message");
  }


 }


/********************* fin d'envoie ***********************/ 
  client.loop();
#endif

}

#endif  // fin de la definition des fonctions mqtt


#ifdef WifiServer

#ifndef WifiMqtt
#include <WiFi.h>
#include "codewifi.h"


WiFiServer server(80);

void serveurHTML(){
 WiFiClient client = server.available();   // listen for incoming clients

  if (client) {                             // if you get a client,
    Serial.println("New Client.");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // the content of the HTTP response follows the header:
            client.print("Le courant dans la batterie est de "); client.print(String(Pince));client.print(" A<br><br>");
            client.print("La tension de la batterie est de "); client.print(String(capteurTension));client.print(" V<br><br>");
            client.print("Le gradateur est a "); client.print(String(PuisGrad/10));client.print(" % <br><br>");
#ifdef MesureTemperature     
            client.print("La temperature est de  "); client.print(String(capteurTemp));client.print(" °C<br><br>");
#endif
#ifdef Pzem04t       
            client.print("La puissance actuelle en sortie est de "); client.print(String(PuissanceMono));client.print(" W");
#endif

            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

       }
    }
    // close the connection:
    client.stop();
    Serial.println("Client Disconnected.");
  }
}




#endif  // fin non mqtt
#endif  // fin de WifiServer




/***************************************/
/* gestion des sauvegarde sur eeprom   */
/***************************************/
#ifdef EEprom          // sauvegarde des configuration dans eeprom passage de parametre par Wifi Mqtt

void sauve_param(){
  int ta=sizeof(struct param);
  EEPROM.writeByte(0,123);  // valeur pour la premiere sauvegarde
  EEPROM.put(1,routeur);
 //    for (int i = 0; i < ta; i++) Serial.print(byte(EEPROM.read(i))); Serial.print(" ");
     resetEsp=1;
 
}

void restore_param(){
  int ta=sizeof(struct param);
  EEPROM.get(1,routeur);
 //    for (int i = 0; i < ta; i++) Serial.print(byte(EEPROM.read(i))); Serial.print(" ");
}

#endif   // fin des declarations des fonctions liées à l'EEprom






/***************************************/
/* Bibliothèques de commande du triac  */
/***************************************/
//interruption timer
static hw_timer_t * timer = NULL;  // variable timer interne
int brightness;    // angle d'amorçage des triacs
bool zc_detected = false;  // variable de détection des passages par zéro
int zc_count = 0;   // comptage de l'horloge
int zc_countAFF = 0;   // comptage de l'horloge
int comTriac=0;

void IRAM_ATTR onTimer() {
     if (zc_detected == true) {
        zc_count++;
         if ( zc_count>brightness) {   // traduit l'angle d'amorcage en impulsion
           if (choixSortie==0)  digitalWrite(pinTriac, HIGH); else digitalWrite(pinSortie2, HIGH);
           comTriac++;
           if (comTriac>5){
                      zc_detected = false; // fin de détection
                      zc_count=0;
                      comTriac=0;
                         }
           }
     }   
    if (comTriac==0) { digitalWrite(pinTriac, LOW); digitalWrite(pinSortie2, LOW); }
    timerWrite(timer, 0); //reset timer 
}

void timer_config(){
  timer = timerBegin(0, 80, true); // horloge 
  timerAttachInterrupt(timer, &onTimer, true); // renvoi vers 
  timerAlarmWrite(timer, 10, true); // 10us*1000 =10 ms, autoreload true   10
  timerAlarmEnable(timer); // enable

}

// interruption passage par zéro
void ZeroCross(){
  
#ifdef Frequence == 60HZ
  const int MaxCommande=620; // a verifier pour le 60Hz nombre d'impulsion max pour faire la periode
#elif Frequence == 50HZ
  const int MaxCommande=850;  // 850
#endif
  if (bloc_i==1) return;
  brightness = map(PuisGrad, 0, 1000, MaxCommande , 10); //750 70pour chistof
  if ((zc_count==0) && (PuisGrad>0)) {zc_detected = true;     timerWrite(timer, 0);} //reset timer 
 // doit terminer le cycle 
}

void zeroCross_config(){
   pinMode(zeroc, INPUT);
   attachInterrupt(zeroc, ZeroCross, RISING);
//   attachInterrupt(digitalPinToInterrupt(zeroc), ZeroCross, RISING);
}



/***************************************/
/******** gestion afficheur   ********/
/***************************************/
#ifdef EcranOled
void cls(){

  for (int16_t i=0; i<DISPLAY_HEIGHT/2; i+=2) {
    Heltec.display->drawRect(i, i, DISPLAY_WIDTH-2*i, DISPLAY_HEIGHT-2*i);
    Heltec.display->display();
//    delay(10);
  }

   Heltec.display->clear();
   Heltec.display->display();

}


int teAff=0;

void affichage_oled(){
    

 if (teAff>=5) {
  Heltec.display->clear();
  Heltec.display->drawProgressBar(0, 53, 120, 10, abs(PuisGrad)/10);
  // draw the percentage as String
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawString(4, 0, "I_bat = " + String(Pince) + " A  ");  
  Heltec.display->drawString(4, 13, "U_bat = " + String(capteurTension) + " V  ");  
  Heltec.display->drawString(4, 27, "T°   = " + String(capteurTemp) + " °C  ");  
  Heltec.display->drawString(4, 39, "Puis = " + String(puissanceMono) + " W  ");  
   // write the buffer to the display
  Heltec.display->display();
  teAff=0;
  }
 teAff++;

}

 #endif  // fin de déclaration des fonctions liées à l'écran oled

/********************************************/
/******** Mesure du courant  tension  ********/
/********************************************/
float Pinceav=0;
int intPince=0;
float pince2=0;
float pince1=0;
float tensionav=0;
int cptimp=0;

void mesurePinceTension(int jmax,int imax){
 int inttension=analogRead(pinTension); // mesure de tension
  pince2=0;
  for(int j=0;j<jmax;j++) {  // boucle exterieure

  for(int i=0;i<imax;i++){    // boucle interieur
  intPince=analogRead(pinPince)-analogRead(pinPinceRef);
   Pince = intPince-routeur.zeropince;  // decalage du zero
   Pince = (Pince)*routeur.coeffPince;  // applique le coeff
   Pince = (Pince+(imax-1)*pince1)/imax; // intégration de la mesure
   pince1=Pince;
   }
   
   Pince=((jmax-1)*pince2+Pince)/jmax;
   pince2=Pince;

   inttension=analogRead(pinTension);       // mesure de tension
   capteurTension=inttension*routeur.coeffTension;  // applique le coeff
      tensionav=(capteurTension+(imax-1)*tensionav)/imax;
  }
  
  capteurTension=tensionav;
    inttension=capteurTension*100;
     capteurTension=inttension;
     capteurTension=capteurTension/100; // arrondi à la 1er virgule
 
 /* if (Pince<0){   // suppression du 1er point négatif
                if(cptimp==0) { Pince=0; cptimp=1;  } 
       }else cptimp=0;*/
 // Pince=pince2/jmax;

}

int afftemp=500;

void mesureTemperature(){
    afftemp++;
    if (afftemp<500) return; else afftemp=0;
  
#ifdef MesureTemperature
   
 //int inttemp=analogRead(pinTemp); // mesure de tension
 //    capteurTemp=inttemp;
 PuisGrad=0;
   sensors.requestTemperatures(); 
 float temperatureC = sensors.getTempCByIndex(0);
 float temperatureF = sensors.getTempFByIndex(0);
  capteurTemp=temperatureC;
#endif      
}


void mesure_puissance(){
#ifdef Pzem04t
  Pzem_i=pzem.current();
  Pzem_U=pzem.voltage();
  Pzem_P=pzem.power();
  Pzem_W=pzem.energy();
  puissanceMono=Pzem_P;
#endif 

}




/**************************************/
/******** regulation dans ballon*******/
/**************************************/
float xmax=0;
float xmin=0;
int devlente=0;
int devdecro=0;
//int devforte=0;
float tabxmin[]={0,0,0,0,0,0,0,0,0,0};
//float tabxmax[]={0,0,0,0,0,0,0,0,0,0};
int itab=0;

int mesureDerive(float x,float seuil){
  int dev=0;
 
  if (x>xmax)    {      
                dev=1;      xmax=x;      xmin=xmax-seuil; 
                  
                }  // si dépasse le seuil haut déplacement du seuil haut et bas
 
  if (x<xmin)    {  
                if ( (xmin-x)>10*seuil) devdecro=1; else devdecro=0;    // gestion du décrochage chute brutale du courant
                dev=-1;      xmin=x;      xmax=xmin+seuil;   
                
                
                for (int i=0;i<9;i++) tabxmin[9-i]= tabxmin[8-i]; tabxmin[0]=xmin; // range les valeurs mini 
                float deltaxmin=0;
                float xminmoy=tabxmin[0];
    //            float ondulxmin=0;
                
                for (int i=1;i<10;i++) { deltaxmin+=tabxmin[i]-tabxmin[0];  }       // calcule la pente négative 
     //           for (int i=0;i<9;i++) { ondulxmin+=abs(tabxmin[i]-tabxmin[i+1]); }
                if (itab<10)itab++;  // retire les 10 premiers points avant de calculer
                if ((itab>=10) && (deltaxmin>10*seuil)) devlente=1; else devlente=0;
      //          if ((itab>=10) && (ondulxmin>40*seuil)) devforte=1; else devforte=0;
                } // si descend en dessous du seuil bas déplacement du seuil haut et bas
 return dev;
}


int calPuis =0;
int calPuisav =0;
int ineg=0;
int devprevious = 0;
int devcount = 0;
int puisGradmax=0;
int tesTension=0;
#define variation_lente 1       // config 5
#define variation_normale 10   // config 10
#define variation_rapide 20   // config 20
#define bridePuissance 900   // sur 1000


int regulGrad(int dev){
    calPuisav =calPuis;
   if (puisGradmax<calPuis) puisGradmax=calPuis;
   if ((Pince <0)&&(Pince >-routeur.toleranceNeg)) Pince=Pince + routeur.toleranceNeg; //correction des mesure proche de zéro
   if (Pince<0)devcount = 0;
   if(devprevious == dev)  {if(devcount<100)devcount++;}  else devcount = 0; 
   if (dev==0) 
     {
      calPuis+= variation_lente;
      if(devcount > 3) calPuis += variation_normale; 
      if ((devcount > 7)&&(calPuis<puisGradmax-50)) calPuis += variation_rapide; 
      }
    if (dev>0) 
    {      
      calPuis+= variation_lente;
      if(devcount > 5) calPuis += variation_normale; 
      if ((devcount > 7)&&(calPuis<puisGradmax)) calPuis += variation_rapide; 
    } 
    if ((dev<0) || (Pince<0))
     {
      if (Pince<2)calPuis-= variation_normale+variation_lente; else calPuis-= variation_rapide+variation_lente;
      if (devcount > 2) calPuis -= variation_normale; 
      if((devcount > 5)&& (Pince>2)) calPuis -= variation_rapide; 
  
      if(devlente == 1) calPuis -= variation_normale; 
 //     if(devforte == 1) calPuis -= variation_normale; 
     if(devdecro==1) calPuis -= variation_normale;

     }
  if ((Pince>2)&&(devlente==0))  { calPuis+=variation_rapide ;}   // si la batterie commence à se décharger 
  if ((Pince<2)&& (Pince>0.2)&&(devlente==0))  { calPuis+=variation_lente ;}   // si la batterie commence à se décharger 

   if (Pince<0)  { calPuis=calPuisav-variation_rapide; }   // si la batterie est déchargée complètement 
   if (Pince<0)  {                                          // si le courant est trop longtemps négatif
                  if (ineg<100) { ineg++; puisGradmax-=variation_normale; }
                  if (ineg>10){puisGradmax=0;calPuis=0;}
                  } else ineg=0;
//   if (Pince<-1)  { puisGradmax=3*calPuis/4; calPuis=3*calPuis/4;}// autorisation de tirer 500mA max sur les batteries
 //  if (Pince<-1)  { puisGradmax=3*calPuis/4; calPuis=0;}// autorisation de tirer 500mA max sur les batteries
   
   devprevious = dev;
   if (capteurTension>routeur.seuilTension+0.5) tesTension=1;
  if (capteurTension<routeur.seuilTension-0.5) tesTension=0;
  if (tesTension==0) calPuis=0;
   calPuis     = min(max(0 , calPuis) , bridePuissance);   
   return(calPuis);
}

/***************************************/
/******** Programme principal   ********/
/***************************************/

void setup()
{
  Serial.begin(115200);
 
  pinMode(pinTriac, OUTPUT);   
  digitalWrite(pinTriac, LOW);  // mise à zéro du triac au démarrage
  pinMode(pinSortie2, OUTPUT);   
  digitalWrite(pinSortie2, LOW);  // mise à zéro du triac de la sortie 2
  pinMode(pinRelais, OUTPUT);   
  digitalWrite(pinRelais, HIGH);  // mise à zéro du relais statique
  pinMode(pinTemp, INPUT);
  pinMode(pinTension, INPUT);
  pinMode(pinPotentiometre, INPUT);
  pinMode(pinPince, INPUT);
  pinMode(pinPinceRef, INPUT);
       Serial.println();
       Serial.println("definition ok ");
  

#ifdef RelaisTemperature
routeur.sortierelaisTemp = true;
routeur.sortierelaisTens = false;
#endif

#ifdef RelaisTension
routeur.sortierelaisTemp = false;
routeur.sortierelaisTens = true;
#endif

#ifdef EEprom
   int configeeprom=0;
  EEPROM.begin(EEPROM_SIZE);
 if(EEPROM.read(0)!=123)  sauve_param(); // action qui se fait au premier démarrage sauve les parametres par défaut
 restore_param();                        // lecture de l'EEprom pour charger les paramètres en cas de coupure de courant
   Serial.println();
   Serial.print("Lecture EEPROM  seuil tension  : ");
   Serial.println(routeur.seuilTension);
 #endif 

// programmation des sorties gradateurs et relais
routeur.sortie2 = false;
#ifdef Sortie2
routeur.sortie2 = true;
#endif


 
  delay(1000); // give me time to bring up serial monitor

 
 #ifdef WifiMqtt
 
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connection au WiFi..");
  }
  Serial.println("WiFi actif");
 
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
  while (!client.connected()) {
    Serial.println("Connection au broker MQTT...");
 
    if (client.connect("ESP32Client", mqttUser, mqttPassword )) { 
      Serial.println("connection active");
    } else 
    {
 
      Serial.print("Erreur de connection ");
      Serial.print(client.state());
      delay(2000);
 
    }
  }
 paramchange=1;           // communication au format mqtt pour le reseau domotique 1 pour l'envoi des parametres
    
#endif

#ifndef WifiMqtt
#ifdef WifiServer
   WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    
    server.begin();

#endif
#endif






#ifdef EcranOled
   Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Disable*/, true /*Serial Enable*/);
   Heltec.display->setContrast(125);
    cls();
#endif 

#ifdef MesureTemperature
  sensors.begin();
#endif 

#ifdef Pzem04t
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
#endif 

  timer_config();
  zeroCross_config();


}    

/***************************************/
/******** Programme simulation   ********/
/***************************************/
int temps=0;
int itemps=0;
#define ond 300

void imageMesure(int tempo){
  if (temps<50)   Pince=(-10+1.0*random(ond/2)/10)/10;
  if ((temps>=50) && (temps<150))   { 
                                  itemps++;
                                   Pince=(100-itemps+1.0*random(ond)/10)/10;
                                  }
  if ((temps>=150) && (temps<250))   { 
                                  itemps++;
                                   Pince=(itemps+1.0*random(ond)/10)/10;
                                  }
  if ((temps>=250)&& (temps<350))   Pince=(100+1.0*random(ond/2)/10)/10;
  if ((temps>=350)&& (temps<450))   Pince=(50+1.0*random(ond/2)/10)/10;
  if ((temps>=450)&& (temps<550))   Pince=(-100+1.0*random(ond/2)/10)/10;
  temps++;
  if (temps>550) { temps=0; itemps=0; }
 delay(tempo);
  capteurTension=(57.0*100+random(100))/100;
 // capteurTemp=(20.0*100+random(100))/100;
  puissanceMono=(10*puissanceMono+random(1000))/11;
//  capteurTemp=25;
}

unsigned long tempdepart;
int tempo=0;

void pilotage(){
  // pilotage du 2eme triac 
#ifdef MesureTemperature
#ifdef Sortie2
  if (routeur.sortie2) {
                if ((capteurTemp>routeur.sortie2_tempHaut)&&(choixSortie==0)) { choixSortie=1; routeur.sortieActive=2; } 
                // commande du gradateur2
                if ((capteurTemp<routeur.sortie2_tempBas)&&(choixSortie==1)) { choixSortie=0;  routeur.sortieActive=1; }
                // commande du gradateur1
                }
#endif                

  if (routeur.sortieRelaisTemp){
                if (capteurTemp>routeur.relaisMax)  digitalWrite(pinRelais, LOW);    // mise à un du relais statique
                if (capteurTemp<routeur.relaisMin)  digitalWrite(pinRelais, HIGH);     // mise à zéro du relais statique
                }
#endif 
               
  if  (routeur.sortieRelaisTens){
                if (capteurTemp>routeur.relaisMax)  digitalWrite(pinRelais, LOW);    // mise à un du relais statique
                if (capteurTemp<routeur.relaisMin)  digitalWrite(pinRelais, HIGH);     // mise à zéro du relais statique
                }
  if  ((routeur.maForce)&&(tempo==0)){
                        tempdepart=millis();                                          //  memorisation de moment de depart
                        tempo=1;
                 }
  if  ((routeur.maForce)&&(tempo==1)){
                      PuisGrad=routeur.maForceval*9;                                  //limitation 90%
                      if (millis()>tempdepart+routeur.temporisation*60000)            // durée de forcage en milliseconde
                      {
                        routeur.maForce=false;
                        tempo=0;
                        paramchange=1;
                      }
                 }

                
}


 

void loop()
{
    int potar=map(analogRead(pinPotentiometre), 0, 4095, 0, 1000); // controle provisoire avec pot
    mesurePinceTension(700,20);                                   // mesure le courant et la tension avec 2 boucles de filtrage (700,20)

#ifndef Parametrage
    mesureTemperature();                    // mesure la temperature sur le ds18b20
    mesure_puissance();                     // mesure la puissance sur le pzem004t
 //imageMesure(0);                          // permet de faire des essais sans matériel
    int dev=mesureDerive(Pince,0.2);        // donne 1 si ça dépasse l'encadrement haut et -1 si c'est en dessous de l'encadrement (Pince,0.2)
    PuisGrad=regulGrad(dev);                // calcule l'augmentation ou diminution du courant dans le ballon en fonction de la deviation
    pilotage();                             // utilisation de la sortie 2 , du forcage 25% 50% ...
#endif


#ifdef Parametrage
   if (potar>10) PuisGrad=potar;
   else  if (potar<2) PuisGrad=0;   
                else PuisGrad=1; // priorité au potensiometre                    // priorité au potensiometre
    Serial.println();
    Serial.print("Courant "); Serial.println(Pince);
    Serial.print("tension "); Serial.println(capteurTension);
    Serial.print("Gradateur "); Serial.println(PuisGrad);
    Serial.println();
    delay(1000);
#endif

// affichage traceur serie
 float c = PuisGrad;
  Serial.print(c/100); Serial.print(',');Serial.print(devlente*2); Serial.print(',');   Serial.print(Pince);  Serial.print(',');Serial.print(56/5);  Serial.print(','); Serial.println(capteurTension/5);
 

#ifdef EcranOled
    affichage_oled();  // affichage de lcd
#endif


#ifdef WifiMqtt
    if ((WiFi.status() == WL_CONNECTED) && (client.connected())){  // teste le wifi
    if (paramchange==1) Mqtt_publish(1); else Mqtt_publish(0);  // communication au format mqtt pour le reseau domotique 
                                                                //  (1) avec les parametres
                                                                //  (0) seulement les indispensables tension courant .. 
    paramchange=0;
    Mqtt_subcrive();                                            // lecture de information sur mqtt output/solar
    testwifi=0;
    }
    else
    {
      Serial.print("Perte de connexion redemarrage en cours ");Serial.println(500-testwifi);
      testwifi++;
    if (testwifi>500) resetEsp=1;   // si perte de signal trop longtemps reset esp32
    }
#endif


#ifdef WifiServer
#ifndef WifiMqtt
    
    serveurHTML();

#endif
#endif



#ifdef EEprom
 if (resetEsp==1)
 {
      timerAlarmDisable(timer);     // stop alarm
      timerDetachInterrupt(timer);  // detach interrupt
      timerEnd(timer);
       sauve_param();
      EEPROM.end();                 // pour liberer la memoire
      delay(5000);
 
 }
#endif



 if (resetEsp==1)
      {
      ESP.restart();  // redemarrage
      }

}
