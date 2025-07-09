# Companion for MaxPV!


"Companion" est un écran déporté et autonome pour l'affichage des informations du routeur solaire MaxPV!

L'afficheur utilise un simple et discret LILYGO T-Display S3 qui se connecte à votre réseau local. Il n'est donc pas nécessaire d'avoir une installation domotique pour fonctionner.
L'afficheur est alimenté soit avec un chargeur de téléphone (port USB-C à privilégier), soit une batterie à rajouter.
 De taille réduire, dimensions : 7x3 cm (écran 1,9 pouces), il peut se déplacer facilement.


![Alt text](images/ecran_accueil_1.png)
![Alt text](images/ecran_principal_4.png)

Les données affichées sont les suivantes : 
- production photovoltaiques, 
- énergie routée vers le cumulus, 
- la consommation électrique du domicile, 
- la quantité d'énergie exportée,
- la réserve d'énergie,
- les informations météo locales,
- l'heure et la date locale,
- l'heure du lever et du coucher de soleil,
- synthèse quotidienne.
  
Ainsi que les fonctionnalités suivantes :
- mise en veille programmable et réactivation automatique,
- puissance du réseau wifi,
- fonction serveur permettant de lire les données dans un navigateur.
  
En option :
- température du cumulus, si vous avez une sonde connectée au réseau local, ou une sonde connectée à MaxPV (à partir de la vesion 3.60),
- les jours EDF TEMPO restants et la couleur du jour et du lendemain.

Les données sont réactualisées toutes les 10 secondes. 


## Liens
Forum d'échange sur Companion : https://forum-photovoltaique.fr/viewtopic.php?f=110&t=62730

Repository de `MaxPV!` :  https://github.com/Jetblack31/MaxPV

&nbsp;
&nbsp;

# Installation
## Pré-requis sur la version de MaxPV!
Vérifiez que vous possédez au moins la version 3.60 de MaxPV!, car le Companion est compatible à partir de cette version.

En effet, une modification de l'API de MaxPV! sur la version 3.60 est incompatible avec la précédente version 3.57.
Si vous ne faites pas la mise à jour : l'écran des index journaliers sera inutilisable, ainsi que la température de la sonde de MaxPV.


## Plateforme Arduino IDE

Pour compiler et téléverser le programme "Companion", l'installation de la plateforme nécessite :
- les drivers pour la carte LilyGO S3 (ESP32 S3) à installer via le gestionnaire de cartes,
- les bibliothèques, listées dans les paragraphes ci-dessous, à installer dans le répertoire `Documents\Arduino\librairies` d'ArduinoIDE.

*Remarque : l'ancien manuel d'installation `Installation.pdf` est disponible à l'adresse https://github.com/JJHontebeyrie/Companion/blob/main/Installation.pdf (utiliser le bouton "Download" si le document ne s'affiche pas directement). Mais il s'agit d'une ancienne version des instructions, notamment pour les drivers de la carte. Aujourd'hui la carte LilyGO est reconnue correctement. Le lien est donné à titre d'information.*

### Drivers carte **LilyGO S3**
A installer à partir de la bibliothèque de cartes d'ArduinoIDE.

Sélectionner "**esp32** par Espressif Systems" v2.0.17 par exemple.
L'installation peut prendre quelques minutes.

Attention : ne pas prendre une v3.0.0 ou supérieure, car la compilation échouera avec un message d'erreur du style : `error: 'ledcSetup' was not declared in this scope / error: 'ledcAttachPin' was not declared in this scope;`

![alt text](images/arduinoIde_esp32_S3.png)

Une fois installé, sélectionner la carte LilyGO T-Display-S3 dans le haut d'Arduino IDE :

![alt text](images/arduinoIde_selectionner_carte.png)


### Librairie **TFT_eSPI**

A installer à partir du gestionnaire de bibliothèques d'ArduinoIDE.

Prendre "**TFT_eSPI** par Bodmer v2.5.43"

![alt text](images/arduinoIde_Lib_TFT_eSPI.png)


**IMPORTANT** : sur les dernières versions (> 2.5.0), il faut paramétrer correctement le fichier `Documents\Arduino\librairies\TFT_eSPI\User_Setup_Select.h` pour que la librairie fonctionne avec le LilyGO S3.
Sinon le programme compilera, mais l'écran restera noir!

En effet, il faut réaliser les actions ci-dessous dans le fichier `User_Setup_Select.h` de la librairie : 
- décommenter la ligne `#include <User_Setups/Setup206_LilyGo_T_Display_S3.h>` (situé vers ligne 133),
- commenter la ligne `#include <User_Setup.h>` (situé vers ligne 27).

### Librairie **Time**
A installer à partir du gestionnaire de bibliothèques d'ArduinoIDE.

