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
  void affiche(int li, String a);
  void affichage_oled();
};

extern RAAfficheurClass RAAfficheur;

#endif // fin de déclaration des fonctions liées à l'écran oled
