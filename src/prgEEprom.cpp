/***************************************/
/* gestion des sauvegarde sur eeprom   */
/***************************************/
#include "prgEEprom.h"
#include "settings.h"
#include <EEPROM.h>
#include <stdbool.h>
#include "communication.h"
#define EEPROM_SIZE 450

void RAPrgEEpromClass::setup()
{
  EEPROM.begin(EEPROM_SIZE);

  if (EEPROM.read(0) != 123)
  {
    sauve_param(); // action qui se fait au premier démarrage sauve les parametres par défaut
    RACommunication.print(1, "Enregistrement dans l'EEprom et redémarrage", true);
    EEPROM.end(); // pour liberer la memoire
    delay(5000);
  }

  delay(1000);
  RACommunication.print(1, "Chargement de l'EEprom", true);

  restore_param(); // lecture de l'EEprom pour charger les paramètres en cas de coupure de courant

  RACommunication.print(1, "EEprom ok", true);

  delay(100);
  RACommunication.print(1, "", true);
  RACommunication.print(1, "le ssid est  ", false);
  RACommunication.print(1, routeur.ssid, true);
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
  setDefaultValue();
}

void RAPrgEEpromClass::setDefaultValue()
{
  // Initialise les nouveaux champs, lors de mises à jour.
  if (isnan(routeur.seuilCoupureAC))
  {
    routeur.seuilCoupureAC = 0.3;
  }
  if (isnan(routeur.coeffMesureAc))
  {
    routeur.coeffMesureAc = 0.321;
  }
  routeur.utilisationPinceAC = atof(String(routeur.utilisationPinceAC).c_str()) > 1 ? false : routeur.utilisationPinceAC;
  routeur.utilisationSAP = atof(String(routeur.utilisationSAP).c_str()) > 1 ? false : routeur.utilisationSAP;
  SAP = routeur.utilisationSAP;
}

void RAPrgEEpromClass::reset()
{
  EEPROM.writeByte(0, 0); // valeur pour la premiere sauvegarde
  EEPROM.put(1, routeur);
  EEPROM.commit(); // pour liberer la memoire
  paramchange = 1; // communication au format mqtt pour le reseau domotique 1 pour l'envoi des parametres}
  resetEsp = 1;
}
RAPrgEEpromClass RAPrgEEprom;
