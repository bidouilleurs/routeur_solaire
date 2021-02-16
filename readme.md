# Réseautonome - Routeur offgrid

## Sommaire
* [Tutoriels videos](#Tutoriels-videos)
* [Parametrage](#Parametrage)
  * [Parametrage pour l algorithme](#Parametrage-pour-l-algorithme)
  * [Fonctionnalites optionnelles du systeme](#Fonctionnalites-optionnelles-du-systeme)
  * [Parametrage le fonctionnement du serveur web](#Parametrage-le-fonctionnement-du-serveur-web)
  * [Parametrage pour le fonctionnement MQTT](#Parametrage-pour-le-fonctionnement-MQTT)
* [Flash sur ESP](#Flash-sur-ESP)
  * [PlatformIO filaire](#PlatformIO-filaire)
  * [PlatformIO OTA](#PlatformIO-OTA)
* [Developpement](#Developpement)


## Tutoriels videos

Fonctionnement

[![Alt text](https://img.youtube.com/vi/k4qtnl1ytJM/0.jpg)](https://www.youtube.com/watch?v=k4qtnl1ytJM)

Présentation du prototype

[![Alt text](https://img.youtube.com/vi/r0o6uWTO2Bw/0.jpg)](https://www.youtube.com/watch?v=r0o6uWTO2Bw)

Démonstration

[![Alt text](https://img.youtube.com/vi/PUMbBfnpi2k/0.jpg)](https://www.youtube.com/watch?v=PUMbBfnpi2k)

Envoyer le programme sur l'esp

[![Alt text](https://img.youtube.com/vi/kEmMACQ/0.jpg)](https://www.youtube.com/watch?v=kEmMACQ)

Présentation du serveur web

[![Alt text](https://img.youtube.com/vi/cQqW8xrT0VU/0.jpg)](https://www.youtube.com/watch?v=cQqW8xrT0VU)

Explication Hardware

[![Alt text](https://img.youtube.com/vi/aPKQgpmWH7U/0.jpg)](https://www.youtube.com/watch?v=aPKQgpmWH7U)

Assemblage partiel Hardware

[![Alt text](https://img.youtube.com/vi/y1rwfJ6BLlo/0.jpg)](https://www.youtube.com/watch?v=y1rwfJ6BLlo)

Explication de corrections mineures

[![Alt text](https://img.youtube.com/vi/yLEv1khMI3Y/0.jpg)](https://www.youtube.com/watch?v=yLEv1khMI3Y)

## Parametrage
### Parametrage pour l algorithme
| Champs      |    Fichier | default |  description |
| ------------- | :---------: |---------: |---------:|
| zeropince  |   settings.h    | -100 | valeur mesurer à zéro |
| coeffPince  |   settings.h   |  0.0294 | Calculer le coefficient |
| coeffTension  |   settings.h    | 0.0177533 | diviseur de tension |
| seuilDemarrageBatterie  |   settings.h   | 56 | seuil de mise en marche de la regulation dans le ballon |
| toleranceNegative  |   settings.h    | 0.5 | autorisation de 500mA négative au moment de la charge complète |
| utilisation2Sorties  |   settings.h   | false | validation de la sortie 2eme gradateur |
| temperatureBasculementSortie2  |   settings.h    | 60 | température pour démarrer la regul sur la sortie 2 |
| temperatureRetourSortie1  |   settings.h   | 45 |température pour rebasculer sur le premier gradateur  |
| relaisStatique  |   settings.h    | false | Indique si un relais statique est utilisé |
| seuilMarche  |   settings.h   | 50 | température ou tension de déclenchement du relais |
| seuilArret  |   settings.h    | 45 | température ou tension d'arret du relais |
| tensionOuTemperature  |   settings.h   | "V" | Indique si le seuil est en "V"olts ou en "D"egrés|
| correctionTemperature  |   settings.h    | -2.3 | |
| basculementMode  |   settings.h   | "T" |  Choix du mode de basculement : T->température, P-> Puissance zero|
| actif  |   settings.h   | true | permet d'activer ou désactiver le système
| utilisationPinceAC  |   settings.h   | true | Utilisation d'une Pince pour la mesure AC
| seuilCoupureAC  |   settings.h   | 0.3 | Seuil de coupure pour la pince AC
| coeffMesureAc  |   settings.h   | 0.321 | Coeff de mesure de la pince AC

### Fonctionnalites optionnelles du systeme
Les champs décrits ci-dessous permettent d'activer/désactiver certaines options
Ils sont présent dans le fichier settings.h. Si l'option ne vous interesse pas, il faut mettre la ligne en commentaire (commencer la ligne par '//')
Pour activer l'option :
```
#define  EcranOled 
```
Pour **ne pas** activer l'option
```
//#define  EcranOled 
```

| Champs      |     Description |
| ------------- | ---------: |
| parametrage      |      A **n**'utiliser **que** pour faire les essais sans les accessoires  |
| MesureTemperature      |       Permet la lecture de la sonde de dallas température  |
| Pzem04t      |       utilise un pzem004 pour la mesure de puissance dans le ballon  |
| Sortie2      |       Indique la connexion d'un deuxième Triac  |
| WifiMqtt      |       Permet la connexion Mqtt sur un serveur : récupération et envoie de données pour de la supervision  |
| Ecran_inverse | Permet d'inverser l'écran
| WifiServer      |     Met à disposition une page web pour visualiser les données du systèmes et modifier les valeurs de l'algorithme |
| EcranOled      |     Affichage de certaines infos sur un afficheur Oled |
| Simulation      |     Simulation de données pour des tests, dans le cas ou le système n'est pas branché |
| OTA      |    Permet la mise à jour par OTA (over the air) |


### Parametrage le fonctionnement du serveur web
Le serveur web permet de faire les régalges depuis une page web. Pour cela, Deux possibités son disponible :
* connecter l'esp au wifi domestic pour y accéder depuis un navigateur web d'un appareil connecté sur le réseau
* Mettre l'ESP en mode point d'accès (SAP) et connecter son smartphone sur le wifi de l'esp (routeur_esp32)

Dans le cas ou la connexion wifi échoue, l'ESP passe automatiquement en mode point d'accès (SAP)

| Champs      |    Fichier | default |  description |
| ------------- | :---------: |---------: |---------:|
| ssid  |   settings.h    | "" |Identifiant "ssid de la  box internet |
| password  |   settings.h   | "" | Mot de passe de la box internet  |
| SAP  |   src.ino   | false | force l'ESP en point d'accès (en dehors du réseau domestique -> ssid: "routeur_esp32"; password : "adminesp32" |

### Parametrage pour le fonctionnement MQTT

Le MQTT permet de recevoir les infos de l'ESP sur un broker MQTT, et de piloter le routeur solaire.
Une connexion au wifi est obligatoire (cf. Serveur web)

| Champs      |    Fichier |  default |  description |
| ------------- | :---------: |---------:|---------:|
| mqttServer  |   settings.h    | "192.168.1.28" | ip du serveur mqtt |
| mqttPort  |   settings.h    |  1883 | port du serveur mqtt |
| mqttUser  |   settings.h    |  "" | utilisateur mqtt si nécessaire |
| mqttPassword  |   settings.h    |  "" | mot "de passe mqtt si nécessaire |
| mqttopic  |   settings.h    | "sensor/solar"  | Récupération Intensite-Tension-Gradateur-Temperature-puissanceMono-rssi |
| mqttopicInput  |   settings.h    |  "output/solar" | *Pilotage à distance, voir le chapitre de pilotage* |
| mqttopicParam1  |   settings.h    |  "param/solar1" | Récupération zeropince-coeffPince-coeffTension-seuilTension-toleranceNeg-actif|
| mqttopicParam2  |   settings.h    |   "param/solar2" |Récupération sortie2-sortie2_tempHaut-sortie2_tempBas-temporisation-sortieActive |
| mqttopicParam3  |   settings.h    | "param/solar3"  | Récupération sortieRelaisTemp-sortieRelaisTens-relaisMax-relaisMin-Forcage_1h-version-seuilCoupureAC-coeffMesureAc|
| mqttopicPzem1  |   settings.h    |  "sensor/Pzem1" | Récupération pzem infos : Intensite-Tension-Puissance-Energie-Cosf|

Pour piloter l'ESP à distance, il est possible d'envoyer des infos via mqt, sur le topic défini via la variable **mqttopicInput** (par défaut = output/solar)
| message      |    description |
| ------------- | ---------:|
| rst  |  reboot l'esp
| batX  |  Changement du seuil de démarrage batterie par la valeur X (X est un nombre, ex: bat51)
| negX  |  Changement de la tolérance négative par la valeur X (X est un nombre, ex: neg0.6)
| sthX  |  Changement du seuil de température haute par la valeur X (X est un nombre, ex: sth51)
| stbX  |  Changement du seuil de température basse par la valeur X (X est un nombre, ex: stb51)
| rthX  |  Changement du relais tension haut par la valeur X (X est un nombre, ex: rth51)
| rtbX  |  Changement du relais tension bas par la valeur X (X est un nombre, ex: rtb51)
| sorX  |  Changement de la réception de sortie pour la commande du 2eme gradateur (X=0 ou X=1)
| cmfX  |  Changement de la marche forcée - 1 pour l'activer, 0 pour l'arreter (X=0 ou X=1)
| rmfX  |  Changement du ratio de la marche force, en % (ex : rfm25)
| tmpX  |  Changement du temps de la marche forcée, en minutes (ex: tmp60)
| relX  |  Changement du relais statique X=0 pour désactiver le relais, X=1 pour activer en mode températuren, X=2 pour activer en mode tension
| tmpX  |  Temps de la marche forcée. X minutes.
| temX  |  indique que le ballon est a X degrés. Uniquement dans le cas ou la propriété MesureTemperature n'est pas activée dans le settings

Pour activer / désactiver le routeur complétement, il faut envoyer un message sur le topic **router/activation**
* 1 pour activer
* 0 pour désactiver

## Flash sur ESP

### PlatformIO filaire
#### Installation VSCodium
https://github.com/VSCodium/vscodium/releases

#### Plugin PlatformIO
Depuis vscodium : 
file -> préference -> extension, rechercher et installer platformIO IDE


#### Application
Choisissez l'environnement **env:routeur_solaire**
![Alt text](readme/ota_select.PNG?raw=true "Ota Select")

Selectionner l'onglet plateformio dans le menu latérial (la tete de fourmi), puis déplier les options de l'environnement **env:routeur_solaire**
![Alt text](readme/env_select.PNG?raw=true "Env Select")
![Alt text](readme/filaire.PNG?raw=true "depliage env")

L'option **Build** compile le projet
L'option **upload** permet de flasher l'ESP avec le code compiler
L'option **Monitor** permet de visualiser les logs de l'ESP (infos IP, infos des mesures, ...)
L'option **upload Filesystem Image** permet d'envoyer les fichiers du serveur web


### PlatformIO OTA
**/!\\ Une première installation filaire est nécessaire !**
La mise à jour à distance nécessite que l'ESP soit connecté au réseau.

Dans le fichier plateformio.ini présent à la racine du dossier, il faut renseigner l'adresse IP de l'esp :
```
upload_port=192.168.1.57
```
L'adresse IP de l'ESP est affichée dans les logs, au démarrage de l'ESP (CF section Monitor)
![Alt text](readme/ip_ota.PNG?raw=true "IP OTA")


Cliquer sur **Default** puis choisissez **env:routeur_solaire_OTA**
![Alt text](readme/ota_select.PNG?raw=true "Ota Select")

Selectionner l'onglet plateformio dans le menu latérial (la tete de fourmi), puis déplier les options de l'environnement **env:routeur_solaire_OTA**
![Alt text](readme/env_select.PNG?raw=true "Env Select")
![Alt text](readme/ota.PNG?raw=true "depliage env")

Seul les options Build, upload et upload filesystem image sont disponibles
L'option **Build** compile le projet
L'option **upload** permet de flasher l'ESP avec le code compiler via OTA
L'option **upload Filesystem Image** permet d'envoyer les fichiers du serveur web via OTA

## Developpement
Pour changer des fonctionnalités, vous pouvez éditer le code.
Si vous souhaitez éditer le code du serveur web, il faut modifier les fichiers présents dans le dossier "dataCode" puis, une fois les changements effectués, minifier le code dans le dossier "data".
Outils en ligne pour minimifier le code : 
html / css / js : http://minifycode.com/javascript-minifier/
json : https://www.webtoolkitonline.com/json-minifier.html