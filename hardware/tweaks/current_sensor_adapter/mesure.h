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
  float mesurePinceAC(int pinPince, float coeff , bool avecsigne);
   void mesureTemperature();
  void mesure_puissance();
};
extern RAMesureClass RAMesure;
#endif
