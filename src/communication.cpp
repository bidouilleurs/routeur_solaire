/***************************************/
/******** mode MQTT   ********/
/***************************************/
#include "settings.h"
#include "communication.h"
#include <PubSubClient.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <string.h>
#include <SPIFFS.h>
#include "WiFi.h"
#include "afficheur.h"
#include "prgEEprom.h"
#include "triac.h"
#include "regulation.h"

WiFiClient espClient;
PubSubClient clientMqtt(espClient);
WiFiServer server(80);
IPAddress local_ip(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

void RACommunicationClass::creationSAP()
{
  const char *ssid = "routeur_esp32"; // mode point d'accès
  const char *password = "adminesp32";
  WiFi.enableAP(true);
  delay(100);
  WiFi.softAP(ssid, password); // l'esp devient serveur et se place à l'adresse 192.168.4.1
  WiFi.softAPConfig(local_ip, gateway, subnet);
  IPAddress myIP = WiFi.softAPIP();
  RACommunication.print(1, "AP IP address: ", false);
  RACommunication.print(1, (char *)myIP.toString().c_str(), true);
  SAPActif = true;
}

void RACommunicationClass::setup(const char *version_soft)
{
  versionsoft = version_soft;
  clientMqtt.setServer(routeur.mqttServer, routeur.mqttPort); // connection au broker mqtt
  clientMqtt.setCallback(callback);
  clientMqtt.setBufferSize(500);
  if (SAP || routeur.utilisationSAP)
  {
    creationSAP();
  }
  else
  {
    RACommunication.print(1, "le broker est ", false);
    RACommunication.print(1, routeur.mqttServer, false);
    RACommunication.print(1, ":", false);
    char port[4];
    sprintf(port, "%d", routeur.mqttPort);
    RACommunication.print(1, port, true);

    RACommunication.reconnect();
  }

  if (!SPIFFS.begin())
  {
    RACommunication.print(1, "Erreur SPIFFS...", true);
    return;
  }
#ifdef WifiServer
  server.begin();
#endif
  paramchange = 1; // communication au format mqtt pour le reseau domotique 1 pour l'envoi des parametres
}

void RACommunicationClass::reconnect()
{

  bool wifiActif = WiFi.status() == WL_CONNECTED;
  if (wifiActif)
  {
#ifdef WifiMqtt
    RACommunication.connexion();
#endif
    return;
  }
  else
  {
    WiFi.begin(routeur.ssid, routeur.password);
    RACommunication.print(3, "Connection au WiFi..", true);
  }
  testconnectWIFI = 0;
  char msg[20];
  while ((testconnectWIFI < maxTentativeConnectWifi) && (WiFi.status() != WL_CONNECTED))
  {
    delay(500);
    testconnectWIFI++;

    sprintf(msg, "Tentative %d/%d", testconnectWIFI, maxTentativeConnectWifi);
    RACommunication.print(3, msg, true);
  }

  if (!wifiActif && WiFi.status() == WL_CONNECTED)
  { // connection réussie
    wifiActif = true;
    RACommunication.print(3, "WiFi actif", true);
    SAP = false;
#ifdef WifiMqtt
    RACommunication.connexion();
#endif
  }
  else
  {
    if (!SAPActif)
    {
      RACommunication.print(3, "Connection au WiFi absente .... passage en mode point d accès (SAP)", true);
      SAP = true; // mode point d'accès activé
      creationSAP();
    }
  }
}

void RACommunicationClass::connexion()
{
  testconnectMQTT = 0;
  char msg[20];
  char numberTentative[3];
  while ((testconnectMQTT < maxTentativeConnectMQTT) && (!clientMqtt.connected())) // connection au broker 'maxTentativeConnectMQTT' tentatives autorisée
  {
    testconnectMQTT++;
    RACommunication.print(3, "Connection au broker MQTT...", true);
    if (clientMqtt.connect("ESP32Client", routeur.mqttUser, routeur.mqttPassword))
    {
      paramchange = 1;
      MQTT = true;
      RACommunication.print(3, "connection active", true);
      RACommunication.mqtt_subscribe();
      loopBeforeReconnect = 0;
    }
    else
    {
      sprintf(msg, "Tentative %d/%d", testconnectMQTT, maxTentativeConnectMQTT);
      RACommunication.print(3, msg, true);
#ifdef EcranOled
      RAAfficheur.cls();
      delay(100);
      sprintf(numberTentative, "%d", testconnectMQTT);
      RAAfficheur.affiche(20, "Conn MQTT");
      RAAfficheur.affiche(35, numberTentative);
#endif
      delay(100);
    }
  }
  if (!clientMqtt.connected()) // connection échouée au broker
  {
    RACommunication.print(3, "Broker Mqtt indisponible", true);
    MQTT = false; // pas de connection mqtt
    loopBeforeReconnect = 0;
  }
}

void RACommunicationClass::callback(char *topic, byte *message, unsigned int length)
{
  RACommunication.print(3, "Message arrived on topic: ", false);
  RACommunication.print(3, topic, false);
  RACommunication.print(3, ". Message: ", false);
  char messageTemp[length + 1];

  for (int i = 0; i < length; i++)
  {
    messageTemp[i] = (char)message[i];
  }
  messageTemp[length] = NULL;
  RACommunication.print(3, messageTemp, true);

  if (strcmp(topic, routeur.mqttopicInput) == 0)
  {
    RACommunication.commande_param(messageTemp);
  }
  else if (strcmp(topic, "router/activation") == 0)
  {
    routeur.actif = strcmp(messageTemp, "1") == 0 ? true : false;
    if (routeur.actif)
    {
      RAPrgEEprom.sauve_param();
      RATriac.start_interrupt();
    }
    else
    {
      RATriac.stop_interrupt();
      RAPrgEEprom.sauve_param();
    }
  }
}

void RACommunicationClass::mqtt_subscribe()
{
  if (clientMqtt.connected())
  {
    clientMqtt.subscribe(routeur.mqttopicInput);
    clientMqtt.subscribe("router/activation");
  }
}

void RACommunicationClass::mqtt_publish(int a)
{

  if (a > 0)
  {
    affpub = 30; // forcage de l'envoie
  }
  affpub++;
  if (affpub < 20)
  {
    return;
  }
  else
  {
    affpub = 0;
  }
  StaticJsonDocument<JSON_OBJECT_SIZE(7)> doc;

  //Exportation des données en trame JSON via MQTT
  doc["Intensite"] = intensiteBatterie;
  doc["Tension"] = capteurTension;
  doc["Gradateur"] = puissanceGradateur;
  doc["Temperature"] = temperatureEauChaude;
  doc["puissanceMono"] = puissanceDeChauffe;
  doc["mesureAC"] = mesureAc;
  doc["rssi"] = WiFi.RSSI();
  size_t n = serializeJson(doc, buffer); // calcul de la taille du json
  buffer[n - 1] = '}';
  buffer[n] = 0; // fermeture de la chaine json
  RACommunication.print(3, buffer, true);
  if (clientMqtt.connected())
  {
    if (clientMqtt.publish(routeur.mqttopic, buffer, n) == true)
    {
      RACommunication.print(3, "Success sending message", true);
    }
    else
    {
      RACommunication.print(2, "Error sending message", true);
    }
  }
  doc.clear();

  if (a == 0)
    return;

  //   char buffer[150];
  //Exportation des données en trame JSON via MQTT
  StaticJsonDocument<JSON_OBJECT_SIZE(6)> doc2;
  doc2["zeropince"] = routeur.zeropince;
  doc2["coeffPince"] = routeur.coeffPince;
  doc2["coeffTension"] = routeur.coeffTension;
  doc2["seuilTension"] = routeur.seuilDemarrageBatterie;
  doc2["toleranceNeg"] = routeur.toleranceNegative;
  doc2["actif"] = routeur.actif;
  n = serializeJson(doc2, buffer); // calcul de la taille du json
  buffer[n - 1] = '}';
  buffer[n] = 0; // fermeture de la chaine json
  RACommunication.print(3, buffer, true);
  if (clientMqtt.connected())
  {
    if (clientMqtt.publish(routeur.mqttopicParam1, buffer, n) == true)
    {
      RACommunication.print(3, "Success sending message", true);
    }
    else
    {
      RACommunication.print(2, "Error sending message param", true);
    }
  }
  doc2.clear();
  const int capacity = JSON_OBJECT_SIZE(5); // 5 données maxi dans json

  StaticJsonDocument<capacity> doc3;
  doc3["sortie2"] = routeur.utilisation2Sorties;
  doc3["sortie2_tempHaut"] = routeur.temperatureBasculementSortie2;
  doc3["sortie2_tempBas"] = routeur.temperatureRetourSortie1;
  doc3["temporisation"] = temporisation;
  doc3["sortieActive"] = sortieActive;

  n = serializeJson(doc3, buffer); // calcul de la taille du json
  buffer[n - 1] = '}';
  buffer[n] = 0; // fermeture de la chaine json
  RACommunication.print(3, buffer, true);
  if (clientMqtt.connected())
  {
    if (clientMqtt.publish(routeur.mqttopicParam2, buffer, n) == true)
    {
      RACommunication.print(3, "Success sending message param", true);
    }
    else
    {
      RACommunication.print(2, "Error sending message param", true);
    }
  }
  doc3.clear();

  StaticJsonDocument<JSON_OBJECT_SIZE(8)> doc4;
  doc4["sortieRelaisTemp"] = (routeur.relaisStatique && (routeur.tensionOuTemperature[0] == 'D'));
  doc4["sortieRelaisTens"] = (routeur.relaisStatique && (routeur.tensionOuTemperature[0] == 'V'));
  doc4["relaisMax"] = routeur.seuilMarche;
  doc4["relaisMin"] = routeur.seuilArret;
  doc4["Forcage_1h"] = marcheForcee;
  doc4["version"] = versionsoft;
  doc4["seuilCoupureAC"] = routeur.seuilCoupureAC;
  doc4["coeffMesureAc"] = routeur.coeffMesureAc;

  n = serializeJson(doc4, buffer); // calcul de la taille du json
  buffer[n - 1] = '}';
  buffer[n] = 0; // fermeture de la chaine json
  RACommunication.print(3, buffer, true);
  if (clientMqtt.connected())
  {
    if (clientMqtt.publish(routeur.mqttopicParam3, buffer, n) == true)
    {
      RACommunication.print(3, "Success sending message param", true);
    }
    else
    {
      RACommunication.print(2, "Error sending message param", true);
    }
  }
  doc4.clear();

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
  RACommunication.print(3, buffer, true);
  if (clientMqtt.connected())
  {
    if (clientMqtt.publish(routeur.mqttopicPzem1, buffer, n) == true)
    {
      RACommunication.print(3, "Success sending message", true);
    }
    else
    {
      RACommunication.print(2, "Error sending message param", true);
    }
  }
  doc5.clear();

  /********************* fin d'envoie ***********************/
#endif
}

/************************* passage de parametre par BT ou MQTT  *********************************/
void RACommunicationClass::commande_param(char *mesg)
{
  Serial.print("new message ");
  Serial.println(mesg);
  char msg[4];
  char value[4];
  memcpy(msg, &mesg[0], 3);
  Serial.println(msg);
  memcpy(value, &mesg[3], strlen(mesg));
  msg[3] = '\0';
  if (strcmp(mesg, "rst") == 0)
  {
    resetEsp = 1;
  }
  else if (strcmp(msg, "bat") == 0) // reception ex: "bat51" 51 est converti en seuil batterie
  {
    routeur.seuilDemarrageBatterie = atof(value);
  }
  else if (strcmp(msg, "neg") == 0) // reception ex: "neg0.6" autorise un seuil de démarrage
  {
    routeur.toleranceNegative = atof(value);
  }
  else if (strcmp(msg, "sth") == 0) // reception ex: "sth51" 51 seuil de temperature haute
  {
    routeur.temperatureBasculementSortie2 = atof(value);
  }
  else if (strcmp(msg, "stb") == 0) // reception ex: "stb51" 51 seuil de temperature basse
  {
    routeur.temperatureRetourSortie1 = atof(value);
  }
  else if (strcmp(msg, "rth") == 0) // reception ex: "rth51" 51 relais tension haut
  {
    routeur.seuilMarche = atof(value);
  }
  else if (strcmp(msg, "rtb") == 0) // reception ex: "rtb51" 51 relais tension bas
  {
    routeur.seuilArret = atof(value);
  }

#ifdef Sortie2
  if (strcmp(msg, "sor") == 0) // reception ex: "sor1"  ou "sor0"  pour la commande du 2eme gradateur
  {
    float com = atof(value);
    if (com == 1)
      routeur.utilisation2Sorties = true;
    else
      routeur.utilisation2Sorties = false;
  }
#endif

  if (strcmp(msg, "cmf") == 0) // reception ex: "cmf1"  ou "cmf0"  pour la commande de la marche forcee
  {
    float com = atof(value);
    if (com == 1)
      marcheForcee = true;
    else
      marcheForcee = false;
  }
  if (strcmp(msg, "rmf") == 0) // reception ex: "rmf25" 25% ratio marche forcée
  {
    float com = atof(value);
    marcheForceePercentage = (int)com;
  }
  if (strcmp(msg, "rel") == 0) // reception ex: "rel0" le relais température et tension à zéro rel1 pour temp rel2 tension
  {
    float com = atof(value);
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
  if (strcmp(msg, "tmp") == 0) // reception ex: "tmp60" 60 minutes de marche forcée
  {
    temporisation = (long)atof(value);
  }
#ifndef MesureTemperature
  if (strcmp(msg, "tem") == 0) // reception ex: "tem60" indique que le ballon est a 60 degrés
  {
    int com = mesg.substring(3).toInt();
    temperatureEauChaude = com;
  }
#endif
  paramchange = 1;
}

DynamicJsonDocument RACommunicationClass::readSettingsFile()
{
  File fileSettings = SPIFFS.open("/settings/settings.json", FILE_READ);

  if (!fileSettings)
  {
    RACommunication.print(2, "Erreur lors de la lecture du fichier contenant les paramètres", true);
  }
  std::string settings = "";
  while (fileSettings.available())
  {
    settings += fileSettings.read();
  }
  DynamicJsonDocument doc(4000);
  deserializeJson(doc, settings);
  fileSettings.close();
  return doc;
}

void RACommunicationClass::saveResumeSettings(String jsonResult)
{
  RATriac.stop_interrupt();
  StaticJsonDocument<1000> doc;
  deserializeJson(doc, jsonResult);
  marcheForcee = doc["marcheForcee"] == "true";
  marcheForceePercentage = atof(doc["marcheForceePercentage"]);
  temporisation = atof(doc["temporisation"]);
  doc.clear();
  RAPrgEEprom.sauve_param();
  pause_inter = 1;
}

void RACommunicationClass::saveMqttSettings(String jsonResult)
{
  RATriac.stop_interrupt();
  StaticJsonDocument<1000> doc;
  deserializeJson(doc, jsonResult);
  strcpy(routeur.mqttServer, doc["mqttServer"]);
  routeur.mqttPort = doc["mqttPort"];
  strcpy(routeur.mqttUser, doc["mqttUser"]);
  strcpy(routeur.mqttopic, doc["mqttopic"]);
  strcpy(routeur.mqttopicInput, doc["mqttopicInput"]);
  strcpy(routeur.mqttopicParam1, doc["mqttopicParam1"]);
  strcpy(routeur.mqttopicParam2, doc["mqttopicParam2"]);
  strcpy(routeur.mqttopicParam3, doc["mqttopicParam3"]);
  strcpy(routeur.mqttopicPzem1, doc["mqttopicPzem1"]);
  strcpy(routeur.mqttPassword, doc["mqttPassword"]);

  doc.clear();
  RAPrgEEprom.sauve_param();
  restartEsp = true;
  pause_inter = 1;
}

void RACommunicationClass::saveWifiSettings(String jsonResult)
{
  RATriac.stop_interrupt();
  StaticJsonDocument<1000> doc;
  deserializeJson(doc, jsonResult);
  strcpy(routeur.ssid, doc["ssid"]);
  strcpy(routeur.password, doc["password"]);
  routeur.utilisationSAP = doc["utilisationSAP"] == "true";
  doc.clear();
}

void RACommunicationClass::saveSystemSettings(String jsonResult)
{
  RATriac.stop_interrupt();
  StaticJsonDocument<1500> doc;
  deserializeJson(doc, jsonResult);
  routeur.coeffPince = atof(doc["coeffPince"]);
  routeur.zeropince = atof(doc["zeropince"]);
  routeur.coeffTension = atof(doc["coeffTension"]);
  routeur.correctionTemperature = atof(doc["correctionTemperature"]);
  routeur.seuilDemarrageBatterie = atof(doc["seuilDemarrageBatterie"]);
  routeur.toleranceNegative = atof(doc["toleranceNegative"]);
  routeur.utilisation2Sorties = doc["utilisation2Sorties"] == "true";
  routeur.temperatureBasculementSortie2 = atof(doc["temperatureBasculementSortie2"]);
  strcpy(routeur.basculementMode, doc["basculementMode"]);
  routeur.temperatureRetourSortie1 = atof(doc["temperatureRetourSortie1"]);
  strcpy(routeur.tensionOuTemperature, doc["tensionOuTemperature"]);
  routeur.relaisStatique = doc["relaisStatique"] == "true";
  routeur.seuilMarche = atof(doc["seuilMarche"]);
  routeur.seuilArret = atof(doc["seuilArret"]);
  routeur.utilisationPinceAC = doc["utilisationPinceAC"] == "true";
  if (routeur.utilisationPinceAC)
  {
    routeur.seuilCoupureAC = atof(doc["seuilCoupureAC"]);
    routeur.coeffMesureAc = atof(doc["coeffMesureAc"]);
  }

  routeur.actif = doc["actif"] == "true";
  if (!routeur.actif)
  {
    RARegulation.desactivation();
  }
  restartEsp = doc["needRestart"] == "true" ? 1 : 0;
  doc.clear();
  RAPrgEEprom.sauve_param();
  pause_inter = 1;
}

void RACommunicationClass::getNewSettings(WiFiClient clientWifi)
{
  DynamicJsonDocument doc(600);
  JsonObject settings = doc.createNestedObject("settings");
  settings["capteurTension"] = capteurTension;
  settings["intensiteBatterie"] = intensiteBatterie;
  settings["sortieActive"] = sortieActive;
  settings["temperatureEauChaude"] = temperatureEauChaude;
  settings["puissanceDeChauffe"] = puissanceDeChauffe;
  settings["puissanceGradateur"] = puissanceGradateur / 10;
  settings["temporisation"] = temporisation;
  settings["marcheForcee"] = marcheForcee;
  settings["etatRelaisStatique"] = etatRelaisStatique;
  settings["actif"] = routeur.actif;
  settings["mesureAc"] = mesureAc;
  String result = doc["settings"];
  doc.clear();
  clientWifi.println(result);
}

void RACommunicationClass::getSettings(WiFiClient clientWifi)
{
  DynamicJsonDocument doc = readSettingsFile();
  doc["settings"]["userSettings"]["marcheForcee"]["value"] = marcheForcee;
  doc["settings"]["userSettings"]["marcheForceePercentage"]["value"] = marcheForceePercentage;
  doc["settings"]["userSettings"]["temporisation"]["value"] = temporisation;
  doc["settings"]["userSettings"]["coeffPince"]["value"] = routeur.coeffPince;
  doc["settings"]["userSettings"]["zeropince"]["value"] = routeur.zeropince;
  doc["settings"]["userSettings"]["coeffTension"]["value"] = routeur.coeffTension;
  doc["settings"]["userSettings"]["correctionTemperature"]["value"] = routeur.correctionTemperature;
  doc["settings"]["userSettings"]["seuilDemarrageBatterie"]["value"] = routeur.seuilDemarrageBatterie;
  doc["settings"]["userSettings"]["toleranceNegative"]["value"] = routeur.toleranceNegative;
  doc["settings"]["userSettings"]["utilisation2Sorties"]["value"] = routeur.utilisation2Sorties;
  doc["settings"]["userSettings"]["basculementMode"]["value"] = routeur.basculementMode ? routeur.basculementMode : "T";
  doc["settings"]["userSettings"]["temperatureBasculementSortie2"]["value"] = routeur.temperatureBasculementSortie2;
  doc["settings"]["userSettings"]["temperatureRetourSortie1"]["value"] = routeur.temperatureRetourSortie1;
  doc["settings"]["userSettings"]["tensionOuTemperature"]["value"] = routeur.tensionOuTemperature;
  doc["settings"]["userSettings"]["relaisStatique"]["value"] = routeur.relaisStatique;
  doc["settings"]["userSettings"]["seuilMarche"]["value"] = routeur.seuilMarche;
  doc["settings"]["userSettings"]["seuilArret"]["value"] = routeur.seuilArret;

  doc["settings"]["systemSettings"]["capteurTension"]["value"] = capteurTension;
  doc["settings"]["systemSettings"]["intensiteBatterie"]["value"] = intensiteBatterie;
  doc["settings"]["systemSettings"]["sortieActive"]["value"] = sortieActive;
  doc["settings"]["systemSettings"]["puissanceDeChauffe"]["value"] = puissanceDeChauffe;
  doc["settings"]["systemSettings"]["temperatureEauChaude"]["value"] = temperatureEauChaude;
  doc["settings"]["systemSettings"]["puissanceGradateur"]["value"] = puissanceGradateur / 10;
  doc["settings"]["systemSettings"]["etatRelaisStatique"]["value"] = etatRelaisStatique;
  doc["settings"]["systemSettings"]["actif"]["value"] = routeur.actif;
  doc["settings"]["systemSettings"]["version"]["value"] = versionsoft;
  doc["settings"]["systemSettings"]["utilisationSAP"]["value"] = routeur.utilisationSAP;
  doc["settings"]["communicationSettings"]["rssi"] = WiFi.RSSI();

  doc["settings"]["userSettings"]["utilisationPinceAC"]["value"] = routeur.utilisationPinceAC;
  doc["settings"]["userSettings"]["seuilCoupureAC"]["value"] = routeur.seuilCoupureAC;
  doc["settings"]["userSettings"]["coeffMesureAc"]["value"] = routeur.coeffMesureAc;
  doc["settings"]["systemSettings"]["mesureAc"]["value"] = mesureAc;

  doc["settings"]["communicationSettings"]["mqttServer"] = routeur.mqttServer;
  doc["settings"]["communicationSettings"]["mqttPort"] = routeur.mqttPort;
  doc["settings"]["communicationSettings"]["mqttUser"] = routeur.mqttUser;
  doc["settings"]["communicationSettings"]["mqttopic"] = routeur.mqttopic;
  doc["settings"]["communicationSettings"]["mqttopicInput"] = routeur.mqttopicInput;
  doc["settings"]["communicationSettings"]["mqttopicParam1"] = routeur.mqttopicParam1;
  doc["settings"]["communicationSettings"]["mqttopicParam2"] = routeur.mqttopicParam2;
  doc["settings"]["communicationSettings"]["mqttopicParam3"] = routeur.mqttopicParam3;
  doc["settings"]["communicationSettings"]["mqttopicPzem1"] = routeur.mqttopicPzem1;
  doc["settings"]["communicationSettings"]["ssid"] = routeur.ssid;

  doc["settings"]["communicationSettings"]["password"] = routeur.password;
  doc["settings"]["communicationSettings"]["mqttPassword"] = routeur.mqttPassword;

  String result = doc["settings"];
  doc.clear();
  clientWifi.println(result);
}

int RACommunicationClass::hexa(char a)
{
  if ((a >= '0') && (a <= '9'))
    return (int(a - '0'));
  if ((a >= 'A') && (a <= 'F'))
    return (int(a - 'A') + 10);
  return (-1);
}

String RACommunicationClass::convert(String a)
{
  String b = "";
  int pos = a.indexOf('%');
  while (pos != -1)
  {
    int icar = 0;
    b = "%";
    b += a.charAt(pos + 1);
    icar = 16 * hexa(a.charAt(pos + 1));
    b += a.charAt(pos + 2);
    icar += hexa(a.charAt(pos + 2));
    String c = "";
    c += char(icar);
    a.replace(b, c);
    pos = a.indexOf('%');
  }
  return (a);
}

void RACommunicationClass::loop()
{
  bool needReconnect = false;
  int reasonReconnect = -1;
  if (!SAP && routeur.utilisationSAP)
  {
    creationSAP();
    SAP = true;
  }
  if (SAP && !routeur.utilisationSAP)
  {
    // Mode SAP actif mais non voulu. On retente la connexion à intervalle régulier
    needReconnect = true;
    reasonReconnect = 1;
  }
  if (WiFi.status() != WL_CONNECTED && !routeur.utilisationSAP && !needReconnect)
  {
    needReconnect = true;
    reasonReconnect = 2;
  }

#ifdef WifiMqtt
  if (!clientMqtt.connected() && !routeur.utilisationSAP && !needReconnect)
  {
    needReconnect = true;
    reasonReconnect = 3;
  }
#endif

  if (needReconnect)
  {
    if (loopBeforeReconnect == 0)
    {
      if (reasonReconnect == 1)
      {
        RACommunication.print(3, "Mode SAP actif temporaire (non choisi dans le paramétrage). Reconnexion dans ", false);
      }
      else if (reasonReconnect == 2)
      {
        RACommunication.print(3, "Wifi non connecté mais paramétré. Reconnexion dans ", false);
      }
      else
      {
        RACommunication.print(3, "Connexion au broker non réalisé. Reconnexion dans ", false);
      }
      char nbLoopBeforeReconnectMsg[5];
      sprintf(nbLoopBeforeReconnectMsg, "%i", nbLoopBeforeReconnect - loopBeforeReconnect);
      RACommunication.print(3, nbLoopBeforeReconnectMsg, true);
    }
    loopBeforeReconnect++;
    if (loopBeforeReconnect > nbLoopBeforeReconnect)
    {
      // Permet de retenter la connexion

      loopBeforeReconnect = 0;
      RACommunication.reconnect();
    }
  }

#ifdef WifiMqtt

  if (!SAP && WiFi.status() == WL_CONNECTED && clientMqtt.connected())
  {
    // lecture de information sur mqtt output/solar
    //  (1) avec les parametres
    //  (0) seulement les indispensables tension courant ..
    clientMqtt.loop();
    mqtt_publish(paramchange);
    paramchange = 0;
  }
#endif

#ifdef WifiServer
  RACommunication.loopServer();
#endif
}

void RACommunicationClass::loopServer()
{
  if (SAP || WiFi.status() == WL_CONNECTED)
  {
    WiFiClient clientWifi = server.available(); // listen for incoming clients
    if (clientWifi)
    {
      String req = clientWifi.readStringUntil('\r'); // lecture la commande
      RACommunication.print(3, req.c_str(), true);
      String content = "";
      if (req.length() > 0)
      {
        File serverFile;
        if (req.startsWith("POST"))
        {
          boolean currentLineIsBlank = false;
          String requestBody = "{\"";
          while (clientWifi.available())
          {
            char c = clientWifi.read();
            if (c == '\n' && currentLineIsBlank)
            {
              while (clientWifi.available())
              {
                c = clientWifi.read();
                if (c == '=')
                {
                  requestBody += "\":\"";
                }
                else if (c == '&')
                {
                  requestBody += "\",\"";
                }
                else
                {
                  requestBody += c;
                }
              }
            }
            else if (c == '\n')
            {
              currentLineIsBlank = true;
            }
            else if (c != '\r')
            {
              currentLineIsBlank = false;
            }
          }
          requestBody += "\"}";
          requestBody.replace("%2F", "/");
          requestBody = convert(requestBody);
          RACommunication.print(3, requestBody.c_str(), true);
          if (req.startsWith("POST /systemsettings"))
          {
            saveSystemSettings(requestBody);
          }
          else if (req.startsWith("POST /wifisettings"))
          {
            saveWifiSettings(requestBody);
          }
          else if (req.startsWith("POST /mqttsettings"))
          {
            saveMqttSettings(requestBody);
          }
          else if (req.startsWith("POST /summarysettings"))
          {
            saveResumeSettings(requestBody);
          }
          else if (req.startsWith("POST /reboot"))
          {
            restartEsp = true;
          }
          else if (req.startsWith("POST /reset"))
          {
            RAPrgEEprom.reset();
          }
          paramchange = 1;
          clientWifi.println("HTTP/1.1 204 OK");
        }
        else
        {
          RATriac.stop_interrupt();
          pause_inter = 1;

          if (req.startsWith("GET /settings"))
          {
            clientWifi.println("HTTP/1.1 200 OK");
            clientWifi.println("Content-Type: plain/text");
            clientWifi.println();
            getSettings(clientWifi);
          }
          else if (req.startsWith("GET /getNewSettings"))
          {
            clientWifi.println("HTTP/1.1 200 OK");
            clientWifi.println("Content-Type: plain/text");
            clientWifi.println();
            getNewSettings(clientWifi);
          }
          else if (req.indexOf(".css") > 0 || req.indexOf(".js") > 0)
          {
            String tmp = req.substring(4);
            String filePath = tmp.substring(0, tmp.length() - 9);
            serverFile = SPIFFS.open(filePath, "r"); //Ouverture fichier pour le lire
          }
          else
          {
            serverFile = SPIFFS.open("/index.html", "r"); //Ouverture fichier pour le lire
          }
        }

        if (serverFile)
        {
          clientWifi.println("HTTP/1.1 200 OK");
          for (int i = 0; i < serverFile.size(); i++)
          {
            content += (char)serverFile.read();
          }

          if (req.indexOf(".js") > 0)
          {
            clientWifi.println("Content-Type: text/javascript");
          }
          else if (req.indexOf(".css") > 0)
          {
            clientWifi.println("Content-Type: text/css");
          }
          clientWifi.println();
          clientWifi.println(content);
          serverFile.close();
        }
      }
      // close the connection:
      clientWifi.stop();
    }
    if (restartEsp)
    {
      delay(200);
      resetEsp = 1;
    }
    if (pause_inter > 0)
    {
      pause_inter++;
    }
    if ((pause_inter > 3) && (!SAP))
    {
      pause_inter = 0;
      RATriac.restart_interrupt();
    }
    if ((pause_inter > 20) && (SAP))
    {
      pause_inter = 0;
      paramchange = 0;
      RATriac.restart_interrupt();
    }
  }
}

void RACommunicationClass::print(int verboseMinimum, const char *msg, bool sautLigne)
{
  if (VERBOSE >= verboseMinimum)
  {

    if (sautLigne)
    {
      Serial.println(msg);
    }
    else
    {
      Serial.print(msg);
    }
  }
}

RACommunicationClass RACommunication;
