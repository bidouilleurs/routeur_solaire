/***************************************/
/* gestion des sauvegarde sur eeprom   */
/***************************************/
#include "prgEEprom.h"
#include "settings.h"
#include <EEPROM.h>
#define EEPROM_SIZE 450

void RAPrgEEpromClass::setup()
{
  EEPROM.begin(EEPROM_SIZE);

  if (EEPROM.read(0) != 123)
  {
    sauve_param(); // action qui se fait au premier démarrage sauve les parametres par défaut
    Serial.println("Enregistrement dans l'EEprom et redémarrage");
    EEPROM.end(); // pour liberer la memoire
    delay(5000);
  }

  delay(1000);
  Serial.println("Chargement de l'EEprom");

  restore_param(); // lecture de l'EEprom pour charger les paramètres en cas de coupure de courant

  Serial.println("EEprom ok");
  delay(100);
  Serial.println();
  Serial.print(F("le ssid est  "));
  Serial.println(routeur.ssid); // verification de la lecture
}

void RAPrgEEpromClass::sauve_param()
{
  EEPROM.writeByte(0, 123); // valeur pour la premiere sauvegarde
  EEPROM.put(1, routeur);
  EEPROM.commit(); // pour liberer la memoire
  paramchange = 1; // communication au format mqtt pour le reseau domotique 1 pour l'envoi des parametres
}

void RAPrgEEpromClass::close_param()
{
  EEPROM.end(); // pour liberer la memoire
}

void RAPrgEEpromClass::restore_param()
{
  EEPROM.get(1, routeur);
}

RAPrgEEpromClass RAPrgEEprom;
