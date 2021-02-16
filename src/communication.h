#ifndef RA_COMMUNICATION_H
#define RA_COMMUNICATION_H

#include <Arduino.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>

class RACommunicationClass
{
public:
    void setup(const char *version_soft);
    void loop();
    void print(int verboseMinimum, const char *message, bool sautLigne);

private:
    void connexion();
    void mqtt_subscribe();
    static void callback(char *topic, byte *message, unsigned int length);
    void commande_param(char *mesg);
    void reconnect();
    void mqtt_publish(int a);
    DynamicJsonDocument readSettingsFile();
    void saveResumeSettings(String jsonResult);
    void saveMqttSettings(String jsonResult);
    void saveWifiSettings(String jsonResult);
    void saveSystemSettings(String jsonResult);
    void getNewSettings(WiFiClient client);
    void getSettings(WiFiClient client);
    int hexa(char a);
    String convert(String a);
    void loopServer();
    void creationSAP();
    const char *versionsoft;
    int affpub = 0;
    int testconnectMQTT = 0;          //  permet de savoir si la connection MQTT est faite
    int testconnectWIFI = 0;          //  permet de savoir si la connection WIFI est faite
    int maxTentativeConnectMQTT = 3;  // Limite de tentative de connexion au MQTT
    int maxTentativeConnectWifi = 20; // Limite de tentative de connexion au Wifi
    int pause_inter = 0;
    bool restartEsp = false;
    const char *version;
    int loopBeforeReconnect = 0;
    int nbLoopBeforeReconnect = 500;
    bool SAPActif = false;
    char nbLoopBeforeReconnectMsg[5];
    char buffer[200];
};

extern RACommunicationClass RACommunication;

#endif