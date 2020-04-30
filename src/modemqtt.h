/***************************************/
/******** mode MQTT   ********/
/***************************************/
#ifndef RA_MODEMQTT_H
#define RA_MODEMQTT_H
#include <Arduino.h>
class RAMQTTClass
{
public:
  void setup();
  void loop();
  void commande_param(String mesg);

private:
  void mqtt_publish(int a);
  static void callback(char *topic, byte *message, unsigned int length);
  void mqtt_subcribe();
};

extern RAMQTTClass RAMQTT;
#endif // fin de la definition des fonctions mqtt
