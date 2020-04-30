/***************************************/
/* Bibliothèques de commande du triac  */
/***************************************/
//interruption timer
#ifndef RA_TRIAC_H
#define RA_TRIAC_H
#include <Arduino.h>

class RATriacClass
{
public:
  // interruption passage par zéro
  void IRAM_ATTR stop_interrupt();
  void IRAM_ATTR restart_interrupt();
  void start_interrupt();
  void watchdog(int i);

private:
  static void IRAM_ATTR pulseTriac();
  static void IRAM_ATTR zeroCross();
};

extern RATriacClass RATriac;
#endif
