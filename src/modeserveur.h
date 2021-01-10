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
    void setup(const char* version);
    void loop();
    void coupure_reseau();

private:
    DynamicJsonDocument readSettingsFile();
    void saveResumeSettings(String jsonResult);
    void saveMqttSettings(String jsonResult);
    void saveWifiSettings(String jsonResult);
    void saveSystemSettings(String jsonResult);
    void getNewSettings(WiFiClient client);
    void getSettings(WiFiClient client);
    int hexa(char a);
    String convert(String a);
};

extern RAServerClass RAServer;
#endif
