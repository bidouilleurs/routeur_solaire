/***************************************/
/******** mode MQTT   ********/
/***************************************/
#include "settings.h"
#ifdef WifiMqtt
#include "modemqtt.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include "WiFi.h"
/************ WiFi & MQTT objects  ******************/
WiFiClient espClient;
PubSubClient client(espClient);
int affpub = 0;
int testconnect = 0; //  permet de savoir si la connection est faite

void RAMQTTClass::setup()
{

 
  if (!SAP)
  {
    WiFi.begin(routeur.ssid, routeur.password); // connection au reseau 20 tentatives autorisées
    while ((testconnect < 20) && (WiFi.status() != WL_CONNECTED))
    {
      delay(500);
      Serial.println(F("Connection au WiFi.."));
      testconnect++;
    }
    if (testconnect < 20)
    { // connection réussie
      Serial.println(F("WiFi actif"));
      SAP = false;
      MQTT = true;
      client.setServer(routeur.mqttServer, routeur.mqttPort); // connection au broker mqtt
      client.setCallback(callback);
      Serial.print(F("le broker est  "));
      Serial.println(routeur.mqttServer);
      testconnect = 0;
      while ((testconnect < 5) && (!client.connected()))
      { // connection au broker 5 tentatives autorisée
        testconnect++;
        Serial.println(F("Connection au broker MQTT..."));
        if (client.connect("ESP32Client", routeur.mqttUser, routeur.mqttPassword))
        {
          Serial.println(F("connection active"));
        }
        else
        {
          Serial.print(F("Erreur de connection "));
        }
        Serial.print(client.state());
        delay(100);
      }
      if (testconnect >= 5) // connection échouée au broker
      {
        Serial.println(F("Broker Mqtt indisponible"));
        MQTT = false; // pas de connection mqtt
      }
    }
    else
    {
      Serial.println(F("Connection au WiFi absente .... passage en mode serveur"));
      SAP = true; // mode poit d'accès activé
    }
  }
  paramchange = 1; // communication au format mqtt pour le reseau domotique 1 pour l'envoi des parametres
}

