# Réseautonome - Routeur offgrid

## Sommaire
* [Parametrage](#Parametrage)
  * [Parametrage pour l algorithme](#Parametrage-pour-l-algorithme)
  * [Fonctionnalites optionnelles du systeme](#Fonctionnalites-optionnelles-du-systeme)
  * [Parametrage le fonctionnement du serveur web](#Parametrage-le-fonctionnement-du-serveur-web)
  * [Parametrage pour le fonctionnement MQTT](#Parametrage-pour-le-fonctionnement-MQTT)
* [Flash sur ESP](#Flash-sur-ESP)
  * [PlatformIO filaire](#Via-platformIO-filaire)
  * [PlatformIO OTA](#Via-platformIO-OTA)
* [Developpement](#Developpement)


## Paramétrage
### Paramétrage pour l'algorithme
| Champs      |    Fichier | default |  description |
| ------------- | :---------: |---------: |---------:|
| zeropince  |   settings.h    | -100 | valeur mesurer à zéro |
| coeffPince  |   settings.h   |  0.0294 | Calculer le coefficient |
| coeffTension  |   settings.h    | 0.0177533 | diviseur de tension |
| seuilDemarrageBatterie  |   settings.h   | 56 | seuil de mise en marche de la regulation dans le ballon |
| toleranceNegative  |   settings.h    | 0.5 | autorisation de 300mA négative au moment de la charge complète |
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

### Fonctionnalités optionnelles du système
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
| WifiServer      |     Met à disposition une page web pour visualiser les données du systèmes et modifier les valeurs de l'algorithme |
| EcranOled      |     Affichage de certaines infos sur un afficheur Oled |
| Simulation      |     Simulation de données pour des tests, dans le cas ou le système n'est pas branché |
| OTA      |    Permet la mise à jour par OTA (over the air) |

### Serveur web
Le serveur web permet de faire les régalges depuis une page web. Pour cela, Deux possibités son disponible :
* connecter l'esp au wifi domestic pour y accéder depuis un navigateur web d'un appareil connecté sur le réseau
* Mettre l'ESP en mode point d'accès (SAP) et connecter son smartphone sur le wifi de l'esp (routeur_esp32)

Dans le cas ou la connexion wifi échoue, l'ESP passe automatiquement en mode point d'accès (SAP)

| Champs      |    Fichier | default |  description |
| ------------- | :---------: |---------: |---------:|
| ssid  |   settings.h    | "" |Identifiant "ssid de la  box internet |
| password  |   settings.h   | "" | Mot de passe de la box internet  |
| SAP  |   src.ino   | false | force l'ESP en point d'accès (en dehors du réseau domestique -> ssid: "routeur_esp32"; password : "adminesp32" |

### Paramétrage pour le fonctionnement MQTT

Le MQTT permet de recevoir les infos de l'ESP sur un broker MQTT, et de piloter le routeur solaire.
Une connexion au wifi est obligatoire (cf. Serveur web)

| Champs      |    Fichier |  default |  description |
| ------------- | :---------: |---------:|---------:|
| mqttServer  |   settings.h    | "192.168.1.28" | ip du serveur mqtt |
| mqttPort  |   settings.h    |  1883 | port du serveur mqtt |
| mqttUser  |   settings.h    |  "" | utilisateur mqtt si nécessaire |
| mqttPassword  |   settings.h    |  "" | mot "de passe mqtt si nécessaire |
| mqttopic  |   settings.h    | "sensor/solar"  | Récupération Intensite-Tension-Gradateur-Temperature-puissanceMono |
| mqttopicInput  |   settings.h    |  "output/solar" | *Pilotage à distance, voir le chapitre de pilotage* |
| mqttopicParam1  |   settings.h    |  "param/solar1" | Récupération zeropince-coeffPince-coeffTension-seuilTension-toleranceNeg-actif|
| mqttopicParam2  |   settings.h    |   "param/solar2" |Récupération sortie2-sortie2_tempHaut-sortie2_tempBas-temporisation-sortieActive |
| mqttopicParam3  |   settings.h    | "param/solar3"  | Récupération sortieRelaisTemp-sortieRelaisTens-relaisMax-relaisMin-Forcage_1h|
| mqttopicPzem1  |   settings.h    |  "sensor/Pzem1" | Récupération pzem infos : Intensite-Tension-Puissance-Energie-Cosf|

Pour piloter l'ESP à distance, il est possible d'envoyer des infos via mqt, sur le topic **mqttopicInput**
| message      |    description |
| ------------- | ---------:|
| rst  |  reboot l'esp
| batX  |  Changement du seuil de démarrage batterie par la valeur X (X est un nombre, ex: bat51)
| negX  |  Changement de la tolérance négative par la valeur X (X est un nombre, ex: neg0.6)
| sthX  |  Changement du seuil de température haute par la valeur X (X est un nombre, ex: stb51)
| stbX  |  Changement du seuil de température basse par la valeur X (X est un nombre, ex: stb51)
| rthX  |  Changement du relais tension haut par la valeur X (X est un nombre, ex: stb51)
| rtbX  |  Changement du relais tension bas par la valeur X (X est un nombre, ex: stb51)
| sorX  |  Changement de la réception de sortie pour la commande du 2eme gradateur (X=0 ou X=1)
| cmfX  |  Changement de la marche forcée - 1 pour l'activer, 0 pour l'arreter (X=0 ou X=1)
| rmfX  |  Changement du ratio de la marche force, en % (ex : rfm25)
| tmpX  |  Changement du temps de la marche forcée, en minutes (ex: tmp60)
| relX  |  Changement du relais statique X=0 pour désactiver le relais, X=1 pour activer en mode températuren, X=2 pour activer en mode tension

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


#### Serveur web dans le spiff
Via l'onglet Plateformio qui est disponible sur la gauche de vscodium (la barre latérale, l'icone qui ressemble à une fourmi) :  
Cliquer sur "Upload File System image"

#### Application  
Depuis la barre d'action rapide en base de VSCodium : cliquer sur la fleche "PlatformIO: Upload"

#### Monitor
Depuis la barre d'action rapide en base de VSCodium : cliquer sur la fleche "PlatformIO: Serial Monitor"
Ceci permet de voir les infos et logs de l'ESP (Ip du serveur, mesures prises, publication mqtt ....)

### PlatformIO OTA
**/!\\ Une première installation filaire est nécessaire !**
La mise à jour à distance nécessite que l'ESP soit connecté au réseau.

Dans le fichier plateformio.ini présent à la racine du dossier, il faut renseigner l'adresse IP de l'esp :
```
upload_port=192.168.1.57
```
L'adresse IP de l'ESP est affichée dans les logs, au démarrage de l'ESP (CF section Monitor)
![Alt text](readme/ip_ota.PNG?raw=true "Ota Select")


Cliquer sur **Default** puis choisissez **env:routeur_solaire_OTA**
![Alt text](readme/ota_select.PNG?raw=true "Ota Select")

Ensuite flasher comme indiqué dans la section précedente

## Développement
Pour changer des fonctionnalités, vous pouvez éditer le code.
Si vous souhaitez éditer le code du serveur web, il faut modifier les fichiers présents dans le dossier "dataCode" puis, une fois les changements effectués, minifier le code dans le dossier "data".
Outils en ligne pour minimifier le code : 
html / css / js : http://minifycode.com/javascript-minifier/
json : https://www.webtoolkitonline.com/json-minifier.html