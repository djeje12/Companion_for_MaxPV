/******************************************************************************
**************************  Données personnelles  *****************************
***    Mettez ici toutes vos valeurs personnelles pour le fonctionnement    ***
***             correct de l'afficheur externe Companion                    ***
******************************************************************************/

// Vos codes accès au wifi. Remplacez les * par vos valeurs
const char* ssid     = "********";
const char* password = "******************";

// Adresse IP du serveur MaxPV!.
// La valeur par défaut lors de l'installation de MaxPV! est : 192.168.1.250
// Adapter la valeur en fonction de votre configuration réseau locale.
// Format : NNN.NNN.NNN.NNN
char serveurMaxPV[] = "192.168.***.***";


// Récupération des données météo openweathermap.org
// Localisation de votre ville, à récupérer sur https://www.google.fr/maps :
// - Se placer au dessus de votre ville et appuyer sur le bouton droit de la souris
// - Cliquer sur la première ligne du menu qui s'affiche
// - Coller les valeurs ici en arrondissant les décimales au format demandé
// Exemple : Paris 48.8567, 2.361 / Toulouse 43.6014, 1.440 / Lyon 45.7688, 4.847
String latitude =  "44.0591"; // 90.0000 to -90.0000 negative for Southern hemisphere
String longitude = "1.385"; // 180.000 to -180.000 negative for West
// Créer une clé gratuite (api_key) sur openweathermap.org. La clé permet un nombre de requête limité par jour, mais suffisant.
String api_key = "**************************";
String units = "metric";  // ou "imperial"
String language = "fr";


/***************************************************************************
 * 4 paramètres pour afficher la température via un autre moyen que MaxPV! *
 ***************************************************************************/
// Mettre "sonde" à true pour afficher la température du cumulus. Sinon mettre à false
bool sonde = false;
// Pour afficher la température avec les décimales : mettre à false. Sinon mettre à true
bool temperatureEntier = false;
// Adresse IP du serveur pour appeler l'API retournant la température du ballon d'eau chaude
// Format : NNN.NNN.NNN.NNN
char serveurTemperature[] = "192.168.***.***"; // Exemple : "192.168.1.12"
// Chemin vers l'API retournant la température deu ballon d'eau chaude.
// Si l'API ne  retourne pas directement la valeur de la température (Ex : 62,5), alors alimenter les paramètres "baliseDebut" et "baliseFin" plus bas
// Format : /XXXXXX
char pathTemperature[] = "/api/get?param=06"; // Exemple : "/myapi/data?key=a2z3da0"
// Définition des balises début et fin pour récupérer la température dans un chaine de caractères structurée, sinon laisser vide
// Si la balise contient des double quotes ("), il faut rajouter un anti-slash (\)
// Exemple : 
// - Shelly 1P retourne une chaine de température du type : {"id": 100,"tC":21.9, "tF":71.5}
// - mettre baliseDebut = "\"tC\":" et baliseFin = ", \"tF\""
String baliseDebut = "";
String baliseFin = "";

// Puissance de coupure de votre compteur électrique (normalement il y a une tolérance de +10%, à confirmer).
// Utilisé pour l'affichage de la consommation sur l'écran de réserve d'énergie.
// Information indiquée sur votre compteur Linky ou facture sous la forme : 6kVA, ou 9kVA, ou 12kVA
// Si 6kVA, indiquer 6. Si 9kVA, indiquer 9. Etc...
uint puissanceCoupure = 6;


// Boitier horizontal prise à gauche = 3
// Boitier horizontal prise à droite = 1
int rotation = 3;

// Affichage en nombres entiers sur les écrans
bool nbrentier = true; // Pour les consos et prod instantanées. Meilleure visibilité à "true" (recommandé)
bool cumulEnWh = true; // Pour les cumuls journalier. Mettre "true" pour afficher les cumuls en Wh (recommandé), car MaxPV! retourne des kWh

// Eclairage intermédiaire au lancement et en sortie de veille
// Modifiable mais mettez IMPERATIVEMENT des multiples de 50
// Valeurs possibles : 0, 50, 100, 150, 200, 250
int luminositeChoisie = 100;

// Si l'alimentation se fait par batterie, mettre "true". Sinon mettre "false"
bool lipo = false;

// Si chauffage par radiateurs électriques, mettre "true" sinon "false". Permet d'afficher une icone radiateur en cas de grosse consommation
bool chauffageElectr = false;

// Mise en veille de l'affichage écran quand Production PV est à 0 (idée de Defaliz)
// La mise en veille se lance au démarrage après la connexion au serveur.
// Un simple clic sur le bouton de luminosité réaffiche l'écran pendant 5s.
// Un double clic sur le bouton de luminosité désactive la mise en veille jusqu'au prochain redémarrage.
bool veille = false;

// Clin d'oeil pour le 1er Avril
// Fait apparaitre un poisson à la place de l'icone météo ce jour là
bool poisson = false;

//*****************************************************************************
//*******************    Fin des données personnelles    **********************
//*****************************************************************************
