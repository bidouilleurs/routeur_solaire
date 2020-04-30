/***************************************/
/******** mode bluetooth   ********/
/***************************************/
#ifndef RA_BLUETOOTH_H
#define RA_BLUETOOTH_H
class RABluetoothClass
{
public:
  void setup();
  void BT_commande();
  void BT_visu_param();
  void read_bluetooth();
};

extern RABluetoothClass RABluetooth;
#endif