Prendre "**Time** par Michael Margolis v1.6.1"

![alt text](images/arduinoIde_Lib_Time.png)


### Librairie **TimeZone**
A installer à partir du gestionnaire de bibliothèques d'ArduinoIDE.

Prendre "**Timezone** par Jack Christensen v1.2.5"

![alt text](images/arduinoIde_Lib_Timezone.png)

### Librairie **OpenWheather**
A installer à partir du gestionnaire de bibliothèques d'ArduinoIDE.

Prendre "**OpenWheather** par Bodmer v0.3.0"

![alt text](images/arduinoIde_Lib_OneWheather.png)

### Librairie **OneButton**
A installer à partir du gestionnaire de bibliothèques d'ArduinoIDE.

Prendre "**OneButton** par Matthias Hertel v2.6.1"

![alt text](images/arduinoIde_Lib_OneButton.png)

### Librairie **ArduinoJson**
A installer à partir du gestionnaire de bibliothèques d'ArduinoIDE.

Prendre "**ArduinoJson** par Benoit Blachon v7.4.2"

![alt text](images/arduinoIde_Lib_ArduinoJson.png)

&nbsp;
&nbsp;

## Téléchargement des sources de "**Companion**"
Depuis cette page Github, vous pouvez télécharger les sources de 2 façons selon le cas qui vous intéresse. La première me semble la plus adaptée.

1. Depuis le tag de la version qui vous intéresse : permet de récupérer uniquement les sources de la  version concernée
   
	Cliquer sur le lien "tag" :
	![Alt text](images/github_tags_1.png)

	Puis sur le lien "Zip" de la version qui vous intéresse :
	![Alt text](images/github_tags_2.png)

	Cette action téléchargera un fichier avec le numéro de la version (par exemple : `Companion_for_MaxPV-1.0.zip`) sur votre poste, qui contient un sous-répertoire "Companion" avec les sources.
	Cette méthode est pratique si l'on veut conserver des versions séparées dans le répertoire des croquis d'ArduinoIDE.
	&nbsp;

2. Depuis la page principale : télécharge la dernière version des sources.
Cliquez sur le bouton vert `<> Code`, puis sur `Download ZIP`.

	![Alt text](images/github_main.png)   

	Cette action téléchargera un fichier `Companion_for_MaxPV-main.zip` sur votre poste, qui contient un sous-répertoire "Companion" avec les sources.

