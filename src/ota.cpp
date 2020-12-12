#include "settings.h"
#ifdef OTA
#include "ota.h"
#include <ArduinoOTA.h>
#include "triac.h"
#include "WiFi.h"
#include <SPIFFS.h>
unsigned long otaMillis;

void RAOTAClass::begin()
{
    if (!SAP && WiFi.status() == WL_CONNECTED)
    {
        ArduinoOTA.setPort(8266);
        ArduinoOTA.setHostname("esp-routeur");
        ArduinoOTA.onStart([]() {
            RATriac.stop_interrupt();
            String type;
            if (ArduinoOTA.getCommand() == U_FLASH)
            {
                type = "sketch";
            }
            else
            { // U_SPIFFS
                type = "filesystem";
            }
            Serial.println("Start updating " + type);
            Serial.println("MAJ OTA is coming! ");
            otaMillis = millis();
        });

        ArduinoOTA.onEnd([]() {
            Serial.print("MAJ terminé en : ");
            Serial.print((millis() - otaMillis) / 1000);
            Serial.println(" secondes.");
        });

        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
            Serial.printf("Progression : %u%%\r", (progress / (total / 100)));
        });

        ArduinoOTA.onError([](ota_error_t error) {
            switch (error)
            {
            case OTA_AUTH_ERROR:
                Serial.println("OTA_AUTH_ERROR");
                break;
            case OTA_BEGIN_ERROR:
                Serial.println("OTA_BEGIN_ERROR");
                break;
            case OTA_CONNECT_ERROR:
                Serial.println("OTA_CONNECT_ERROR");
                break;
            case OTA_END_ERROR:
                Serial.println("OTA_END_ERROR");
                break;
            case OTA_RECEIVE_ERROR:
                Serial.println("OTA_RECEIVE_ERROR");
                break;
            default:
                Serial.println("Erreur inconnue");
            }
        });

        ArduinoOTA.begin();
        Serial.print("Adresse IP sur le réseau (info pour l'OTA) : ");
        Serial.println(WiFi.localIP());
    }
}

void RAOTAClass::loop()
{
    if (!SAP && WiFi.status() == WL_CONNECTED)
    {
        ArduinoOTA.handle();
    }
}

RAOTAClass RAOTA;
#endif
