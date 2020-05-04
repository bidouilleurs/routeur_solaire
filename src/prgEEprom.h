/***************************************/
/* gestion des sauvegarde sur eeprom   */
/***************************************/
#ifndef RA_EEprom_H
#define RA_EEprom_H

class RAPrgEEpromClass
{
public:
  void setup();
  void sauve_param();
  void restore_param();
  void close_param();
};
extern RAPrgEEpromClass RAPrgEEprom;

#endif // fin des declarations des fonctions liées à l'EEprom