/************************* passage de parametre par BT ou MQTT  *********************************/
void RAMQTTClass::commande_param(String mesg)
{
  Serial.println(mesg);
  if (mesg == "rst")
    resetEsp = 1;
  if (mesg.substring(0, 3).equals("bat")) // reception ex: "bat51" 51 est converti en seuil batterie
  {
    int com = mesg.substring(3).toInt();
    routeur.seuilDemarrageBatterie = com;
  }
  if (mesg.substring(0, 3).equals("neg")) // reception ex: "neg0.6" autorise un seuil de démarrage
  {
    float com = mesg.substring(3).toFloat();
    routeur.toleranceNegative = com;
  }
  if (mesg.substring(0, 3).equals("sth")) // reception ex: "sth51" 51 seuil de temperature haute
  {
    int com = mesg.substring(3).toInt();
    routeur.temperatureBasculementSortie2 = com;
  }
  if (mesg.substring(0, 3).equals("stb")) // reception ex: "stb51" 51 seuil de temperature basse
  {
    int com = mesg.substring(3).toInt();
    routeur.temperatureRetourSortie1 = com;
  }
  if (mesg.substring(0, 3).equals("rth")) // reception ex: "rth51" 51 relais tension haut
  {
    int com = mesg.substring(3).toInt();
    routeur.seuilMarche = com;
  }
  if (mesg.substring(0, 3).equals("rtb")) // reception ex: "rtb51" 51 relais tension bas
  {
    int com = mesg.substring(3).toInt();
    routeur.seuilArret = com;
  }

#ifdef Sortie2
  if (mesg.substring(0, 3).equals("sor")) // reception ex: "sor1"  ou "sor0"  pour la commande du 2eme gradateur
  {
    int com = mesg.substring(3).toInt();
    if (com == 1)
      routeur.utilisation2Sorties = true;
    else
      routeur.utilisation2Sorties = false;
  }
#endif

  if (mesg.substring(0, 3).equals("cmf")) // reception ex: "cmf1"  ou "cmf0"  pour la commande de la marche forcee
  {
    int com = mesg.substring(3).toInt();
    if (com == 1)
      marcheForcee = true;
    else
      marcheForcee = false;
  }
  if (mesg.substring(0, 3).equals("rmf")) // reception ex: "rmf25" 25% ratio marche forcée
  {
    int com = mesg.substring(3).toInt();
    Serial.println(com);
    marcheForceePercentage = com;
  }
  if (mesg.substring(0, 3).equals("rel")) // reception ex: "rel0" le relais température et tension à zéro rel1 pour temp rel2 tension
  {
    int com = mesg.substring(3).toInt();
    if (com == 0)
    {
      strcpy(routeur.tensionOuTemperature, "");
        routeur.relaisStatique = false;
    }
    if (com == 1)
    {
      strcpy(routeur.tensionOuTemperature, "D");
        routeur.relaisStatique = true;

    }
    if (com == 2)
    {
      strcpy(routeur.tensionOuTemperature, "V");
        routeur.relaisStatique = true;
    }
  }
  if (mesg.substring(0, 3).equals("tmp")) // reception ex: "tmp60" 60 minute de marche forcée
  {
    int com = mesg.substring(3).toInt();
    temporisation = com;
  }
  paramchange = 1;
}
void RAMQTTClass::callback(char *topic, byte *message, unsigned int length)
{
  Serial.print(F("Message arrived on topic: "));
  Serial.print(topic);
  Serial.print(F(". Message: "));

  String messageTemp;

  for (int i = 0; i < length; i++)
  {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();
  if (String(topic) == routeur.mqttopicInput)
  {
    RAMQTT.commande_param(messageTemp);
  }
  // Mqtt_publish(1);
}

void RAMQTTClass::mqtt_subcribe()
{
  if (client.connected())
  {
    client.subscribe(routeur.mqttopicInput);
  }
  client.loop();
}

void RAMQTTClass::mqtt_publish(int a)
{

  if (a > 0)
    affpub = 30; // forcage de l'envoie
  affpub++;
  if (affpub < 20)
    return;
  else
    affpub = 0;

  const int capacity = JSON_OBJECT_SIZE(5); // 5 données maxi dans json
  StaticJsonDocument<capacity> doc;
  char buffer[200];
  //Exportation des données en trame JSON via MQTT
  doc["Intensite"] = intensiteBatterie;
  doc["Tension"] = capteurTension;
  doc["Gradateur"] = puissanceGradateur;
  doc["Temperature"] = temperatureEauChaude;
  doc["puissanceMono"] = puissanceDeChauffe;
  size_t n = serializeJson(doc, buffer); // calcul de la taille du json
  buffer[n - 1] = '}';   buffer[n] = 0; // fermeture de la chaine json
  Serial.println(buffer);
  if (client.connected())
  {
    if (client.publish(routeur.mqttopic, buffer, n) == true)     {      Serial.println(F("Success sending message"));
    }    else    {      Serial.println(F("Error sending message"));    }
  }
   doc.clear();

  client.loop();
  if (a == 0)
    return;

  //   char buffer[150];
  //Exportation des données en trame JSON via MQTT
  StaticJsonDocument<capacity> doc2;
  doc2["zeropince"] = routeur.zeropince;
  doc2["coeffPince"] = routeur.coeffPince;
  doc2["coeffTension"] = routeur.coeffTension;
  doc2["seuilTension"] = routeur.seuilDemarrageBatterie;
  doc2["toleranceNeg"] = routeur.toleranceNegative;
  n = serializeJson(doc2, buffer); // calcul de la taille du json
  buffer[n - 1] = '}';
  buffer[n] = 0; // fermeture de la chaine json
  Serial.println(buffer);
  if (client.connected())   {
    if (client.publish(routeur.mqttopicParam1, buffer, n) == true)     {       Serial.println(F("Success sending message param"));
    }     else     {       Serial.println(F("Error sending message param"));     }
  }
   doc2.clear();

  StaticJsonDocument<capacity> doc3;
  doc3["sortie2"] = routeur.utilisation2Sorties;
  doc3["sortie2_tempHaut"] = routeur.temperatureBasculementSortie2;
  doc3["sortie2_tempBas"] = routeur.temperatureRetourSortie1;
  doc3["temporisation"] = temporisation;
  doc3["sortieActive"] = sortieActive;
  n = serializeJson(doc3, buffer); // calcul de la taille du json
  buffer[n - 1] = '}';  buffer[n] = 0; // fermeture de la chaine json
  Serial.println(buffer);
  if (client.connected())
  {
    if (client.publish(routeur.mqttopicParam2, buffer, n) == true)    {       Serial.println(F("Success sending message param"));
    }     else     {       Serial.println(F("Error sending message param"));     }
  }
   doc3.clear();
  
  StaticJsonDocument<capacity> doc4;
  doc4["sortieRelaisTemp"] = (routeur.relaisStatique && (routeur.tensionOuTemperature[0]=='D'));
  doc4["sortieRelaisTens"] = (routeur.relaisStatique && (routeur.tensionOuTemperature[0]=='V'));
  doc4["relaisMax"] = routeur.seuilMarche;
  doc4["relaisMin"] = routeur.seuilArret;
  doc4["Forcage_1h"] = marcheForcee;

  
  n = serializeJson(doc4, buffer); // calcul de la taille du json
  buffer[n - 1] = '}';
  buffer[n] = 0; // fermeture de la chaine json
  Serial.println(buffer);
  if (client.connected())
  {
    if (client.publish(routeur.mqttopicParam3, buffer, n) == true)
    {
      Serial.println(F("Success sending message param"));
    }
    else
    {
      Serial.println(F("Error sending message param"));
    }
  }
   doc4.clear();

  client.loop();

  //  sortie du pzem
#ifdef Pzem04t

  StaticJsonDocument<capacity> doc5; // ajout d'un json pour le Pzem
                                     //  char buffer[400];
  //Exportation des données en trame JSON via MQTT
  doc5["Intensite"] = Pzem_i;
  doc5["Tension"] = Pzem_U;
  doc5["Puissance"] = Pzem_P;
  doc5["Energie"] = Pzem_W;
  doc5["Cosf"] = 0;

  n = serializeJson(doc5, buffer); // calcul de la taille du json
  buffer[n - 1] = '}';
  buffer[n] = 0; // fermeture de la chaine json
  Serial.println(buffer);
  if (client.connected())
  {
    if (client.publish(routeur.mqttopicPzem1, buffer, n) == true)
    {
      Serial.println(F("Success sending message"));
    }
    else
    {
      Serial.println(F("Error sending message"));
    }
  }
   doc5.clear();

  /********************* fin d'envoie ***********************/
  client.loop();
#endif
}

void RAMQTTClass::loop()
{
  if ((testwifi == 0) && (!SAP) && (MQTT) && (!serverOn))
  {
    if ((WiFi.status() == WL_CONNECTED) && (client.connected()))
    {                  // teste le wifi
      mqtt_subcribe(); // lecture de information sur mqtt output/solar
                       //  (1) avec les parametres
      //  (0) seulement les indispensables tension courant ..
      if (paramchange == 1)
      {
        mqtt_publish(1);
      }
      else
      {
        mqtt_publish(0); // communication au format mqtt pour le reseau domotique
      }

      paramchange = 0;
    }
    else
      testwifi++;
  }

  if (testwifi != 0)
  {
    Serial.print(F("Perte de connexion redemarrage en cours "));
    Serial.println(500 - testwifi);
    testwifi++;
    if (testwifi > 500)
      resetEsp = 1; // si perte de signal trop longtemps reset esp32
  }

  if (SAP)
  {
    Serial.print(F("Mode serveur ESP"));
  }
}

RAMQTTClass RAMQTT;
#endif