## Installation des sources de "**Companion**" dans ArduinoIDE
Décompresser l'archive ZIP de Companion dans le répertoire des croquis d'ArduinoIDE.
Par exemple dans `D:\Documents\Arduino\` vous devez avoir une arborescence de répertoires du type : 
 - `D:\Documents\Arduino\Companion_for_MaxPV-1.0\Companion\`
dans laquelle se trouvent les fichiers `Companion.ino` et les autres fichiers `.h` 
 
Depuis l'Arduino IDE, ouvrir "**Fichier > Carnet de croquis > Companion_for_MaxPV-1.0 > Companion**".

Modifier le fichier `perso.h` avec vos paramètres locaux (réseau WiFi, l'IP de MaxPV sur le réseau, etc...). Chaque paramètre est documenté en commentaire dans le fichier.

## Installation du "Companion" sur le "LilyGO S3"
Branchez la carte `LilyGo T-Display-S3` avec un cable USB à votre PC.

Sélectionnez la carte `LilyGo T-Display-S3` et le port associé. Le nom de la carte apparait alors en gras.

S'assurer que **"Outils > USB CDC On Boot"** est à **"Disabled"**. Car sinon le démarrage de la carte ne sera pas possible en autonome, ni tant que l'écran "Moniteur série" n'est pas ouvert dans ArduinoIDE... Et la carte reste bloquée sur l'écran d'accueil...


Pour information, voici les paramètres sélectionnés dans le menu `Outils` d'ArduinoIDE :

![Alt text](images/arduinoIde_outils.png)

Lancer le téléversement avec le 2ème bouton ci-dessous :

![Alt text](images/arduinoIde_televerser.png)

L'opération de compilation dure environ 1 min car il y a pas mal de bibliothèques à compiler...

L'installation se termine avec les messages `Writing 100%` et `Hard reseting`.

&nbsp;
&nbsp;

# Guide d'utilisation

## Ecran principal

L'écran est une synthèse des données les plus importantes fournies par MaxPV! agrémentées de données météo.

![Alt text](images/ecran_principal_1.png)

Un indicateur dans le coin supérieur gauche vous averti quand les données sont en cours d'acquisition.

Lorsqu'il n'y a pas de production PV, alors les heures de coucher et lever du soleil sont affichées.

Des jauges colorées à droite des valeurs de production PV, du routage de l'eau chaude sanitaire (ECS) ou de la consommation, permettent de visualiser le niveau actuel par rapport au niveau maximum admis. 

Code des couleurs : 
- _**rouge**_ : "pas bon"
- _**vert**_ : "bon"

La consommation réseau passe à `Injection réseau` si de l'énergie est envoyée vers le réseau. La jauge devient alors blanche et évalue la quantité "perdue" par rapport à la quantité produite. Par exemple, si elle est pleine et blanche, alors toute la production est injectée sur le réseau!

**Le bouton rond coloré** à droite "réserve d'énergie" affiche la valeur de l'énergie disponible (en kW) avant de devoir consommer sur le réseau. Il permet de visualiser rapidement si vous pouvez lancer un appareil énergivore sans crainte (aspirateur, chauffage, etc...)

Les couleurs indicatrices sont :
- _**rouge**_ : consommation réseau, pas de réserve d'énergie,
- _**orange**_ : réserve d'énergie faible (moins de 200W),
- _**jaune**_ : réserve d'énergie moyenne (moins de 1000W),
- _**vert**_ : reserve d'énergie importante (jusqu'à 2000W),
- _**bleu turquoise**_ : réserve d'énergie très importante (plus de 2000W).

**L'indicateur wifi** vous prévient de la qualité de la connexion. Si la connexion est mauvaise et que la récupération des données prend trop de temps, l'afficheur **redémarrera automatiquement au bout de 20 secondes**.

![Alt text](images/ecran_principal_2.png)
![Alt text](images/ecran_principal_3.png)
![Alt text](images/ecran_principal_4.png)
![Alt text](images/ecran_principal_5.png)
![Alt text](images/ecran_principal_6.png)

**L'indicateur de batterie** est affiché, sous l'indicateur Wifi, si vous avez activé la prise en charge de la batterie dans le fichier `perso.h`.

Il sera peut-être nécessaire de calibrer les valeurs `BAT_VAL_MAX` (charge 100%) et `BAT_VAL_MIN` (charge 0%) pour votre batterie. Pour vous aider, suivez les instructions dans le fichier `perso.h` pour afficher temporairement la tension de la batterie sur l'écran.

|Indicateur | Description |
|:-----:| :-----|
|![alt text](images/ecran_batterie_1.png)| Batterie en charge|
|![alt text](images/ecran_batterie_2.png) | Batterie chargée|
|![alt text](images/ecran_batterie_3.png) ![alt text](images/ecran_batterie_4.png) | Batterie déchargée|

## Ecran réserve d'énergie
L'écran dédié pour la réserve d'énergie permet d'améliorer la lisibilité de l'afficheur quand on est positionné plus loin. Et que l'on veut lancer des appareils en fonction de l'énergie disponible.

Le même code couleur que précédemment est utilisé. 

En situation de production PV, une couleur supplémentaire "blanche" indique la quantité d'énergie injectée sur le réseau.

![Alt text](images/ecran_reserve_energie_1.png)
![Alt text](images/ecran_reserve_energie_2.png)
![Alt text](images/ecran_reserve_energie_3.png)
![Alt text](images/ecran_reserve_energie_4.png)
![Alt text](images/ecran_reserve_energie_5.png)


## Ecran index journaliers

Affiche les totaux quotidiens des indicateurs principaux de MaxPV. Bien sûr, les valeurs augmentent au cours de la journée, et repassent à zéro au changement de journée. La bascule est gérée par MaxPV!

![Alt text](images/ecran_index_journaliers_1.png)

## Ecran des jours EDF TEMPO restants

Affiche le nombre de jours EDF TEMPO restants par couleur. Ainsi que la couleur du jour actuel et du lendemain.
Un rafraichissemnet est effectué toutes les heures sur le portail edf.fr pour rafraichir les valeurs.

![Alt text](images/ecran_tempo_1.png)

La couleur des jours est également affichée en bas à gauche de l'écran principal. Le 1er carré pour le jour actuel (ici "blanc"), le 2eme carré pour le lendemain (ici "rouge").

![Alt text](images/ecran_principal_7.png)

## Les boutons de commande du boitier

Le **bouton orange supérieur** gauche permet d'ajuster la luminosité. Appuyer plusieurs fois sur ce bouton pour modifier la valeur de la luminosité. La luminosité va s'incrémenter jusqu'à 100%, puis décroitre jusqu'à 20% et ainsi de suite.

L'écran de luminosité est masqué automatiquement lors du rafraichissement des données.

![Alt text](images/ecran_luminosite_1.png)

Le **bouton orange inférieur** gauche permet de changer d'écran.

Remarque : Il faut attendre que le point vert de rafraichissement des données disparaisse pour pouvoir changer d'écran.

Un double clic permet de revenir directement au premier écran, sans faire défiler les suivants.

Le **bouton `Reset`** placé sur le dessus de la carte permet de le redémarrer. Sinon, vous pouvez aussi débrancher/rebrancher l'alimentation.


## Interface Web

Une adresse locale permet également d'accéder à une interface Web : http://companion affichant les données principales.

![Alt text](images/ecran_web.png)

Le bouton orange permet d'afficher les données de la journée.

&nbsp;
&nbsp;

# Crédits

L'afficheur est basé sur les travaux de `Companion MSunPV 2.50` de @jjhontebeyrie (https://github.com/JJHontebeyrie/Companion).

Le code a été adapaté aux spécificités de MaxPV!

&nbsp;
&nbsp;

# Matériel LiLyGO T-Display-S3
Testé avec cette carte ESP32-S3 en USB-C et avec le boitier plastique (Shell Black) : https://fr.aliexpress.com/item/1005004496543314.html?spm=a2g0o.order_list.order_list_main.82.65435e5bYQbfne&gatewayAdapt=glo2fra

(Carte ESP32-S3 avec écran LCD ST7789, résolution 170x320, 1.9 pouces).

Mais d'autres cartes de votre choix avec les mêmes spécifications seront également adaptées.

En avril 2023, le tout était vendu à 22€.
![Alt text](images/lilygo.png)

&nbsp;
&nbsp;

# Versions

## Version 1.3 (08/07/2025)
- Correction problème affichage des jours TEMPO : utilisation de la nouvelle API,
- Ajout couleur jaune sur indicateur température ECS,
- Suppression de la dépendance avec librairie JSON_Decoder pour utiliser seulement la librairie ArduinoJson,
- Mise à jour README d'installation.
  
## Version 1.2 (28/02/2024)
- Ajout écran affichage des jours EDF TEMPO (activation paramétrable),
- Amélioration de la gestion de la batterie,
- Nettoyage de code,
- Aide au bon paramétrage de la librairie TFT_eSPI pour LilyGO S3.


## Version 1.1 (22/02/2024)
- Correction index journaliers suite à modifications de l'API dans MaxPV 3.60,
- Prise en compte de la température fournie par MaxPV 3.60 (avec possibilté de garder la température via une autre sonde comme auparavant)


## Version 1.0 (27/10/2023)
- Ajout double clic sur bouton droit pour revenir à l'écran principal,
- Amélioration lecture balise de fin pour les données du serveur de température,
- Affichage de la température de l'ECS limité à une décimale,
- Inversion position de la température extérieure et du taux d'humidité.


## Version 0.10 (10/05/2023)
- Correction de l'affichage du niveau de batterie
- Le voyant représente une réserve d'énergie :
	- si importation, alors rouge
	- si réserve jusqu'à 200w, alors orange
	- si réserve jusqu'à 1000w, alors jaune
	- si réserve jusqu'à 2000w, alors vert
	- si plus, alors turquoise
	- en fonction de chaque statut, si exportation d'énergie alors un logo supplémentaire apparait sur le voyant
- Affichage de la valeur de la réserve sur le voyant en kW
- Modification du mode de défilement des écrans par l'appui du bouton bas (Suppression du retour automatique vers l'écran principal)
- Ajout écran "Réserve d'énergie" / "Consommation" : l'objectif est de pouvoir visualiser de loin l'énergie disponible, ou la consommation actuelle
- Augmentation du timeout avant reboot à 20s

## Version 0.9 (03/05/2023)
- Basée sur la version 2.50 de Companion "MSunPV" https://github.com/JJHontebeyrie/Companion

- Adaptation pour la lecture des données de puissance de MaxPV et de quelques paramètres
- Ajout possibilité d'accéder à la température de l'ECS via une API autre que MaxPV (testé avec Jeedom et Shelly 1P)
- Refonte écran de démarrage pour MaxPV!
	- Nouveau logo
	- Indications de connexion avec compte à rebours avant redémarrage auto
- Mise à jour écran principal :
	- Réactualisation des icônes
	- Ajout taux d'humidité
	- Amélioration lisibilité qualité du signal WiFi
	- Suppression de l'indicateur de luminosité
	- Ajout indicateur de rafraichissement des données (en haut à gauche)
	- Amélioration finesse des jauges
	- Jauge d'injection : affichage du taux injecté par rapport à la production courante
- Ecran en surimpression, dédié à la gestion de la luminosité
- Refonte indicateur des cumuls journaliers
	- Ajout d'information techniques (IP + force signal + version)
	- Ajout séparateur de milliers pour meilleure lisibilité

