/********************************************/
/******** Mesure du courant  tension  ********/
/********************************************/
#ifndef RA_MESURE_H
#define RA_MESURE_H

class RAMesureClass
{
public:
  void setup();
  void mesurePinceTension(int jmax, int imax);
  void mesureTemperature();
  void mesure_puissance();
};
extern RAMesureClass RAMesure;
#endif
