#ifndef RA_MODE_SERVEUR_H
#define RA_MODE_SERVEUR_H
/***************************************/
/******** mode Serveur   ********/
/***************************************/
#include <ArduinoJson.h>
#include <WiFiClient.h>
#include <WiFi.h>
class RAServerClass
{

public:
    void setup();
    void loop();

private:
    DynamicJsonDocument readSettingsFile();
    void saveResumeSettings(String jsonResult);
    void saveMqttSettings(String jsonResult);
    void saveWifiSettings(String jsonResult);
    void saveSystemSettings(String jsonResult);
    void getNewSettings(WiFiClient client);
    void getSettings(WiFiClient client);
};

extern RAServerClass RAServer;
#endif