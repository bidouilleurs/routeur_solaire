/***************************************/
/******** gestion afficheur   ********/
/***************************************/
#ifndef RA_AFFICHEUR_H
#define RA_AFFICHEUR_H
#include <Arduino.h>
class RAAfficheurClass
{
public:
  void setup();
  void cls();
  void affiche(int li, const char *a);
  void affichage_oled();

private:
  char intBattery[15];
  char tension[15];
  char tempEau[15];
};

extern RAAfficheurClass RAAfficheur;

#endif // fin de déclaration des fonctions liées à l'écran oled
