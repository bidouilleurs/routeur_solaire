/***************************************/
/******** mode Serveur   ********/
/***************************************/
#include "settings.h"
#ifdef WifiServer
#include "modeserveur.h"
#include "triac.h"
#include "regulation.h"
#include "prgEEprom.h"
#include "afficheur.h"
#include <string.h>

#include <SPIFFS.h>

#ifdef WifiSAPServeur
bool wifiSAP = true;
#else
bool wifiSAP = false;
#endif

int pause_inter = 0;
WiFiServer server(80);
IPAddress local_ip(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);
bool restartEsp = false;

DynamicJsonDocument RAServerClass::readSettingsFile()
{
    File fileSettings = SPIFFS.open("/settings/settings.json", FILE_READ);

    if (!fileSettings)
    {
        Serial.println("Erreur lors de la lecture du fichier contenant les paramètres");
    }
    std::string settings = "";
    while (fileSettings.available())
    {
        settings += fileSettings.read();
    }
    DynamicJsonDocument doc(3500);
    deserializeJson(doc, settings);
    fileSettings.close();
    return doc;
}

void RAServerClass::setup()
{

#ifndef WifiMqtt
    int testconnect = 0;
    if (!SAP)
    {
        WiFi.begin(routeur.ssid, routeur.password); // connection au reseau 20 tentatives autorisées
        while ((testconnect < 20) && (WiFi.status() != WL_CONNECTED))
        {
            delay(500);
            Serial.println(F("Connection au WiFi.."));
            testconnect++;
        }
        if (testconnect >= 20)
            SAP = true;
        else
        {
            Serial.println("\n");
            Serial.println("Connexion etablie!");
            Serial.print("Adresse IP: ");
            Serial.println(WiFi.localIP());""
        }
    }
#endif
    if (SAP)
    {
        const char *ssid = "routeur_esp32"; // mode point d'accès
        const char *password = "adminesp32";
        WiFi.enableAP(true);
        delay(100);
        WiFi.softAP(ssid, password); // l'esp devient serveur et se place à l'adresse 192.168.4.1
        WiFi.softAPConfig(local_ip, gateway, subnet);
        IPAddress myIP = WiFi.softAPIP();
        Serial.print("AP IP address: ");
        Serial.println(myIP);
    }

    if (!SPIFFS.begin())
    {
        Serial.println("Erreur SPIFFS...");
        return;
    }
    server.begin();
}
void RAServerClass::saveResumeSettings(String jsonResult)
{
    RATriac.stop_interrupt();
    StaticJsonDocument<1000> doc;
    deserializeJson(doc, jsonResult);
    marcheForcee = doc["marcheForcee"] == "true";
    marcheForceePercentage = atof(doc["marcheForceePercentage"]);
    temporisation = atof(doc["temporisation"]);
    Serial.print("marche forcee ");
    Serial.println(temporisation);
    doc.clear();
    RAPrgEEprom.sauve_param();
    pause_inter = 1;
}

void RAServerClass::saveMqttSettings(String jsonResult)
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

void RAServerClass::saveWifiSettings(String jsonResult)
{
    RATriac.stop_interrupt();
    StaticJsonDocument<1000> doc;
    deserializeJson(doc, jsonResult);
    strcpy(routeur.ssid, doc["ssid"]);
    strcpy(routeur.password, doc["password"]);
    doc.clear();
}

void RAServerClass::saveSystemSettings(String jsonResult)
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
    routeur.actif = doc["actif"] == "true";
    if(!routeur.actif){
        RARegulation.desactivation();
    }
    restartEsp = doc["needRestart"] == "true" ? 1 : 0;
    doc.clear();
    RAPrgEEprom.sauve_param();
    pause_inter = 1;
}

void RAServerClass::getNewSettings(WiFiClient client)
{
    DynamicJsonDocument doc(500);
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
    String result = doc["settings"];
    client.println(result);
}

void RAServerClass::getSettings(WiFiClient client)
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
    client.println(result);
}

int RAServerClass::hexa(char a)
{
    if ((a >= '0') && (a <= '9'))
        return (int(a - '0'));
    if ((a >= 'A') && (a <= 'F'))
        return (int(a - 'A') + 10);
    return (-1);
}

String RAServerClass::convert(String a)
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

void RAServerClass::loop()
{
    if ((!SAP) && (WiFi.status() != WL_CONNECTED))
    {
        resetEsp = 1;
        return;
    }
    WiFiClient client = server.available(); // listen for incoming clients
    if (client)
    {
        String req = client.readStringUntil('\r'); // lecture la commande
        Serial.println(req);
        String content = "";
        if (req.length() > 0)
        {
            File serverFile;
            if (req.startsWith("POST"))
            {
                boolean currentLineIsBlank = false;
                String requestBody = "{\"";
                while (client.available())
                {
                    char c = client.read();
                    if (c == '\n' && currentLineIsBlank)
                    {
                        while (client.available())
                        {
                            c = client.read();
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
                Serial.println(requestBody);
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
                client.println("HTTP/1.1 204 OK");
            }
            else
            {
                RATriac.stop_interrupt();
                pause_inter = 1;

                if (req.startsWith("GET /settings"))
                {
                    client.println("HTTP/1.1 200 OK");
                    client.println("Content-Type: plain/text");
                    client.println();
                    getSettings(client);
                }
                else if (req.startsWith("GET /getNewSettings"))
                {
                    client.println("HTTP/1.1 200 OK");
                    client.println("Content-Type: plain/text");
                    client.println();
                    getNewSettings(client);
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
                client.println("HTTP/1.1 200 OK");
                for (int i = 0; i < serverFile.size(); i++)
                {
                    content += (char)serverFile.read();
                }

                if (req.indexOf(".js") > 0)
                {
                    client.println("Content-Type: text/javascript");
                }
                else if (req.indexOf(".css") > 0)
                {
                    client.println("Content-Type: text/css");
                }
                client.println();
                client.println(content);
                serverFile.close();
            }
        }
        // close the connection:
        client.stop();
    }
    if (restartEsp)
    {
        delay(200);
        resetEsp = 1;
    }
    if (pause_inter > 0)
    {
        pause_inter++;
        Serial.print("Serveur en cours ");
        Serial.println(pause_inter);
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

int resloop = 0;
void RAServerClass::coupure_reseau()
{
    if ((!wifiSAP) && (SAP))
    {
        resloop++;
        if (resloop > 40)
        {
            Serial.println(F("Attente de reseau "));
            WiFi.disconnect();
            int n = WiFi.scanNetworks();
            Serial.println("scan done");
            if (n == 0)
            {
                Serial.println("no networks found");
            }
            else
            {
                Serial.print(n);
                Serial.println(" networks found");
                for (int i = 0; i < n; ++i)
                {
                    if (WiFi.SSID(i) == routeur.ssid)
                        resetEsp = 1;
                    delay(10);
                }
            }
            resloop = 0;
#ifdef EcranOled
            RAAfficheur.cls();
            RAAfficheur.affiche(20, "WIFI");
            RAAfficheur.affiche(35, "Deconnecte");
#endif
        }
    }
}

RAServerClass RAServer;
#endif
