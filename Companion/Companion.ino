//***********************************************************************************************************
// COMPANION pour MaxPV!                                                                                   **/
//
String Version = "1.2";
//                                                                                                         **
/************************************************************************************************************
** Affichage déporté de consommation solaire pour MaxPV! basé sur Campanion MSunPV 2.50 de @jjhontebeyrie  **
************************************************************************************************************* 
** ATTENTION :                                                                                             **
** Le code est prévu pour LILYGO T-Display S3 avec écran 320x170                                           **   
*************************************************************************************************************
** Repository du Companion MSun PV : https://github.com/JJHontebeyrie/Companion                            **
*************************************************************************************************************
** Bibliothèques nécessaires :                                                                             **
**                                                                                                         **
**  1) A installer manuellement dans /Documents/Arduino/libraries en décompactant les bibliothèques ZIPpées**
**                                                                                                         **
**  https://github.com/PaulStoffregen/Time                                                                 **
**  https://github.com/JChristensen/Timezone                                                               **
**  https://github.com/Bodmer/JSON_Decoder                                                                 **
**  https://github.com/Bodmer/OpenWeather                                                                  **
**  TFT_eSPI : https://github.com/Bodmer/TFT_eSPI dans la version 2.5.34, attention à réaliser les actions **
**     ci-dessous dans le fichier "User_Setup_Select.h" de la librairie :                                 **
**     - décommenter la ligne "#include <User_Setups/Setup206_LilyGo_T_Display_S3.h>"                      **
**     - commenter la ligne "#include <User_Setup.h>"                                                      **
**    Permet de déclarer les bons paramètres pour LilyGO S3.                                               **
**    Si non paramétré alors l'écran s'affichera en noir!                                                  **
**                                                                                                         **
**  2) "Gestionnaire de bibliothèque" de Arduino IDE => à rajouter :                                       **
**   - OneButton par Matthias Hertel v2.0.4                                                                **
**   - ArduinoJson par Benoit Blachon v7.0.0                                                               **
**                                                                                                         ** 
**  3) "Gestionnaire de carte" de Arduino IDE => à rajouter :                                              **
**   - esp32 par Espressif Systems v2.0.7 (attention, téléchargement très long!)                           **
**                                                                                                         ** 
*************************************************************************************************************/

#include <TFT_eSPI.h>      // Attention : paramétrage de la librairie TFT_eSPI à réaliser pour utiliser LilyGO S3 (cf ci-dessus)
#include <WiFi.h>          
#include <JSON_Decoder.h>  // https://github.com/Bodmer/JSON_Decoder
#include <OpenWeather.h>   // Latest here: https://github.com/Bodmer/OpenWeather
#include "NTP_Time.h"      // Attached to this sketch, see that tab for library needs
#include <OneButton.h>     // OneButton par Matthias Hertel (dans le gestionnaire de bibliothèque de Arduino IDE)
#include "perso.h"         // Données personnelles à modifier dans le fichier (voir en haut de cet écran)
#include "logo.h"          // Logo de départ
#include "images.h"        // Images affichées sur l'écran
#include "meteo.h"         // Icones météo
#include <ESPmDNS.h>       // Pour le mDNS
#include <esp_adc_cal.h>   // Pour gérer la calibration de la batterie (cf. https://github.com/Xinyuan-LilyGO/T-Display-S3/blob/main/examples/GetBatteryVoltage/GetBatteryVoltage.ino)
// Watchdog (relance le Companion si perte de signal) Idée géniale de Bellule!
#include <esp_task_wdt.h>  // Watchdog en cas de déconnexion
#define WDT_TIMEOUT 20     // Délai d'activation du timeout du watchdog en secondes

#define MAXPV_DELAI_MAJ_SECS  10  // Intervalle en s de rafraichissement des données de MaxPV!
#define MAXPV_DATA_API_SIZE   29  // Nombre d'éléments retournés par l'API de MaxPV! "/api/get?alldata"
#define MAXPV_PARAM_API_SIZE  16  // Nombre d'éléments retournés par l'API de MaxPV! "/api/get?allparam"

#define ECRAN_DEMARRAGE         0 // Déclaré, mais pas géré
#define ECRAN_PRINCIPAL         1
#define ECRAN_INDEX_JOURNALIERS 2
#define ECRAN_METEO             3 // Déclaré, mais non géré
#define ECRAN_RESERVE_ENERGIE   4
#define ECRAN_JOURS_TEMPO       5
int ecranCourant = -1;            // Identifie l'écran à afficher
#define REGLE_MEME_ECRAN        0
#define REGLE_ECRAN_SUIVANT     1
#define REGLE_ECRAN_PRINCIPAL   2

// Controle que le bon paramétrage est utilisé pour la bibliothèque TFT_eSPI (cf. instructions dans l'entête)
#if USER_SETUP_ID != 206
#error "Attention, la librairie TFT_eSPI n'est pas correctement paramétrée pour LilyGO S3! Action à réaliser : le fichier 'TFT_eSPI\User_Setup_Select.h' doit avoir la ligne '#include <User_Setups/Setup206_LilyGo_T_Display_S3.h>' décommentée et la ligne '#include <User_Setup.h>' commentée."
#endif


TFT_eSPI lcd = TFT_eSPI();
TFT_eSprite sprite     = TFT_eSprite(&lcd);  // Tout l'écran
TFT_eSprite voyant     = TFT_eSprite(&lcd);  // Sprite voyant
TFT_eSprite depart     = TFT_eSprite(&lcd);  // Sprite écran d'accueil
TFT_eSprite chauffe    = TFT_eSprite(&lcd);  // Sprite indicateur chauffage électrique
TFT_eSprite wifi       = TFT_eSprite(&lcd);  // Sprite wifi
TFT_eSprite batterie   = TFT_eSprite(&lcd);  // Sprite batterie
TFT_eSprite meteo      = TFT_eSprite(&lcd);  // Sprite meteo
TFT_eSprite luminosite = TFT_eSprite(&lcd);  // Sprite réglage luminosité

// Couleurs bargraph
#define color0 0x10A2  // Gris foncé
// Couleurs pour affichage valeurs cumuls et légende
// Couleur Température ECS
#define color8 0x16DA  // Bleu clair

// Rayon d'arrondi des rectangles
#define RECT_RADIUS 4


// Chemin acces à l'API des données de MaxPV!
char pathData[] = "/api/get?alldata";
char pathParam[] = "/api/get?allparam";

// Variables pour programme
int puissancePV = 0;  // Production max en watt. Alimenté automatiquement à partir de P_INSTALLPV dans MaxPV!
int puissanceECS = 0; // Puissance ECS en watt. Alimenté automatiquement à  partir de P_RESISTANCE dans MaxPV!

long lastTime = 0;
long lastUpdateMaxPV = 0;
String Months[13] = { "Mois", "Jan", "Fev", "Mars", "Avril", "Mai", "Juin", "Juill", "Aout", "Sept", "Oct", "Nov", "Dec" };
String IP;      // Adresse IP de connexion du Companion
String RSSI;    // Puissance signal WiFi
bool wink = false;

// Boutons :
// - 0 : à gauche de la prise USB -> pour l'éclairage
// - 14 : à droite de la prise USB -> pour changement d'écran (cumuls)
// Pour inverser les fonctionnalités, inversez les valeur 0 et 14 ci-dessous
OneButton buttonGauche(0, true);  // Bouton éclairage
OneButton buttonDroit(14, true);  // Bouton cumuls

bool premiereLectureDonneesMaxPV = true; // Sera mis à false quand les données MaxPV auront été lues au moins une fois
// Pointeurs pour relance recherche valeurs
bool awaitingArrivals = true;    // Passe à "false" quand on vient de lire les données. Passe à "true" toutes les 10s lors de la réinit du cycle pour rafraichir les données
bool arrivalsRequested = false;  // Passe à "true" quand on est déjà en train de lire les données. Passe à "false" toutes les 10s lors de la réinit du cycle


// Variables pour dimmer
const int PIN_LCD_BL = 38;
const int freq = 1000;
const int ledChannel = 0;
const int resolution = 8;
bool inverse = true;
int dim, x;

// Variables pour batterie
long tensionBatterie;           // Voltage batterie en mV
long pourcentageBatterie = 100; // Charge en pourcentage de la batterie (100% à l'initialisation)
#define PIN_POWER_ON 15
#define PIN_BAT_VOLT 4
#define MIN_CHARG_USB 4300      // Tension mV minimum à partir de laquelle on considère que la batterie est en cours de charge (ie : port USB branché)


// Variables affichant les valeurs reçues depuis le MaxPV!
String PV, CU, CO, TEMPCU;            // Consos et températures.
String CUMCO, CUMINJ, CUMPV, CUMBAL;  // Cumuls

// Wifi
int status = WL_IDLE_STATUS;
WiFiClient client;

// Chaines pour decryptage
String dataString = "";
String dataMaxPV = "";                       // Données restituées par l'API MaxPV (compteurs)
String paramMaxPV = "";                      // Données restituées par l'API MaxPV (paramètres)
String dataTemperature = "";                 // Données restituées par l'API externe de température (hors MaxPV)
String MsgDataSplit[MAXPV_DATA_API_SIZE];    // Valeurs à récupérer depuis l'api
String MsgParamSplit[MAXPV_PARAM_API_SIZE];  // Valeurs à récupérer depuis l'api

// Données de openweather.org
#define TIMEZONE euCET  // Voir NTP_Time.h tab pour d'autres "Zone references", UK, usMT etc
String lever, coucher, tempExt, humExt, icone, ID;
OW_Weather ow;  // Weather forecast librairie instance
// Update toutes les 15 minutes, jusqu'à 1000 requêtes par jour gratuit (soit ~40 par heure)
const int UPDATE_WEATHER_INTERVAL_SECS = 15 * 60UL;  // 15 minutes (900s)
bool booted = true;
long lastDownloadUpdateWeather = millis();
String timeNow = "";
String dateNow = "";
String dateNow2 = "";

// Variables pour serveur web
WiFiServer server(80);
String header;                         // Variable to store the HTTP request
unsigned long currentTime = millis();  // Current time
unsigned long previousTime = 0;        // Previous time
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 5000;


// Déclaration pour les jours TEMPO
// Code repris de "Afficheur_Tempo" de pjeantaud
// A corriger, car on utilise une 2ième API JSON (en plus de celle de la météo), idem pour la connexion Web. Mais bon ça marche et pas eu suffisament de temps pour optimiser tout ça...
#include <HTTPClient.h>
#include <ArduinoJson.h>
long lastUpdateTempo = 0;
const int UPDATE_INTERVALLE_TEMPO_SECS  = 60 * 60UL; // Soit 3600s (1h)
String restantsBleus, restantsBlancs, restantsRouges, JourJJ, JourJJ1;
uint32_t colorTempoJJ  = TFT_LIGHTGREY;
uint32_t colorTempoJJ1 = TFT_LIGHTGREY;
// Est-ce que l'année est bissextile ?
#define EstAnneeBissextile(year) ((((year) % 4 == 0) && ((year) % 100 != 0)) || ((year) % 400 == 0))

// Transforme les kWh en Wh (pour les index journalier)
int kwh_to_wh(float kwh) { return kwh * 1000;}
// Transforme les Wh en kWh
float wh_to_kwh(float wh) {return wh / 1000.0;}

///////////////////////////////////////////////////////////////////////////////////////
//                                 Routine SETUP                                     //
///////////////////////////////////////////////////////////////////////////////////////
void setup() {

  // Activation du port batterie interne du LilyGo S3, afin que l'alimentation externe fonctionne (cf. https://github.com/Xinyuan-LilyGO/T-Display-S3?tab=readme-ov-file#9%EF%B8%8F%E2%83%A3-faq)
  // This IO15 must be set to HIGH, otherwise nothing will be displayed when USB is not connected.
  pinMode(PIN_POWER_ON, OUTPUT);
  digitalWrite(PIN_POWER_ON, HIGH);
  
    // Initialisation ecran
  lcd.init();
  lcd.setRotation(rotation);
  //lcd.setSwapBytes(true);

  // Paramètres pour dimmer
  // Initialisation dès le début pour éviter les flashs de changement de luminosité pendant l'affichage de l'écran de démarrage
  ledcSetup(ledChannel, freq, resolution);
  ledcAttachPin(PIN_LCD_BL, ledChannel);
  // Tamisage écran par défaut (foçage à 100 minimum au démarrage en cas de mauvais paramétrage)
  if (luminositeChoisie < 100) dim = 100;
  else                         dim = luminositeChoisie;
  ledcWrite(ledChannel, dim);

  // Affichage logo depart
  lcd.fillScreen(TFT_BLACK);
  //w = lcd.width();
  //h = lcd.height();
  depart.createSprite(320, 170);
  depart.setSwapBytes(true);
  depart.pushImage(20, 15, 280, 120, splashscreen);
  depart.setTextColor(TFT_LIGHTGREY);
  depart.setTextDatum(MC_DATUM);
  depart.drawString("Version " + (Version), 160, 150, 2);
  depart.pushSprite(0, 0, TFT_BLACK);

  // Initialisation port série et attente ouverture
  Serial.begin(115200);
  while (!Serial) {
    delay(100);  // wait for serial port to connect. Needed for native USB port only
  }

  Serial.println("Configuraton du WDT...");
  esp_task_wdt_init(WDT_TIMEOUT, true);  //enable panic so ESP32 restarts
  esp_task_wdt_add(NULL);                //add current thread to WDT watch
  lastTime = millis();                   // Récupération du temps de départ du lancement du Watchdog pour prévenir du redémarrage éventuel pendant la phase de lancement et de connexion

  // delete old config et vérif de deconnexion
  WiFi.disconnect(true);
  delay(1000);

  depart.setTextColor(TFT_LIGHTGREY);
  depart.setTextDatum(MC_DATUM);
  depart.drawString("Connexion sur " + String(ssid) + " en cours", 160, 150, 2);
  depart.pushSprite(0, 0);

  // Creation des sprites affichage
  sprite.createSprite(320, 170);  // Tout l'ecran
  sprite.setTextDatum(MC_DATUM);  // Alignement texte au centre du rectangle le contenant
  voyant.createSprite(64, 64);    // Voyant rouge, vert ou bleu indiquant si on peut lancer un truc
  voyant.setSwapBytes(true);      // (Pour affichage correct d'une image)
  chauffe.createSprite(40, 40);   // Chauffage en cours
  chauffe.setSwapBytes(true);
  wifi.createSprite(27, 27);      // Icone construite pour wifi
  batterie.createSprite(15, 30);  // Image batterie
  meteo.createSprite(50, 50);     // Icone meteo
  meteo.setSwapBytes(true);
  luminosite.createSprite(200, 90);  // Ecran de sélection de la luminosité

  // Etablissement connexion wifi
  WiFi.mode(WIFI_STA);
  WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);
  //*****************************************************************************
  // Si difficultés à se connecter >  wpa minimal (décommenter la ligne suivante)
  //WiFi.setMinSecurity(WIFI_AUTH_WPA_PSK);
  //*****************************************************************************
  String waitingTxt = "";
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");

    // Message d'activité sur l'écran d'accueil pendant l'attente de la connexion
    if (waitingTxt.length() < 40)
      waitingTxt = waitingTxt + ".";
    else {
      waitingTxt = ".";
      // Pour réinitialiser la zone de texte "...."
      depart.fillRoundRect(0, 158, 320, 10, 0, TFT_BLACK);
    }

    // Détection d'un redémarrage imminent dans 1 seconde environ => Echec de connexion
    if (lastTime + (WDT_TIMEOUT - 1) * 1000 < millis()) {
      // Pour réinitialiser la zone de texte "Connexion...."
      depart.fillRoundRect(0, 143, 320, 15, 0, TFT_BLACK);
      depart.drawString("Echec de la connexion ...", 160, 150, 2);
    } else {
      // Pour réinitialiser la zone de texte "Connexion...."
      depart.fillRoundRect(0, 143, 320, 15, 0, TFT_BLACK);
      // Message de connexion avec le temps restant avant le reboot
      depart.drawString("Connexion sur " + String(ssid) + " en cours (" + (lastTime + (WDT_TIMEOUT - 1) * 1000 - millis()) / 1000 + "s)", 160, 150, 2);
    }

    depart.setTextColor(TFT_LIGHTGREY, TFT_BLACK); // Conserver la couleur de fond pour éviter un mauvais affichage pour les "...."
    depart.drawString(waitingTxt, 160, 163, 2);
    depart.pushSprite(0, 0);
    delay(100);
  }

  Serial.println("WiFi connected.");

  IP = WiFi.localIP().toString();
  RSSI = String(WiFi.RSSI());

  Serial.println("IP de Companion attribuée sur le réseau : " + IP);
  Serial.println("Force du signal (RSSI): " + RSSI + " dB \n");

  // Récupération de l'heure
  udp.begin(localPort);
  syncTime();
  esp_task_wdt_reset();

  // Affichage texte sur écran de départ
  depart.setTextColor(TFT_LIGHTGREY);
  depart.setTextDatum(MC_DATUM);
  // Pour réinitialiser la zone de texte "Connexion...."
  depart.fillRoundRect(0, 143, 320, 15, 0, TFT_BLACK);
  // Pour réinitialiser la zone de texte "...."
  depart.fillRoundRect(0, 158, 320, 10, 0, TFT_BLACK);
  // Message de confirmation de connexion avec l'IP attribuée et la puissance du signal
  depart.drawString("Connexion OK (" + (IP) + ") " + RSSI + "dB", 160, 150, 2);
  depart.pushSprite(0, 0);

  // Tamisage écran (dim varie de 0 à 250) d'après le paramétrage dans "perso.h"
  dim = luminositeChoisie;
  ledcWrite(ledChannel, dim);

  if (veille) {
    delay(5000);
    // Pour réinitialiser la zone de texte "Connexion...."
    depart.fillRoundRect(0, 143, 320, 15, 0, TFT_BLACK);
    depart.drawString("Mise en veille automatique", 160, 150, 2);
    depart.pushSprite(0, 0);
    delay(2000);
  }

  // Activation du simple et double clic sur les 2 boutons
  buttonGauche.attachClick(handleClickGauche);
  buttonGauche.attachDoubleClick(handleDoubleClickGauche);
  buttonDroit.attachClick(handleClickDroit);
  buttonDroit.attachDoubleClick(handleDoubleClickDroit);
  
  // Activation du mDNS responder: encore idée de Bellule
  // il suffit de taper dans l'invite du navigateur web
  // cette simple commande http://companion pour y accéder
  if (!MDNS.begin("companion")) {
    Serial.println("Error setting up MDNS responder!");
    while (1) { delay(1000); }
  }
  Serial.println("mDNS responder started");

  // Lancement serveur web
  server.begin();
  // Ajout du service to MDNS-SD
  MDNS.addService("http", "tcp", 80);

  esp_task_wdt_reset();
}

///////////////////////////////////////////////////////////////////////////////////////
//                                 Routine LOOP                                      //
///////////////////////////////////////////////////////////////////////////////////////
void loop() {

  // Lit l'heure et l'état de la batterie toutes les secondes
  if (lastTime + 1000 < millis()) {
    GetTimeDate();

    // Etat batterie
    if (lipo)
      GetBatteryVoltage();
    
    lastTime = millis();
  }

  // Données météo
  // Test pour voir si un rafraissement est nécessaire
  if (booted || (lastDownloadUpdateWeather + UPDATE_WEATHER_INTERVAL_SECS * 1000UL < millis())) { // 15 min convertis en ms
    RecuperationDonneesMeteo();
    lastDownloadUpdateWeather = millis();
  }

  // Données EDF jours TEMPO
  if (affichageEcranTempo)
    if (booted || (lastUpdateTempo + UPDATE_INTERVALLE_TEMPO_SECS  * 1000UL < millis())) {
      RecuperationDonneesJoursTempo();
      lastUpdateTempo = millis();
    }

  // Rafraichissement des données émises par MaxPV
  if (awaitingArrivals) {
    if (!arrivalsRequested) {
      arrivalsRequested = true;
      RecuperationDonneesMaxPV();
      FormatageDonnesMaxPV();
      // On réaffiche l'écran courant
      AfficheEcran(REGLE_MEME_ECRAN);
    }
  }

  // Relance de lecture des données (10 sec)
  if (lastUpdateMaxPV + MAXPV_DELAI_MAJ_SECS * 1000UL < millis()) {
    Serial.println("Rafraichissement des données...");
    resetCycle();
    lastUpdateMaxPV = millis();
  }

  // Teste si la veille est demandée
  if (veille) {
    dim = luminositeChoisie;       // Pour la sortie de veille
    if (PV.toInt() <= 0) dim = 0;  // on met l'écran en arrêt si pv = 0

    ledcWrite(ledChannel, dim);
  }

  // Ecoute des boutons
  buttonGauche.tick();
  buttonDroit.tick();

  // Teste si demande lecture serveur web
  serveurweb();

  booted = false;
  esp_task_wdt_reset();
}




/***************************************************************************************
**   Récupération des infos Tempo                                                     **
**   Pour vérifier la période active de Tempo :                                       **
**   if (( numJour < 91 ) || ( numJour > 303 )).                                      **
***************************************************************************************/
void RecuperationDonneesJoursTempo() {

  String requeteTempo;

  Serial.println("[HTTP TEMPO] begin...");
  HTTPClient http;
  //
  // Première requete pour connaitre la couleur du jour J et J+1
  //
  requeteTempo = ("https://particulier.edf.fr/services/rest/referentiel/searchTempoStore?dateRelevant=");
  requeteTempo += String(dateNow2);
  Serial.println(requeteTempo);
  http.begin(requeteTempo);

  Serial.print("[HTTP TEMPO] GET...\n");
  int httpJour = http.GET();
  Serial.printf("", httpJour);
  String recup1 = http.getString();
  Serial.println(recup1);

  StaticJsonDocument<200> doc1;   
  deserializeJson(doc1, recup1); 
  const char* JourJ = doc1["couleurJourJ"];
  const char* JourJ1 = doc1["couleurJourJ1"];

  JourJJ =  String(JourJ); 
  Serial.println("Jour J : " + JourJJ);
  JourJJ1 = String(JourJ1);
  Serial.println("Jour J+1 : " + JourJJ1);
  // Résultats :
  colorTempoJJ = Couleur_Jour_Tempo (JourJJ);
  colorTempoJJ1 = Couleur_Jour_Tempo (JourJJ1);

  //
  // Deuxième requête pour connaitre le nombre de jours restants par couleur
  //
  requeteTempo = "https://particulier.edf.fr/services/rest/referentiel/getNbTempoDays?TypeAlerte=TEMPO";
  Serial.println(requeteTempo);
  http.begin(requeteTempo);
  
  Serial.println("[HTTP] GET...");
  int httpJoursRestants = http.GET();   
  String recup2 = http.getString();

  StaticJsonDocument<200> doc2;   
  deserializeJson(doc2, recup2); 
  int couleurParamBLANC = doc2["PARAM_NB_J_BLANC"];
  int couleurParamROUGE = doc2["PARAM_NB_J_ROUGE"];
  int couleurParamBLEU = doc2["PARAM_NB_J_BLEU"];
  http.end();
  Serial.println("[HTTP TEMPO] End");

  // Résultats :
  restantsBleus =  String(couleurParamBLEU); 
  restantsBlancs = String(couleurParamBLANC);
  restantsRouges = String(couleurParamROUGE); 
  Serial.println("TEMPO Jours restants => Bleu : " + restantsBleus+ " / Blancs : " + restantsBlancs+ " / Rouges : " + restantsRouges);

  esp_task_wdt_reset();
}

/* Détermine la couleur du jour TEMPO en fonction du texte retourné */
uint32_t Couleur_Jour_Tempo (String texteCouleurJour)
{
  if      (texteCouleurJour.equalsIgnoreCase("TEMPO_BLEU"))
      return TFT_NAVY;
  else if (texteCouleurJour.equalsIgnoreCase("TEMPO_BLANC"))
      return TFT_WHITE;
  else if (texteCouleurJour.equalsIgnoreCase("TEMPO_ROUGE"))
      return TFT_RED;
  else if (texteCouleurJour.equalsIgnoreCase("NON_DEFINI"))
      return TFT_LIGHTGREY;
  else
      return TFT_LIGHTGREY;
}




/*
 * Ajout d'un séparateur pour les milliers pour améliorer la lisibilité. Par défaut : ".", sinon préciser un autres caractère
 * Fonctionne avec des entiers positifs ou négatifs. (Pas testé avec les décimales!)
 */
void strAjoutSepMillier(char dst[16], int32_t nombre, const char separateur = ' ') {
  char src[16];
  char *p_src = src;
  char *p_dst = dst;
  uint8_t longeur, virgules;
  ltoa(nombre, src, 10);
  longeur = strlen(src);

  if (*p_src == '-') {  // si négatif
    *p_dst++ = *p_src++;
    longeur--;
  }

  for (virgules = 2 - (longeur % 3); (*p_src != '\0'); virgules = (virgules == 2) ? 0 : virgules + 1) {
    *p_dst++ = *p_src++;
    if (virgules == 1) {
      *p_dst++ = separateur;
    }
  }
  *--p_dst = '\0';
}


/***************************************************************************************
**                             Affichage principal
***************************************************************************************/
void AfficheEcranPrincipal() {

  uint32_t selectedColor;
  uint16_t *selectedButton;
  char str2Display[16]; // Pour affichage des valeur avec séparateur des milliers
  
  ecranCourant = ECRAN_PRINCIPAL;

  //test(); // teste les affichages

  //  Dessin fenêtre principale
  sprite.fillSprite(TFT_BLACK);
  sprite.setTextDatum(MC_DATUM);
  sprite.setTextColor(TFT_WHITE);

  // Panneaux principaux
  sprite.drawRoundRect(0, 0, 226, 55, RECT_RADIUS, TFT_LIGHTGREY);
  sprite.drawString("PRODUCTION PV", 113, 10, 2);
  sprite.drawRoundRect(0, 57, 226, 55, RECT_RADIUS, TFT_LIGHTGREY);
  sprite.drawString("ROUTAGE ECS", 113, 68, 2);
  sprite.drawRoundRect(0, 114, 226, 55, 4, TFT_LIGHTGREY);
  if (CO.toInt() >= 0)  sprite.drawString(" CONSOMMATION RESEAU ", 115, 126, 2);
  else                  sprite.drawString(" EXPORTATION RESEAU ", 115, 126, 2);

  // Panneau de droite sur l'écran : heure, date, batterie
  sprite.drawRoundRect(234,  0, 86, 54, RECT_RADIUS, TFT_LIGHTGREY);

  // Affichage Météo
  meteo.pushImage(0,0,50,50,unknown);
  if  (icone == "01d")                      {meteo.pushImage(0,0,50,50,clear_day);            goto suite;}
  if  (icone == "01n")                      {meteo.pushImage(0,0,50,50,clear_night);          goto suite;}
  if  (icone == "02d")                      {meteo.pushImage(0,0,50,50,partly_cloudy_day);    goto suite;}
  if  (icone == "02n")                      {meteo.pushImage(0,0,50,50,partly_cloudy_night);  goto suite;}
  if ((icone == "03d") or (icone == "03n")) {meteo.pushImage(0,0,50,50,few_clouds);           goto suite;}
  if ((icone == "04d") or (icone == "04n")) {meteo.pushImage(0,0,50,50,cloudy);               goto suite;}
  if ((icone == "09d") or (icone == "09n")) {meteo.pushImage(0,0,50,50,rain);                 goto suite;}
  if ((icone == "10d") or (icone == "10n")) {meteo.pushImage(0,0,50,50,lightRain);            goto suite;}
  if ((icone == "11d") or (icone == "11n")) {meteo.pushImage(0,0,50,50,thunderstorm);         goto suite;}
  if ((icone == "13d") or (icone == "13n")) {meteo.pushImage(0,0,50,50,snow);                 goto suite;}
  if ((icone == "50d") or (icone == "50n")) {meteo.pushImage(0,0,50,50,fog);                  goto suite;}
  if  (icone == "80d")                      {meteo.pushImage(0,0,50,50,splash);               goto suite;}
  if  (ID == "301")                         {meteo.pushImage(0,0,50,50,drizzle);              goto suite;} 
  if  (ID == "221")                          meteo.pushImage(0,0,50,50,wind);

suite:
  // Affiche icone metéo et température extérieure
  // Affichage température et du ° de la température
  sprite.setTextDatum(MR_DATUM);  // centre droit
  sprite.drawString(tempExt, 256, 140, 2);
  sprite.drawCircle(260, 136, 2, TFT_WHITE);  // cercle pour le caractère "°" qui n'est pas dans la police de caractères
  // Affichage du taux d'humidité
  sprite.drawString(humExt, 256, 160, 2);
  sprite.setTextDatum(ML_DATUM);  // centre droit
  sprite.drawString("%", 258, 160, 2); // Position centre cercle 260 - rayon 2
  
  sprite.setTextDatum(MC_DATUM);              // centre milieu
  // Affiche "*" pour indiquer le risque de gel
  if (tempExt.toInt() <= 3) {
    sprite.setTextColor(TFT_CYAN);
    sprite.drawString("*", 235, 168, 4);
  }
  
  // Icone météo
  meteo.pushToSprite(&sprite, 272, 122, TFT_TRANSPARENT); // Normalement devrait être 270,120 pour une image 50x50. Mais comme il y a un peu de bordure sauf pour "partly_cloudy_day", on ruse en décalant plus l'image

  // Affichage heure et date
  sprite.setTextColor(TFT_WHITE);
  sprite.drawString(String(timeNow), 277, 19, 4);
  sprite.drawString(String(dateNow), 277, 42, 2);

  // Puissance du signal WiFi
  AffichageSignalWifi();

  // Gestion batterie
  if (lipo) 
    AffichageIndicateurBatterie();

  // Affichage de la température de l'ECS si sonde de température présente
  if (sondeTemperatureECS) {

    // Pour afficher le cercle de température coloré en fonction de la valeur de la température
    selectedColor = TFT_CYAN;
    if (TEMPCU.toInt() >= 30) selectedColor = color8;     // Bleu clair
    if (TEMPCU.toInt() >= 50) selectedColor = TFT_ORANGE; // Orange
    if (TEMPCU.toInt() >= 60) selectedColor = TFT_RED;    // Rouge

    // Superposition d'un grand disque et d'un petit disque
    sprite.fillSmoothCircle(25, 84, 22, selectedColor); // Disque coloré
    sprite.fillSmoothCircle(25, 84, 19, TFT_BLACK);     // Disque noir pour l'intérieur

    // TODO : à améliorer en cas de format différent de xx.x
    sprite.setTextDatum(MC_DATUM);  // centre milieu
    if (TEMPCU.length() > 2) {
      // Température avec 1 décimale (=> 4 caractères)
      sprite.drawString(TEMPCU, 23, 84, 2);
      sprite.drawCircle(39, 80, 2, TFT_WHITE);  // Cercle pour °
    } else {
      // Température sans décimale (=> 2 caractères )
      sprite.drawString(TEMPCU, 25, 84, 2);
      sprite.drawCircle(36, 80, 2, TFT_WHITE);  // Cercle pour °
    }
  }

  //
  // Affichage des valeurs des compteurs
  //
  sprite.setFreeFont(&Orbitron_Light_24);  // Police d'affichage "Orbitron_Light_24", liste des caractères disponibles https://github.com/Bodmer/TFT_eSPI/blob/master/Fonts/Custom/Orbitron_Light_24.h

  // Affichage valeur production PV
  if (PV.toInt() > 0)  {
    if (nbrentier) {
      strAjoutSepMillier(str2Display, PV.toInt());
      sprite.drawString(String(str2Display) + " w", 113, 35);
    } else {
      sprite.drawString(replacePointParVirgule(PV) + " w", 113, 35);
    }
  } else {
    // Pas de production, affichage heure lever et coucher du soleil
    sprite.setTextColor(TFT_YELLOW);
    sprite.drawString(lever + " <-----> " + coucher, 113, 40, 4);
  }

  sprite.setTextColor(TFT_WHITE);

  // Affichage valeur routage ECS
  if (nbrentier) {
    strAjoutSepMillier(str2Display, CU.toInt());
    sprite.drawString(String(str2Display) + " w", 113, 92);
  } else {
    sprite.drawString(replacePointParVirgule(CU) + " w", 113, 92);
  }

  // Affichage valeur Consommation réseau
  if (nbrentier) {
    strAjoutSepMillier(str2Display, CO.toInt());
    sprite.drawString(String(str2Display) + " w", 113, 150);
  } else {
    sprite.drawString(replacePointParVirgule(CO) + " w", 113, 150);
  }
  // Affichage de la couleur tempo du jour et j+1 sur l'écran principal
  if (affichageEcranTempo) {
    sprite.fillRoundRect(4, 120, 20, 20, RECT_RADIUS, colorTempoJJ);
    sprite.fillRoundRect(4, 144, 20, 20, RECT_RADIUS, colorTempoJJ1);
  }


  // Voyant assistant de la réserve d'énergie (permet de savoir si l'on peut lancer un appareil électrique sans consommer depuis le réseau)
  // Calcul de la réserve d'énergie :
  // - Si vide < 0W (= consommation)  => Rouge
  // - Si entre 0 et 200W             => Orange
  // - Si entre 200 et 1000W          => Jaune
  // - Si entre 1000 et 2000W         => vert
  // - Si supérieure à 2000W          => Turquoise
  // Si exportation d'énergie, alors même bouton avec icone énergie
  int reserveEnergie = CU.toInt() - CO.toInt(); // En W
  //bool topInjection = (CO.toInt() < 0) ? true : false;
  if      (reserveEnergie <=    0)                                            selectedButton = (uint16_t *) Button_Icon_Red;
  else if (reserveEnergie >     0 and reserveEnergie <  200)                  selectedButton = (uint16_t *) Button_Icon_Orange;
  else if (reserveEnergie >=  200 and reserveEnergie < 1000)                  selectedButton = (uint16_t *) Button_Icon_Yellow;
  else if (reserveEnergie >= 1000 and reserveEnergie < 2000)                  selectedButton = (uint16_t *) Button_Icon_Green;
  else                                                                        selectedButton = (uint16_t *) Button_Icon_Turquoise;

  // Dessin du voyant
  voyant.pushImage(0, 0, 64, 64, selectedButton);
  voyant.pushToSprite(&sprite, 230, 60, TFT_BLACK);
  if (reserveEnergie > 100) {
    // Affiche la valeur de la réserve d'énergie
    sprite.setTextDatum(MC_DATUM);
    sprite.setTextColor(TFT_BLACK);
    sprite.drawString(replacePointParVirgule(String(wh_to_kwh(reserveEnergie),1)), 230+31, 60+28); // ",1" permet de ne garder qu'une décimale pour les float
  }

  // En cas de chauffage électrique : affichage icone radiateur quand une grosse consommation est détectée
  if (chauffageElectr) {
    if ((CO.toInt() + PV.toInt() > 3000) and (CU.toInt() < 100)) {
      chauffe.pushImage(0, 0, 40, 40, Radiateur);
      chauffe.pushToSprite(&sprite, 4, 122, TFT_BLACK);
    }
  }

  // Appel routine affichage des bargraphes latéraux
  Indicateurs_Graphiques();

  // Rafraichissement de tout l'écran
  sprite.pushSprite(0, 0);

  // Remise à zéro du watchdog
  esp_task_wdt_reset();
}



/***************************************************************************************
**                        Affichage de la page des cumuls
***************************************************************************************/
void AfficheEcranIndexJournaliers() {

  ecranCourant = ECRAN_INDEX_JOURNALIERS;

  //  Dessin fenêtre noire et titre
  sprite.fillSprite(TFT_BLACK);
  sprite.fillRoundRect(0, 2, 320, 25, 0, TFT_DARKGREY);
  sprite.setTextDatum(MC_DATUM);
  sprite.setTextColor(TFT_WHITE);
  if (cumulEnWh) sprite.drawString("INDEX JOURNALIERS (en Wh)", 160, 15, 2);
  else           sprite.drawString("INDEX JOURNALIERS (en kWh)", 160, 15, 2);

  sprite.setTextColor(TFT_WHITE);
  sprite.drawRoundRect(0, 32, 159, 55, RECT_RADIUS, TFT_LIGHTGREY);
  sprite.drawString("PRODUCTION PV", 82, 42, 2);
  sprite.drawRoundRect(0, 89, 159, 55, RECT_RADIUS, TFT_LIGHTGREY);
  sprite.drawString("ROUTAGE ECS", 82, 99, 2);

  sprite.drawRoundRect(161, 32, 159, 55, RECT_RADIUS, TFT_LIGHTGREY);
  sprite.drawString("CONSOMMATION", 241, 42, 2);
  sprite.drawRoundRect(161, 89, 159, 55, RECT_RADIUS, TFT_LIGHTGREY);
  sprite.drawString("INJECTION", 241, 99, 2);


  // Affichage des valeurs cumuls
  if (cumulEnWh) {
    char str2Display[16];

    strAjoutSepMillier(str2Display, CUMPV.toInt());
    sprite.drawString(str2Display, 82, 65);
    strAjoutSepMillier(str2Display, CUMBAL.toInt());
    sprite.drawString(str2Display, 82, 122);
    strAjoutSepMillier(str2Display, CUMCO.toInt());
    sprite.drawString(str2Display, 241, 65);
    strAjoutSepMillier(str2Display, CUMINJ.toInt());
    sprite.drawString(str2Display, 241, 122);
  } else {
    sprite.drawString(replacePointParVirgule(CUMPV), 82, 65);
    sprite.drawString(replacePointParVirgule(CUMBAL), 82, 122);
    sprite.drawString(replacePointParVirgule(CUMCO), 241, 65);
    sprite.drawString(replacePointParVirgule(CUMINJ), 241, 122);
  }

  // Affichage d'une ligne avec les infos Wifi
  //sprite.fillRoundRect(0,150,320,20,RECT_RADIUS,TFT_LIGHTGREY);
  sprite.setTextColor(TFT_LIGHTGREY);
  sprite.drawString("http://companion (" + IP + ") " + RSSI + "dB v" + Version, 160, 160, 2);

  // Rafraichissement écran (indispensable après avoir tout dessiné)
  sprite.pushSprite(0, 0);
}


/************************************************************************
 ** Permet l'affichage en gros de la réserve d'énergie afin d'être
 ** lisible de loin. On la consommation réseau si réserve energie vide
 ************************************************************************/
// Zone de traçage : 316 px (car cadre gris autour)
void AfficheEcranReserveEnergie() {

  char str2Display[16]; // Pour affichage des valeur avec séparateur des milliers

  int reserveEnergie = CU.toInt() - CO.toInt(); // En W
  bool topInjection = (CO.toInt() < 0) ? true : false;
  int i;

  ecranCourant = ECRAN_RESERVE_ENERGIE;

  sprite.fillSprite(TFT_BLACK);
  sprite.fillRoundRect(0, 2, 320, 25, 0, TFT_DARKGREY);
  // Encadrement
  sprite.drawRoundRect(0, 32, 320, 138, RECT_RADIUS, TFT_LIGHTGREY);

  if (reserveEnergie >= 0) {
    //
    // Cas : réserve d'énergie disponible
    //
    // Le max de la réserve correspond à la production PV max possible => Pmax? ou puissancePV

    uint32_t selectedColor;
    float echelle = 316.0 / puissancePV;
    //float echelle = 316.0 / puissancePV * 9 / 10; // On prend 90% de la puissance de crete théorique
    int reserveEnergieSupPV = 0; // Est utilisé dans le cas où la réserve est supérieure à la puissance de prod théorique. On limite la taille de la zone colorée, mais on affiche la bonne valeur.

    if (reserveEnergie > puissancePV) { // Les 2 valeurs sont en W
      reserveEnergieSupPV = reserveEnergie;
      reserveEnergie = puissancePV;
    }
    int width = reserveEnergie * echelle; // Largeur du rectancle coloré en fonction de l'échelle

    // Calcul de la réserve d'énergie :
    // - Si vide < 0W (= consommation)  => Rouge
    // - Si entre 0 et 200W             => Orange
    // - Si entre 200 et 1000W          => Jaune
    // - Si entre 1000 et 2000W         => vert
    // - Si supérieure à 2000W          => Turquoise
    if      (reserveEnergie >     0 and reserveEnergie <  200) selectedColor = TFT_ORANGE;
    else if (reserveEnergie >=  200 and reserveEnergie < 1000) selectedColor = TFT_YELLOW;
    else if (reserveEnergie >= 1000 and reserveEnergie < 2000) selectedColor = TFT_GREEN;
    else                                                       selectedColor = TFT_CYAN;

    // Titre
    sprite.setTextDatum(MC_DATUM);
    sprite.setTextColor(TFT_WHITE);
    sprite.drawString("RESERVE D'ENERGIE", 160, 15, 2);

    // Zone de réserve d'énergie totale
    sprite.fillRoundRect(2, 34, width, 134, RECT_RADIUS, selectedColor);

    // Zone de la réserve correspondant à de l'injection
    if (topInjection) {
      // Création zone blanche (injection => CO négatif) sur la zone déjà tracée
      int width2 = abs(CO.toInt()) * echelle; // Largeur du rectancle en fonction de l'échelle
      sprite.fillRoundRect(2+width-width2, 34, width2, 134, RECT_RADIUS, TFT_WHITE);
      if (2+width-width2 > 3)
        sprite.fillRect(2+width-width2, 34, 2, 134, TFT_WHITE); // Rectangle 2px avec angles droit sur la partie gauche pour ne pas avoir d'arrondi, sauf si c'est le début de la zone de traçage pour garder les arrondis
    }

    // Création des lignes verticales de l'échelle
    for (i = 1; i < puissancePV / 1000; i++) {
      int x = i * 1000 * echelle;
      sprite.drawLine(x, 34, x, 167, TFT_DARKGREY);
      sprite.setTextDatum(ML_DATUM);
      sprite.setTextColor(TFT_DARKGREY);
      if (puissancePV <= 6000) { // Si moins de 6 graduations, on met l'unité sur la même ligne. Sinon retour à la ligne
        sprite.drawString(String(i)+" kW", x+5, 42, 2); 
      } else if (puissancePV <= 9000) {
        sprite.drawString(String(i), x+5, 42, 2);
        sprite.drawString("kW",      x+5, 53, 2);
      } else {
        sprite.drawString(String(i), x+5, 42, 2);
      }
    }

    // Affichage valeur
    sprite.setTextDatum(MC_DATUM);
    sprite.setTextColor(TFT_LIGHTGREY);

    if (reserveEnergieSupPV) strAjoutSepMillier(str2Display, reserveEnergieSupPV);
    else                     strAjoutSepMillier(str2Display, reserveEnergie);
    if (nbrentier) sprite.drawString(String(str2Display) + " w", 160, 101); // Pas utile normalement, car les données sont en entier
    else           sprite.drawString(replacePointParVirgule(String(str2Display)) + " w", 160, 101);

  } else {

    //
    // Cas : consommation réseau (reserveEnergie < 0)
    //
    float echelle = 316.0 / puissanceCoupure; // Attention : puissanceCoupure est en kW
    int reserveEnergieSupCoup = 0; // Est utilisé dans le cas où la conso est supérieure à la puissance de coupure. On limite la taille de la zone colorée, mais on affiche la bonne valeur.

    reserveEnergie = abs(reserveEnergie);
    if (reserveEnergie / 1000.0 > puissanceCoupure) {
      reserveEnergieSupCoup = reserveEnergie;
      reserveEnergie = puissanceCoupure * 1000.0;
    }
    int width = reserveEnergie * echelle / 1000.0; // Largeur du rectancle coloré en fonction de l'échelle

    // Titre
    sprite.setTextDatum(MC_DATUM);
    sprite.setTextColor(TFT_WHITE);
    sprite.drawString("CONSOMMATION RESEAU", 160, 15, 2);

    // Zone de la réserve d'énergie, qui est ici de la consommation réseau
    sprite.fillRoundRect(2, 34, width, 134, RECT_RADIUS, TFT_RED);

    // Création des lignes verticales de l'échelle
    for (i = 1; i < puissanceCoupure; i++) {
      int x = i * echelle;
      sprite.drawLine(x, 34, x, 167, TFT_DARKGREY);
      sprite.setTextDatum(ML_DATUM);
      sprite.setTextColor(TFT_DARKGREY);
      if (puissanceCoupure <= 6) { // Si moins de 6 graduations, on met l'unité sur la même ligne. Sinon retour à la ligne
        sprite.drawString(String(i)+" kW", x+5, 42, 2); 
      } else if (puissanceCoupure <= 9) {
        sprite.drawString(String(i), x+5, 42, 2);
        sprite.drawString("kW",      x+5, 53, 2);
      } else {
        sprite.drawString(String(i), x+5, 42, 2);
      }
    }

    // Libellé "coupure" (puissance de coupure), sur la droite de l'écran
    sprite.setTextDatum(MC_DATUM);
    sprite.setTextColor(TFT_DARKGREY);
    sprite.drawString("C", 310, 67, 2);
    sprite.drawString("O", 310, 79, 2);
    sprite.drawString("U", 310, 91, 2);
    sprite.drawString("P", 310, 103, 2);
    sprite.drawString("U", 310, 115, 2);
    sprite.drawString("R", 310, 127, 2);
    sprite.drawString("E", 310, 139, 2);

    // Affichage valeur
    sprite.setTextDatum(MC_DATUM);
    sprite.setTextColor(TFT_LIGHTGREY);
    if (reserveEnergieSupCoup) strAjoutSepMillier(str2Display, reserveEnergieSupCoup);
    else                       strAjoutSepMillier(str2Display, reserveEnergie);
    if (nbrentier) sprite.drawString(String(str2Display) + " w", 160, 101); // Pas utile normalement, car les données sont en entier
    else           sprite.drawString(replacePointParVirgule(String(str2Display)) + " w", 160, 101);
  }

  // Rafraichissement écran (indispensable après avoir tout dessiné)
  sprite.pushSprite(0, 0);
}


/************************************************************************
 ** Permet l'affichage des jours TEMPO d'ENEDIS
 ************************************************************************/
// Zone de traçage : 316 px (car cadre gris autour)
// Tempo EDF : https://www.hellowatt.fr/fournisseurs/edf/tarif-tempo
//             https://particulier.edf.fr/fr/accueil/gestion-contrat/options/tempo.html#/selection-bp
// 
// - 22 jours rouges, répartis du 1er novembre au 31 mars, sauf les samedis et les dimanches ;
// - 43 jours blancs, répartis sur l’ensemble de l’année calendaire, sauf les dimanches ;
// - 300 jours bleus (ou 301 les années bissextiles), les dimanches sont tous des jours bleus.
// Période du 1er septembre 20xx au 31 août 20xx+1
void  AfficheEcranJoursTempo() {

  uint hauteurBarre = 0;

  ecranCourant = ECRAN_JOURS_TEMPO;

  uint year = String(dateNow2).substring(0,4).toInt();
  // Nombres Max de jours par catégorie
  uint nbJoursBleusMax = ( EstAnneeBissextile(year) ? 301 : 300 ); // 301 pour les années bissextiles, mais on ne le prend pas en compte pour simplifier
  uint nbJoursBlancsMax = 43;
  uint nbJoursRougesMax = 22;


  // Jours restants reçus de l'API Rest ENEDIS
  uint nbJoursBleusRestants  = restantsBleus.toInt();
  uint nbJoursBlancsRestants = restantsBlancs.toInt();
  uint nbJoursRougesRestants = restantsRouges.toInt();

  sprite.fillSprite(TFT_BLACK);
  sprite.fillRoundRect(0, 2, 320, 25, 0, TFT_DARKGREY);
  // Encadrement
  sprite.drawRoundRect(0, 32, 320, 138, RECT_RADIUS, TFT_LIGHTGREY);

  // Titre
  sprite.setTextDatum(MC_DATUM);
  sprite.setTextColor(TFT_WHITE);
  sprite.drawString("JOURS TEMPO RESTANTS", 123, 15, 2);
  sprite.drawString("Couleurs", 283, 15, 2);

  // Barres bleu/blanche/rouge pour chaque couleur des jours TEMPO (Y=34 à 134)
  hauteurBarre = 134 * nbJoursBleusRestants / nbJoursBleusMax;
  sprite.drawRoundRect(  2, 34, 80, 134, RECT_RADIUS, TFT_NAVY); // Cadre de la jauge (sur 2 pixels car le bleu est un peu sombre)
  sprite.drawRoundRect(  3, 35, 78, 132, RECT_RADIUS, TFT_NAVY); 
  sprite.fillRoundRect(  2, 168-hauteurBarre, 80, hauteurBarre, RECT_RADIUS, TFT_NAVY);
  hauteurBarre = 134 * nbJoursBlancsRestants / nbJoursBlancsMax;
  sprite.drawRoundRect( 84, 34, 80, 134, RECT_RADIUS, TFT_WHITE); // Cadre de la jauge
  sprite.fillRoundRect( 84, 168-hauteurBarre, 80, hauteurBarre, RECT_RADIUS, TFT_WHITE);
  hauteurBarre = 134 * nbJoursRougesRestants / nbJoursRougesMax;
  sprite.drawRoundRect(166, 34, 80, 134, RECT_RADIUS, TFT_RED); // Cadre de la jauge
  sprite.fillRoundRect(166, 168-hauteurBarre, 80, hauteurBarre, RECT_RADIUS, TFT_RED);
  // Affichage échelle max
  sprite.setTextDatum(MC_DATUM);
  sprite.setTextColor(TFT_DARKGREY);
  sprite.drawString(String(nbJoursBleusMax)  + " j",  42, 43, 2); 
  sprite.drawString(String(nbJoursBlancsMax) + " j", 124, 43, 2); 
  sprite.drawString(String(nbJoursRougesMax) + " j", 206, 43, 2); 

  // Affichage valeurs jours
  sprite.setTextDatum(MC_DATUM);
  sprite.setTextColor(TFT_LIGHTGREY);
  sprite.drawString(String(nbJoursBleusRestants)  + " j",  42, 101); 
  sprite.drawString(String(nbJoursBlancsRestants) + " j", 124, 101); 
  sprite.drawString(String(nbJoursRougesRestants) + " j", 206, 101); 
  
  
  // Affichage couleur du jour J et jour J+1. Si indéfini => gris
  // Carré pour J
  sprite.setTextColor(TFT_WHITE);
  sprite.drawString("Aujourd'hui", 283, 42, 2);
  sprite.fillRoundRect(253, 51, 60, 60, RECT_RADIUS, colorTempoJJ);
  // Carré pour J+1
  sprite.drawString("Demain", 283, 120, 2);
  sprite.fillRoundRect(263, 128, 40, 40, RECT_RADIUS, colorTempoJJ1);

  // Rafraichissement écran (indispensable après avoir tout dessiné)
  sprite.pushSprite(0, 0);
}


/***************************************************************************************
**                    Decryptage des valeurs lues dans le xml
***************************************************************************************/
void FormatageDonnesMaxPV() {

  delay(1000);

  // dataMaxPV : chaine avec toutes les valeurs recherchées
  Serial.println("Données MaxPV! reçues : '" + dataMaxPV + "'");
  // paramMaxPV : chaine avec toutes les valeurs recherchées
  if (!puissancePV or !puissanceECS) Serial.println("Paramètres MaxPV! reçus : '" + paramMaxPV + "'");
  // dataTemperature : chaine contenant une seule valeur de température (à améliorer pour gérer plusieurs valeurs). Utilisé uniquement dans le cas où l'on utilise un serveur de température hors MaxPV
  if (sondeTemperatureECS && !sondeTemperatureECSMaxPV) Serial.println("Donnée température reçue : " + dataTemperature);

  // Mise en variables des données MaxPV!, MsgDataSplit[0 à MAXPV_DATA_API_SIZE-1]
  split(MsgDataSplit, MAXPV_DATA_API_SIZE, dataMaxPV, ',');
  // DEBUG - Affichage des données de l'API MaxPV! dans le Moniteur Série
  //for(int j = 0;j<MAXPV_DATA_API_SIZE;j++) {Serial.print("Données API : "); Serial.print (j); Serial.println(" = " + MsgDataSplit[j]);}

  // Mise en variables des paramètres MaxPV!, MsgParamSplit[0 à MAXPV_PARAM_API_SIZE-1]
  split(MsgParamSplit, MAXPV_PARAM_API_SIZE, paramMaxPV, ',');
  // DEBUG - Affichage des données de l'API MaxPV! dans le Moniteur Série
  //for(int j = 0;j<MAXPV_PARAM_API_SIZE;j++) {Serial.print("Données API : "); Serial.print (j); Serial.println(" = " + MsgParamSplit[j]);}

  // Mise en variable de la température
  // Si les données reçues sont structurées, on recherche la température d'après les balises de fin et de début
  // Exemple : Shelly 1P retourne une chaine du type : {"id": 100,"tC":21.9, "tF":71.5}
  // Si on utilise la sonde de température de MaxPV, alors on récupère la température depuis les "Data" MaxPV
  if (sondeTemperatureECSMaxPV) {
    TEMPCU = MsgDataSplit[23];
  } else {
    if (baliseDebut.length() > 0 and baliseFin.length() > 0) {
      int startValue = dataTemperature.indexOf(baliseDebut);
      int endValue   = dataTemperature.indexOf(baliseFin,startValue+1);
      TEMPCU = dataTemperature.substring(startValue + baliseDebut.length(), endValue);
    } else {
      TEMPCU = dataTemperature;  // Sonde de température ECS
    }
  }


  /***********************************************
  **            MODIFICATION DES INDEX
  ************************************************/
  CO = MsgDataSplit[2];   // Consommation réseau instantanée (W) - pactive
  PV = MsgDataSplit[12];  // Production solaire instantanée PV - pimpulsion
  CU = MsgDataSplit[4];   // Routage vers la charge - prouted

  // L'affichage de la température est limité à 1 décimale
  int decimalValue = TEMPCU.indexOf("."); // Séparateur des décimales est un point
  if (decimalValue < 0) decimalValue = TEMPCU.indexOf(","); // Séparateur des décimales est une virgule
  if (decimalValue >= 0)
    TEMPCU = TEMPCU.substring(0, decimalValue + 2); // Garder une seule décimale

  TEMPCU.replace('.', ',');  // Remplace le point par une virgule

  /*Serial.println("Compteurs instantanés : ");
  Serial.println(" - Conso : " + CO);
  Serial.println(" - Production PV : " + PV);
  Serial.println(" - Routage : " + CU);
  if (sondeTemperatureECS) Serial.println(" - Température : " + TEMPCU);*/

  // Formatage des valeurs pour affichage sur l'écran
  PV = String(PV.toInt());  // (avec prod en + ou en -)
  if (temperatureEntier) TEMPCU = TEMPCU.toInt();
  //if (TEMPCU.length() < 2) TEMPCU = " " + TEMPCU; // Formatage sur 2 caractères

  // Affichage en entiers si demandé dans perso.h
  if (nbrentier) {
    CO = String(CO.toInt());
    PV = String(PV.toInt());
    CU = String(CU.toInt());
  }


  /***********************************************
  **             MODIFICATION DES CUMULS
  ************************************************/
  CUMCO = String(MsgDataSplit[9].toFloat() - MsgDataSplit[25].toFloat());    // Cumul Consommation réseau journalière (kWh) - indeximportJ
  CUMINJ = String(MsgDataSplit[10].toFloat() - MsgDataSplit[26].toFloat());  // Cumul Injection journalière (kWh) - indexexportJ
  CUMPV = String(MsgDataSplit[11].toFloat() - MsgDataSplit[27].toFloat());   // Cumul Production solaire (kWh) - indeximpulsionJ
  CUMBAL = String(MsgDataSplit[8].toFloat() - MsgDataSplit[24].toFloat());   // Cumul routage vers ballon ECS (kWh) - indexroutedJ
  /*Serial.println("Compteurs journaliers : ");
  Serial.println(" - Conso : " + CUMCO);
  Serial.println(" - Injection : " + CUMINJ);
  Serial.println(" - Production PV : " + CUMPV);
  Serial.println(" - Routage : " + CUMBAL);*/

  // Affichage en Wh si demandé dans perso.h
  if (cumulEnWh) {
    CUMCO = String(kwh_to_wh(CUMCO.toFloat()));
    CUMINJ = String(kwh_to_wh(CUMINJ.toFloat()));
    CUMPV = String(kwh_to_wh(CUMPV.toFloat()));
    CUMBAL = String(kwh_to_wh(CUMBAL.toFloat()));
  }


  /***********************************************
  **          MODIFICATION DES PARAMETRES       **
  ************************************************/
  if (!puissancePV or !puissanceECS) {          // Réalisé une seule fois, car les paramètres sont figés
    puissancePV      = MsgParamSplit[15].toInt();  // Puissance PV max en W - P_INSTALLPV
    puissanceECS = MsgParamSplit[04].toInt();  // Puissance ECS en W - P_RESISTANCE
    /*Serial.println("Paramètres : ");
    Serial.println(" - Puissance PV : " + String(puissancePV));
    Serial.println(" - Puissance ECS : " + String(puissanceECS));*/
  }

  esp_task_wdt_reset();
}


/***************************************************************************************
**              Affichage des bargraphs verticaux sur l'écran principal
***************************************************************************************/
void Indicateurs_Graphiques() {
  float valeur;
  int i, ecart, nbbarres, pmax;
  bool demie_barre;  // A true si on a déterminé qu'il fallait rajouter une demie-barre supérieure
  uint32_t selectedColor;

  //
  // Panneaux Photovoltaiques
  //
  valeur = PV.toFloat();             // PV.toInt();
  pmax = puissancePV * 9 / 10;       // On prend 90% de la puissance de crete théorique
  ecart = pmax / 9;                  // steps ecart entre min et max
  nbbarres = round(valeur / ecart);  // combien de steps dans prod en cours (en utilisant l'arrondi pour plus de précision)
  demie_barre = false;               // A true si on a déterminé qu'il fallait rajouter une demie-barre supérieure

  // Test avec couleurs variant de rouge (basse production) à vert (forte production) en fonction de la puissance produite
  if (valeur > 0)  // Affichage seulement s'il y a de la production
  {
    for (i = 0; i < 9; i++) sprite.fillRect(200, (46 - (i * 5)), 20, 4, color0);  // Réinitialisation affichage

    // Optimisation par rapport à la demi-valeur
    // Car si nbbarres_sans_arrondi=2,4 alors on affiche 2 barres. Il faudrait afficher la demi-barre basse entre la 2 et la 3, pour simuler 2,5 barres
    //     si nbbarres_sans_arrondi=2,8 alors on affiche 3 barres. On affiche 3 barres pleines
    // si valeur >  xxx,5 : demi-barre haute colorée et demi-barre basse colorée
    // si valeur <= xxx,5 : demi-barre haute sombre  et demi-barre basse colorée
    if (nbbarres * ecart < valeur && nbbarres < 9) {  // La demi-barre supérieure doit être colorée
      demie_barre = true;
      nbbarres += 1;
    }
    if (nbbarres > 9) nbbarres = 9;  // on bloque les steps à 9

    if      (nbbarres > 6) selectedColor = TFT_GREEN;  // 7, 8 et 9 barres (Vert)
    else if (nbbarres > 3) selectedColor = TFT_YELLOW;  // 4, 5, 6 barres (Jaune)
    else if (nbbarres > 1) selectedColor = TFT_ORANGE;  // 2 et 3 barres (Orange)
    else if (nbbarres > 0) selectedColor = TFT_RED;  // 1 barre (Rouge)

    for (i = 0; i < nbbarres; i++) sprite.fillRect(200, (46 - (i * 5)), 20, 4, selectedColor);

    if (demie_barre)  // Suppression de la zone trop remplie ;)
      sprite.fillRect(200, (46 - (nbbarres - 1) * 5), 20, 2, color0);

    /*if (nbbarres >= 7) // Petit tuning : on affiche un trait sur le haut pour visualiser où se trouve la limite max
      sprite.drawLine(200,6,219,6,selectedColor);*/
  }


  //
  // Routage vers ECS
  //
  valeur = CU.toFloat();  //CU.toInt();  // Puissance routée vers ECS

  ecart = puissanceECS / 9;      // Puissance correspondante pour chacune des 9 barres (par rapport à puissance max du ECS)
  nbbarres = round(valeur / ecart);  // Calcul nombre de barres à afficher en couleur (avec l'arrondi pour plus de précision)
  demie_barre = false;               // A true si on a déterminé qu'il fallait rajouter une demie-barre supérieure

  for (i = 0; i < 9; i++) sprite.fillRect(200, (102 - (i * 5)), 20, 4, color0);  // Réinitialisation couleur sombre, du bas vers le haut
  if (nbbarres * ecart < valeur && nbbarres < 9) {                               // La demi-barre supérieure doit être colorée
    demie_barre = true;
    nbbarres += 1;
  }
  if (nbbarres > 9) nbbarres = 9;

  // Couleurs variant de rouge (faible routage) à vert (fort routage) en fonction de la puissance routée
  if      (nbbarres > 5) selectedColor = TFT_GREEN;  // 6, 7 et 8 barres (Vert)
  else if (nbbarres > 3) selectedColor = TFT_YELLOW;  // 4 et 5 barres (Jaune)
  else if (nbbarres > 1) selectedColor = TFT_ORANGE;  // 2 et 3 barres (Orange)
  else if (nbbarres > 0) selectedColor = TFT_RED;  // 1 barre (Rouge)

  for (i = 0; i < nbbarres; i++) sprite.fillRect(200, (102 - (i * 5)), 20, 4, selectedColor);

  if (demie_barre)  // Suppression de la zone trop remplie ;)
    sprite.fillRect(200, (102 - (nbbarres - 1) * 5), 20, 2, color0);

  /*if (nbbarres >= 8) // Petit tuning : on affiche un trait sur le haut pour visualiser où se trouve la limite max
    sprite.drawLine(200,62,219,62,selectedColor);*/


  //
  // Consommation Réseau (importation)
  //
  valeur = CO.toFloat();  //CO.toInt();

  for (i = 0; i < 9; i++) sprite.fillRect(200, (158 - (i * 5)), 20, 4, color0);  // Réinitialisation couleur sombre, du bas vers le haut

  if (valeur > 0)  // Cas de la consommation depuis le réseau
  {
    ecart = 4000 / 9;                                 // Conso max arbitraire 4000W => step 500W
    nbbarres = round(valeur / ecart);                 // Calcul nombre de barres à afficher en couleur (avec l'arrondi pour plus de précision)
    demie_barre = false;                              // A true si on a déterminé qu'il fallait rajouter une demie-barre supérieure

    if (nbbarres * ecart < valeur && nbbarres < 9) {  // La demi-barre supérieure doit être colorée
      demie_barre = true;
      nbbarres += 1;
    }
    if (nbbarres > 9) nbbarres = 9;

    // Couleurs variant de vert (faible consommation) à rouge (forte consommation) en fonction de la puissance consommée
    if      (nbbarres > 6) selectedColor = TFT_RED;  // 3500W et plus - 7, 8 et 9 barres
    else if (nbbarres > 4) selectedColor = TFT_ORANGE;  // 2500W à 3499W - 5 et 6 barres
    else if (nbbarres > 2) selectedColor = TFT_YELLOW;  // 1500W à 2499W - 3 et 4 barres
    else if (nbbarres > 0) selectedColor = TFT_GREEN;  //  500W à 1499W - 1 et 2 barres

    for (i = 0; i < nbbarres; i++) sprite.fillRect(200, (160 - (i * 5)), 20, 4, selectedColor);

    if (demie_barre)  // Suppression de la zone trop remplie ;)
      sprite.fillRect(200, (160 - (nbbarres - 1) * 5), 20, 2, color0);

    /*if (nbbarres >= 7) // Petit tuning : on affiche un trait sur le haut pour visualiser où se trouve la limite max
      sprite.drawLine(200,120,219,120,selectedColor);*/

  } else if (valeur < 0)  // Cas de l'injection vers le réseau
  {
    valeur = abs(valeur);  // on enlève le signe

    // Cas 1 : Le bargraph d'injection max est étalonné en fonction de la production max théorique
    // On garde pmax de la production solaire pour évaluer l'injection max possible
    //ecart = pmax / 8;
    // Cas 2 : Le bargraph d'injection max est étalonné en fonction de la production courante
    // Ppermet de visualiser facilement si on injecte 25%, 50%, 90% de la production
    ecart = PV.toInt() / 9;

    nbbarres = round(valeur / ecart);  // Avec l'arrondi pour plus de précision
    demie_barre = false;               // A true si on a déterminé qu'il fallait rajouter une demie-barre supérieure

    if (nbbarres * ecart < valeur && nbbarres < 9) {  // La demi-barre supérieure doit être colorée
      demie_barre = true;
      nbbarres += 1;
    }
    if (nbbarres > 9) nbbarres = 9;  // on bloque les steps à 9

    if (nbbarres > 0)
      for (i = 0; i < nbbarres; i++) sprite.fillRect(200, (160 - (i * 5)), 20, 4, TFT_WHITE);  // (Blanc)

    if (demie_barre)  // Suppression de la zone trop remplie ;)
      sprite.fillRect(200, (160 - (nbbarres - 1) * 5), 20, 2, color0);

    /*if (nbbarres >= 7) // Petit tuning : on affiche un trait sur le haut pour visualiser où se trouve la limite max
      sprite.drawLine(200,120,219,120,TFT_WHITE);*/
  }
}



/***************************************************************************************
**      Fait varier l'intensité d'éclairage de 50 à 250 dans un sens et l'inverse
***************************************************************************************/
void Affichage_Eclairage() {
  if (!inverse) {
    dim = dim - 50;
    if (dim <= 50) {dim = 50; inverse = true;}
  }
  else
  {
    dim = dim + 50;
    if (dim >= 250) {dim = 250; inverse = false;}
  }
  delay(300);

  // Affichage en sur-impression au centre de l'écran, le réglage de la luminosité
  // Cet écran s'efface après une rafraichissement de l'écran (lors de la lecture des données)
  luminosite.setTextDatum(MC_DATUM);  // Alignement texte au centre du rectangle le contenant
  luminosite.setTextColor(TFT_WHITE);
  luminosite.fillRoundRect(0, 0, 200, 90, RECT_RADIUS, TFT_DARKGREY);
  luminosite.drawString("Luminosite " + String(dim * 2 / 5) + "%", 100, 15, 2);            // Affichage du % de dim par rapport au max de 250 (=> 2/5)
  x = dim / 50;                                                                            // steps de 50
  for (int i = 0; i < x; i++)     luminosite.fillRect(25 + (i * 30), 35, 25, 35, TFT_GOLD);// La zone sélectionnée
  for (int i = 0 + x; i < 5; i++) luminosite.fillRect(25 + (i * 30), 35, 25, 35, color0);  // La zone non sélectionnée
  
  luminosite.pushToSprite(&sprite, 60, 40, TFT_BLACK);

  // Réglage de la luminosité
  ledcWrite(ledChannel, dim);

  // Rafraichissement de l'écran
  sprite.pushSprite(0, 0);
}


/***************************************************************************************
**                            Gestion du signal Wifi
***************************************************************************************/
void AffichageSignalWifi() {

  // Etat du signal WiFi
  RSSI = String(WiFi.RSSI());
  Serial.println("\nForce du signal (RSSI): " + RSSI + " dB");

  if (RSSI.toInt() <= -95) {  // Pas de signal, arc rouge plein!
    wifi.drawArc(27, 0, 27, 0, 0, 90, TFT_RED, TFT_TRANSPARENT);
    wifi.setTextDatum(MC_DATUM);
    wifi.setTextColor(TFT_BLACK);
    wifi.drawString("?", 17, 14, 4);
  } else if (RSSI.toInt() <= -85) {                                       // Signal faible
    wifi.drawArc(27, 0, 27, 0, 0, 90, TFT_TRANSPARENT, TFT_TRANSPARENT);  // Réinitialiser couleur de fond
    wifi.drawSmoothArc(27, 0, 13, 10, 0, 90, TFT_ORANGE, TFT_TRANSPARENT,true);
    wifi.drawSmoothArc(27, 0, 6, 0, 0, 90, TFT_ORANGE, TFT_TRANSPARENT,true);
  } else if (RSSI.toInt() <= -80) {                                       // Signal moyen
    wifi.drawArc(27, 0, 27, 0, 0, 90, TFT_TRANSPARENT, TFT_TRANSPARENT);  // Réinitialiser couleur de fond
    wifi.drawSmoothArc(27, 0, 20, 17, 0, 90, TFT_GOLD, TFT_TRANSPARENT,true);
    wifi.drawSmoothArc(27, 0, 13, 10, 0, 90, TFT_GOLD, TFT_TRANSPARENT,true);
    wifi.drawSmoothArc(27, 0, 6, 0, 0, 90, TFT_GOLD, TFT_TRANSPARENT,true);
  } else if (RSSI.toInt() <= -75) {                                       // Très bon signal
    wifi.drawArc(27, 0, 27, 0, 0, 90, TFT_TRANSPARENT, TFT_TRANSPARENT);  // Réinitialiser couleur de fond
    //wifi.drawArc(27, 0, 27, 24, 0, 90, TFT_GREEN, TFT_TRANSPARENT);
    wifi.drawSmoothArc(27, 0, 20, 17, 0, 90, TFT_GREEN, TFT_TRANSPARENT,true);
    wifi.drawSmoothArc(27, 0, 13, 10, 0, 90, TFT_GREEN, TFT_TRANSPARENT,true);
    wifi.drawSmoothArc(27, 0, 6, 0, 0, 90, TFT_GREEN, TFT_TRANSPARENT,true);
  } else {
    wifi.drawArc(27, 0, 27, 0, 0, 90, TFT_TRANSPARENT, TFT_TRANSPARENT);  // Réinitialiser couleur de fond
    wifi.drawSmoothArc(27, 0, 27, 24, 0, 90, TFT_GREEN, TFT_TRANSPARENT,true);
    wifi.drawSmoothArc(27, 0, 20, 17, 0, 90, TFT_GREEN, TFT_TRANSPARENT,true);
    wifi.drawSmoothArc(27, 0, 13, 10, 0, 90, TFT_GREEN, TFT_TRANSPARENT,true);
    wifi.drawSmoothArc(27, 0, 6, 0, 0, 90, TFT_GREEN, TFT_TRANSPARENT,true);
  }

  wifi.pushToSprite(&sprite, 293, 60, TFT_TRANSPARENT);
}



/***************************************************************************************
**                            Gestion de la batterie
***************************************************************************************/
void AffichageIndicateurBatterie() {

  uint32_t selectedColor;

  // Calcul % de charge de la batterie
  // BAT_VAL_MAX : 4000mv pour charge 100% (estimation)
  // BAT_VAL_MIN : 2700mv pour charge 0% (estimation)
  pourcentageBatterie = (tensionBatterie - BAT_VAL_MIN) * 100 / (BAT_VAL_MAX - BAT_VAL_MIN);

  // Pour le cas où l'arduino resterait allumé même si la batterie est en dessous du seuil "mini"
  if (pourcentageBatterie < 0) pourcentageBatterie = 0;

  // Calibrage batterie : à décommenter pour visualiser la tension courante de la batterie
  //AfficheDebugTFT(String(tensionBatterie) + "mV " + String(pourcentageBatterie) + "%");
  Serial.println("Batterie: " + String(tensionBatterie) + "mV " + String(pourcentageBatterie) + "%");


  // Détermination de la couleur interne de la pile
  if      (pourcentageBatterie < 15)   selectedColor = TFT_RED;
  else if (pourcentageBatterie < 31 )  selectedColor = TFT_ORANGE;
  else if (pourcentageBatterie < 110 ) selectedColor = TFT_GREEN;
  // Batterie en charge : % > 110 quand USB branché (correspond à > 4300mV)
  else                                 selectedColor = TFT_WHITE;
  
  // Réinitialisation du fond de la batterie
  batterie.fillRect(0, 0, 15, 30, TFT_BLACK);

  // Contour de la pile
  batterie.drawRoundRect(0, 5, 15, 25, 1, TFT_LIGHTGREY);  // Bas de la pile
  batterie.drawRect(3, 0, 9, 6, TFT_LIGHTGREY);            // Haut de la pile
  batterie.drawLine(4, 5, 10, 5, TFT_BLACK);               // Suppression trait de jonction des carrés haut et bas

  if (pourcentageBatterie > 100) pourcentageBatterie = 100; // Protection en cas de charge USB

  // Contenu de la pile
  //  26 lignes colorisables quand batterie à 100%
  //    Partie fine (haute) : 5 lignes
  //    Partie large (basse): 21 lignes
  int nbrLignes = pourcentageBatterie * 26 / 100;
  int nbrLignesInit = nbrLignes;

  if (nbrLignes > 21 && nbrLignes <= 26 ) {
    // Remplissage du haut de la batterie
    batterie.fillRect(5, 7-(nbrLignes-21), 5, (nbrLignes-21), selectedColor);    // Partie haute
    // Remplissage complet du bas de la batterie
    nbrLignes = 21;
  } 
  if (nbrLignes <= 21 ) {
    // Remplissage du bas de la batterie
    batterie.fillRect(2, 28-nbrLignes, 11, nbrLignes, selectedColor);
  } 
  /*if (pourcentageBatterie < 10) {
    // On rajoute un "!" d'avertissement
    batterie.setTextDatum(ML_DATUM);
    batterie.setTextColor(TFT_RED);
    batterie.drawString("!", 4, 18, 4);
  }*/
  // Affichage du % sur la batterie
  batterie.setTextDatum(MC_DATUM);
  // La valeur est affichée sur la partie "pleine" du bas. Sauf si trop petite : lors en haut
  if (nbrLignesInit != 26) { // Pas à 100%
    if (nbrLignesInit > 10) {
      batterie.setTextColor(TFT_BLACK);
      batterie.drawString(String(pourcentageBatterie), 8, 28-nbrLignesInit/2); // 28 = 2px bordure + 26 lignes max
    } else {
      batterie.setTextColor(selectedColor);
      batterie.drawString(String(pourcentageBatterie), 8, 7+(21-nbrLignesInit)/2); // 7 = 2px bordure + lignes du haut inutilisables
    }
  }
  
  batterie.pushToSprite(&sprite, 303, 92, TFT_BLACK);
}



/***************************************************************************************
**                           Réception des données météo
**                 Valeurs issues de Open Weather (Gestion Forecast)
***************************************************************************************/
void RecuperationDonneesMeteo() {
  // Valeurs issues de Open Weather (Gestion Forecast)
  OW_forecast *forecast = new OW_forecast;
  ow.getForecast(forecast, api_key, latitude, longitude, units, language);

  /******
  Serial.println("");
  Serial.println("#####  Données météo  #####");
  Serial.println("MAX_3HRS:"+String(MAX_3HRS) + " MAX_DAYS:"+String(MAX_DAYS));
  
  Serial.print("Latitude         : "); Serial.println(ow.lat);
  Serial.print("Longitude        : "); Serial.println(ow.lon);
  Serial.print("Timezone         : "); Serial.println(forecast->timezone);
  Serial.print("City Name        : "); Serial.println(forecast->city_name);
  Serial.print("Heure actuelle   : "); Serial.println(strTime(forecast->dt[0]));
  Serial.print("Lever soleil     : "); Serial.println(strTime(forecast->sunrise));
  Serial.print("Coucher soleil   : "); Serial.println(strTime(forecast->sunset));
  Serial.print("Température °    : "); Serial.println(forecast->temp[0]);
  Serial.print("Humidité         : "); Serial.println(forecast->humidity[0]);
  Serial.print("Description EN   : "); Serial.println(forecast->main[0]);
  Serial.print("Description      : "); Serial.println(forecast->description[0]);
  Serial.print("Vitesse vent m/s : "); Serial.println(forecast->wind_speed[0]);
  int windAngle = (forecast->wind_deg[0] + 22.5) / 45;
  if (windAngle > 7) windAngle = 0;
  String wind[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW" };
  Serial.print("Angle vent       : "); Serial.println(forecast->wind_deg[0] + " -->" + wind[windAngle]);
  Serial.print("Pression hPA     : "); Serial.println(forecast->pressure[0]);
  Serial.print("Icone            : "); Serial.println(forecast->icon[0]);
  Serial.print("ID               : "); Serial.println(forecast->id[0]);
  Serial.print("Heure +3         : "); Serial.println(strTime(forecast->dt[1]));
  Serial.print("Heure +6         : "); Serial.println(strTime(forecast->dt[2]));
  Serial.print("Heure +9         : "); Serial.println(strTime(forecast->dt[3]));
  Serial.print("Description +3   : "); Serial.println(forecast->description[1]);
  Serial.print("Description +6   : "); Serial.println(forecast->description[2]);
  Serial.print("Description +9   : "); Serial.println(forecast->description[3]);
  Serial.print("Description +12  : "); Serial.println(forecast->description[4]);
  Serial.print("Description +15  : "); Serial.println(forecast->description[5]);
  *******/
  
  lever   = strTime(forecast->sunrise);
  coucher = strTime(forecast->sunset);
  tempExt = String(forecast->temp[0], 0);             // Température sans décimale
  if (tempExt.length() < 2) tempExt = " " + tempExt;  // et sur 2 caractères
  humExt = forecast->humidity[0];
  icone  = forecast->icon[0];
  ID     = forecast->id[0];
  if (poisson) {
    if (wink) icone = "80d";
  } 

  // Effacement des chaines pour libérer la mémoire
  delete forecast;
  esp_task_wdt_reset();
}



/***************************************************************************************
**                         Relancement du cycle de lecture
***************************************************************************************/
void RecuperationDonneesMaxPV() {

  if (!premiereLectureDonneesMaxPV) { // Pour éviter d'afficher un écran noir après le premier démarrage
    // Affichage indicateur de rafraichissement des données : démarrage
    sprite.drawSmoothCircle(8, 8, 4, TFT_GREENYELLOW, TFT_TRANSPARENT); 
    sprite.pushSprite(0, 0);
    }

  /****************************************************
   * Etape 1 : récupération des données depuis MaxPV! *
   ****************************************************/
  // Use WiFiClient class to create TLS connection
  Serial.println("\nE1 : Initialisation de la connexion au serveur MaxPV!...");
  // Connexion au serveur web
  Serial.print("E1 : Connexion à MaxPV! : ");
  Serial.print(serveurMaxPV + String(":") + portServeurMaxPV);
  Serial.println(pathData);

  if (!client.connect(serveurMaxPV, portServeurMaxPV)) {
    Serial.println("E1 : Connexion échouée!");
    return;
  }

  // Make a HTTP request:
  client.print("GET ");
  client.print(pathData);
  client.println(" HTTP/1.1");
  client.print("Host: ");
  client.println(serveurMaxPV);
  client.println();

  while (!client.available())
    ;  //wait for client data to be available
  Serial.println("E1 : Attente de la réponse serveur...");
  delay(5);

  while (client.available()) {
    // Retourne toutes les lignes de la réponse, mais on ne garde que la dernière!
    // Exemple de réponse :
    //  HTTP/1.1 200 OK\n
    //  Content-Length: 170\n
    //  Content-Type: text/plain\n
    //  Connection: close\n
    //  Accept-Ranges: none\n
    //  \n
    //  224.7,1.208,250.9,271.4,0.0,250.9,0.0,0.9244,463.705,458.300,195.505,861.991,0.0,9,9,N/A,N/A,N/A,2.508,00000000,11:03:58:19,8327,7478,457.025,454.350,189.535,845.764,7478
    String line = client.readStringUntil('\n');
    dataString = line;  // Mise en mémoire de la chaine de données reçues de MaxPV! (/api/get?alldata)
  }
  dataMaxPV = dataString;  // Sauvegarde des données reçues de MaxPV!

  Serial.println("E1 : Requête traitée avec succès!");
  client.stop();
  esp_task_wdt_reset();


  /*****************************************************
   * Etape 2 : récupération des paramètres de MaxPV    *
   *****************************************************/
  // Effectué une seule fois pour alimenter ces paramètres (ces paramètres sont fixes)
  if (!puissancePV || !puissanceECS) {
    Serial.print("E2 : Connexion à MaxPV! : ");
    Serial.print(serveurMaxPV + String(":") + portServeurMaxPV);
    Serial.println(pathParam);

    if (!client.connect(serveurMaxPV, portServeurMaxPV)) {
      Serial.println("E2 : Connexion échouée!");
      return;
    }

    // Make a HTTP request:
    client.print("GET ");
    client.print(pathParam);
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(serveurMaxPV);
    client.println();

    while (!client.available())
      ;  //wait for client data to be available
    Serial.println("E2 : Attente de la réponse serveur...");
    delay(5);

    while (client.available()) {
      String line = client.readStringUntil('\n');
      dataString = line;  // Mise en mémoire de la chaine de données reçues de MaxPV! (/api/get?alldata)
    }
    paramMaxPV = dataString;  // Sauvegarde des données reçues de MaxPV!

    Serial.println("E2 : Requête traitée avec succès!");
    client.stop();
    esp_task_wdt_reset();
  }


  /*****************************************************
   * Etape 3 : récupération des données de température *
   *****************************************************/
  if (sondeTemperatureECS && !sondeTemperatureECSMaxPV) {
    // Use WiFiClient class to create TLS connection
    Serial.println("\nE3 : Initialisation de la connexion au serveur température...");
    // Connexion au serveur web
    Serial.print("E3 : Connexion à l'API température : ");
    Serial.print(serveurTemperatureECS + String(":") + portServeurTemperatureECS);
    Serial.println(pathTemperatureECS);

    if (!client.connect(serveurTemperatureECS, portServeurTemperatureECS)) {
      Serial.println("E3 : Connexion échouée!");
      return;
    }

    // Make a HTTP request:
    client.print("GET ");
    client.print(pathTemperatureECS);
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(serveurTemperatureECS);
    client.println();

    while (!client.available())
      ;  //wait for client data to be available
    Serial.println("E3 : Attente de la réponse serveur...");
    delay(5);

    while (client.available()) {
      String line = client.readStringUntil('\n');
      dataString = line;  // Mise en mémoire de la chaine de données reçues de l'API de température
    }
    dataTemperature = dataString;  // Sauvegarde des données reçues du serveur de température

    Serial.println("E3 : Requête traitée avec succès!");
    client.stop();
    esp_task_wdt_reset();
  }

  if (!premiereLectureDonneesMaxPV) {
    // Indicateur de rafraichissement des données => statut terminé (il sera effacé lors du rafraichissement de l'écran)
    sprite.fillSmoothCircle(8, 8, 4, TFT_GREENYELLOW);
    sprite.pushSprite(0, 0);
  }
  awaitingArrivals = false;
  premiereLectureDonneesMaxPV = false;
}


void resetCycle() {
  awaitingArrivals = true;
  arrivalsRequested = false;
  Serial.println("Rafraichissement de l'affichage...");
}


/***************************************************************************************
**                      Serveur web (idée et conception Bellule)
***************************************************************************************/
void serveurweb() {
  // Activation de la fonction serveur Web (superbe idée de Bellule)
  // Ceci permet une lecture sur un téléphone par exemple mais aussi
  // à distance si l'adresse du companion est fixe. Commencez par
  // vous connecter sur l'adresse affichée sur l'écran d'accueil
  WiFiClient clientweb = server.available();  // Listen for incoming clients

  if (clientweb) {  // If a new client connects,
    Serial.println("");
    Serial.println("Nouveau client.");  // print a message out in the serial port
    String currentLine = "";            // make a String to hold incoming data from the client
    currentTime = millis();
    previousTime = currentTime;
    while (clientweb.connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
      currentTime = millis();
      if (clientweb.available()) {  // if there's bytes to read from the client,
        char c = clientweb.read();  // read a byte, then
        Serial.write(c);            // print it out the serial monitor
        header += c;
        if (c == '\n') {  // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            clientweb.println("HTTP/1.1 200 OK");
            clientweb.println("Content-type:text/html");
            clientweb.println("Connection: close");
            clientweb.println();

            clientweb.println("<!DOCTYPE html><html>");
            clientweb.println("<html lang=\"fr\">");
            clientweb.println("<head>");
            clientweb.println("<meta charset=\"UTF-8\" />");
            clientweb.println("<title>MaxPV! Companion</title>");
            clientweb.println("<link rel=\"stylesheet\" href=\"https://www.w3schools.com/w3css/4/w3.css\">");
            clientweb.println("<link rel=\"stylesheet\" href=\"https://fonts.googleapis.com/css?family=Allerta+Stencil\">");
            // Utilisation du favicon de MaxPV!
            clientweb.println("<link rel=\"icon\" type=\"image/x-icon\" href=\"http://" + String(serveurMaxPV) + "/favicon.ico\">");
            clientweb.println("<script src=\"https://code.jquery.com/jquery-3.6.4.js\" integrity=\"sha256-a9jBBRygX1Bh5lt8GZjXDzyOB+bWve9EiO7tROUtj/E=\" crossorigin=\"anonymous\"></script>");

            clientweb.println("<script>");
            clientweb.println("$( document ).ready(function() {");
            clientweb.println("$('#div_refresh').load(document.URL +  ' #div_refresh');");
            clientweb.println("setInterval(function() {");
            clientweb.println("$('#div_refresh').load(document.URL +  ' #div_refresh');");
            clientweb.println("},5000);");
            clientweb.println("});");
            clientweb.println("</script>");

            clientweb.println("</head>");
            clientweb.println("<body>");
            //script
            clientweb.println("<script>");
            clientweb.println("function toggleBottom() {");
            clientweb.println("var bottomDiv = document.getElementById(\"bottom\");");
            clientweb.println("if (bottomDiv.style.display === \"none\") {");
            clientweb.println("bottomDiv.style.display = \"block\";");
            clientweb.println("} else {");
            clientweb.println("bottomDiv.style.display = \"none\";");
            clientweb.println("}");
            clientweb.println("}");
            clientweb.println("</script>");

            // Web Page Heading
            clientweb.println("<div class= \"w3-container w3-black w3-center w3-allerta\">");
            clientweb.println("<h1>MaxPV! Companion v" + Version + "</h1>");
            clientweb.println("</div>");
            clientweb.println("<div id=\"div_refresh\">");

            // <<<<<<<<<<<<<<<<<<<<<<<< Affichage des données MaxPV!  >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
            clientweb.println("<div class=\"w3-card-4 w3-teal w3-padding-16 w3-xxxlarge w3-center\">");
            clientweb.println("<p>Puissance produite</p>");
            clientweb.print(replacePointParVirgule(PV));  // Valeur Panneaux Photovoltaiques
            clientweb.println(" W");
            clientweb.println("</div>");

            clientweb.println("<div class=\"w3-card-4 w3-blue-grey w3-padding-16 w3-xxxlarge w3-center\">");
            clientweb.println("<p>Puissance routée</p>");
            clientweb.print(replacePointParVirgule(CU));  // Valeur Recharge ECS
            clientweb.println(" W");
            clientweb.println("</div>");

            clientweb.println("<div class=\"w3-card-4 w3-light-grey w3-padding-16 w3-xxxlarge w3-center\">");
            clientweb.println("<p>Puissance active</p>");
            clientweb.print(replacePointParVirgule(CO));  // Valeur Consommation EDF
            clientweb.println(" W");
            clientweb.println("</div>");

            if (sondeTemperatureECS) {
              clientweb.println("<div class=\"w3-card-4 w3-pale-blue w3-padding-16 w3-xxxlarge w3-center\">");
              clientweb.println("<p>Température ECS</p>");
              clientweb.print(replacePointParVirgule(TEMPCU));  // Valeur Température ECS
              clientweb.println(" °C");
              clientweb.println("</div>");
            }

            clientweb.println("</div>");

            clientweb.println("<center>");
            clientweb.println("<button class=\"w3-bar w3-deep-orange w3-button w3-padding-16 w3-xxlarge w3-hover-orange\" onclick=\"toggleBottom()\">Informations journalières (Afficher/Masquer)</button>");
            clientweb.println("</center>");
            clientweb.println("<div id=\"bottom\" style=\"display:none;\">");

            clientweb.println("<div class=\"w3-card-4  w3-teal w3-padding-16 w3-xxxlarge w3-center\">");
            clientweb.println("<p>Energie produite</p>");
            clientweb.print(replacePointParVirgule(CUMPV));  // Valeur cumul recharge ECS
            if (cumulEnWh) clientweb.println(" Wh");
            else clientweb.println(" kWh");
            clientweb.println("</div>");

            clientweb.println("<div class=\"w3-card-4 w3-blue-grey w3-padding-16 w3-xxxlarge w3-center\">");
            clientweb.println("<p>Energie routée</p>");
            clientweb.print(replacePointParVirgule(CUMBAL));  // Cumul Consommation EDF
            if (cumulEnWh) clientweb.println(" Wh");
            else clientweb.println(" kWh");
            clientweb.println("</div>");

            clientweb.println("<div class=\"w3-card-4 w3-light-grey w3-padding-16 w3-xxxlarge w3-center\">");
            clientweb.println("<p>Energie importée</p>");
            clientweb.print(replacePointParVirgule(CUMCO));  // Cumul Panneaux Photovoltaiques
            if (cumulEnWh) clientweb.println(" Wh");
            else clientweb.println(" kWh");
            clientweb.println("</div>");

            clientweb.println("<div class=\"w3-card-4 w3-pale-blue w3-padding-16 w3-xxxlarge w3-center\">");
            clientweb.println("<p>Energie Exportée</p>");
            clientweb.print(replacePointParVirgule(CUMINJ));  // Cumul Injection EDF
            if (cumulEnWh) clientweb.println(" Wh");
            else clientweb.println(" kWh");
            clientweb.println("</div>");
            // <<<<<<<<<<<<<<<<<<<<<<<< Fin affichage des données MaxPV!  >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
            clientweb.println("</div>");
            clientweb.println("</body></html>");
            // The HTTP response ends with another blank line
            clientweb.println();
            // Break out of the while loop
            break;
          } else {  // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    clientweb.stop();
    Serial.println("Client déconnecté.");
    Serial.println("");
    esp_task_wdt_reset();
  }
}


/***************************************************************************************
**         Découpe la partie du xml voulue en autant de valeurs que trouvées
***************************************************************************************/
void split(String *vecSplit, int dimArray, String content, char separator) {
  if (content.length() == 0)
    return;

  content = content + separator;
  int countVec = 0;
  int posSep = 0;
  int posInit = 0;
  while (countVec < dimArray) {
    posSep = content.indexOf(separator, posSep);
    if (posSep < 0) return;
    String splitStr = content.substring(posInit, posSep);
    posSep = posSep + 1;
    posInit = posSep;
    vecSplit[countVec] = splitStr;
    countVec++;
  }
}


/***************************************************************************************
** Conversion des points par des virgules dans une chaine pour affichage de nombres
***************************************************************************************/
String replacePointParVirgule(String txtIn) {
  String txtOut = txtIn;     // On ne fait pas la modif sur la chaine originale, car sinon pb en cas de conversion vers float()
  txtOut.replace('.', ',');  // Remplace le point par une virgule
  return txtOut;
}


/***************************************************************************************
**             Conversion Unix time vers "time" time string "12:34"
***************************************************************************************/
String strTime(time_t unixTime) {
  String localTime = "";
  time_t local_time = TIMEZONE.toLocal(unixTime, &tz1_Code);

  if (hour(local_time) < 10) localTime += "0";
  localTime += hour(local_time);
  localTime += ":";

  if (minute(local_time) < 10) localTime += "0";
  localTime += minute(local_time);

  return localTime;
}

/***************************************************************************************
**                        Lecture de l'heure et Date
***************************************************************************************/
void GetTimeDate() {
  // Convert UTC to local time, returns zone code in tz1_Code, e.g "GMT"
  time_t local_time = TIMEZONE.toLocal(now(), &tz1_Code);

  timeNow = "";
  if (hour(local_time) < 10) timeNow += "0";
  timeNow += hour(local_time);
  timeNow += ":";
  if (minute(local_time) < 10) timeNow += "0";
  timeNow += minute(local_time);

  // Format : numéro du jour + mois en abrégé
  dateNow = "";
  dateNow += day(local_time);
  dateNow += " ";
  dateNow += String(Months[month(local_time)]);

  //Serial.println("Date et heure locale : " + dateNow + " " + timeNow);

  // Format : AAAA-MM-JJ
  dateNow2 = "";
  dateNow2 += year(local_time);
  dateNow2 += "-";
  dateNow2 += month(local_time);
  dateNow2 += "-";
  dateNow2 += day(local_time);

  //Serial.println("Date et heure locale 2 : " + dateNow2);

  if (poisson) {if (dateNow == "1 Avril") wink = true;}
}


/***************************************************************************************
**                        Lecture tension de la batterie
***************************************************************************************/
void GetBatteryVoltage() {

  // Documentation :
  // https://github.com/Xinyuan-LilyGO/T-Display-S3
  // https://github.com/Xinyuan-LilyGO/T-Display-S3/blob/main/examples/GetBatteryVoltage/GetBatteryVoltage.ino
  // https://docs.espressif.com/projects/esp-idf/en/release-v3.3/api-reference/peripherals/adc.html
  // https://github.com/Xinyuan-LilyGO/TTGO-T-Display/blob/master/TFT_eSPI/examples/FactoryTest/FactoryTest.ino
  esp_adc_cal_characteristics_t adc_chars;

  // Get the internal calibration value of the chip
  // "1100" is actually 1.1V in mV. 1.1V is the internal reference voltage that readings get compared to.
  esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
  uint32_t raw = analogRead(PIN_BAT_VOLT);
  tensionBatterie = esp_adc_cal_raw_to_voltage(raw, &adc_chars) * 2; //The partial pressure is one-half

  // Bizarre, car ce calcul semblerait plus juste quand on compare avec un Voltmètre. Bon... juste, dans certains cas.
  //float battery_voltage = ((float)raw / 4095.0) * 2.0 * 3.3 * 1100;

  // If the battery is not connected, the ADC detects the charging voltage of TP4056, which is inaccurate.
  // Can judge whether it is greater than 4300mV. If it is less than this value, it means the battery exists.
  /*if (tensionBatterie > 4300) {
      AfficheDebugTFT ("No battery! U:" + String(tensionBatterie) + "mV");
  } else {
      AfficheDebugTFT ("U: " + String(tensionBatterie) + "mV");
  }*/
  // Message débug pour afficher tension de la batterie
  //AfficheDebugTFT (String(tensionBatterie) + "mV " + String(pourcentageBatterie) + "%");
}

/*************************
**  Gestion des boutons 
**************************/
// Simple clic gauche : Modification intensité lumineuse sous forme va & vient. Ou affichage écran 5s pendant la veille
void handleClickGauche() {
  if (veille and PV.toInt() <= 0) {  // Si on clique bouton, veille annulée momentanément
    dim = luminositeChoisie;

    // Réglage de la luminosité
    ledcWrite(ledChannel, dim);

    delay(5000);  // affichage pendant 5 secondes
  }

  Affichage_Eclairage();
}

// Double clic gauche : sortie de la veille
void handleDoubleClickGauche() {
  if (veille) {
    veille = false;
    dim = luminositeChoisie;

    // Réglage de la luminosité
    ledcWrite(ledChannel, dim);
  }

  //Affichage_Eclairage();
}

/******************************************
 ** Gestion bouton de changement d'écran **
 ******************************************/
// Simple clic droit : passage à l'écran suivant
void handleClickDroit() {

  AfficheEcran(REGLE_ECRAN_SUIVANT);
}

// Double clic droit : retour à l'écran principal
void handleDoubleClickDroit() {

  AfficheEcran(REGLE_ECRAN_PRINCIPAL);
}


/****************************************
 ** Gestion ordre affichage des écrans **
 ****************************************/
void AfficheEcran (uint regle) {

  if (regle == REGLE_ECRAN_PRINCIPAL) { // Afficher l'écran principal

    AfficheEcranPrincipal();


  } else if (regle == REGLE_MEME_ECRAN) { // Afficher le même écran


    switch (ecranCourant)
    {
      //case ECRAN_DEMARRAGE :
      //  break;

      case ECRAN_PRINCIPAL :
        AfficheEcranPrincipal();
        break;
      case ECRAN_RESERVE_ENERGIE :
        AfficheEcranReserveEnergie();
        break;
      case ECRAN_INDEX_JOURNALIERS :
        AfficheEcranIndexJournaliers();
        break;
      case ECRAN_JOURS_TEMPO :
        AfficheEcranJoursTempo();
	      break;
      default :
        AfficheEcranPrincipal();
        break;
    }


  } else { // Afficher l'écran suivant


    switch (ecranCourant)
    {
      //case ECRAN_DEMARRAGE :
      //  break;
      case ECRAN_PRINCIPAL :
        // On revient à l'écran réserve d'énergie
        AfficheEcranReserveEnergie();
        break;
      case ECRAN_RESERVE_ENERGIE :
        // On passe à l'écran des index journaliers
        AfficheEcranIndexJournaliers();
        break;
      case ECRAN_INDEX_JOURNALIERS :
        // On revient à l'écran principal
        if (affichageEcranTempo)
          AfficheEcranJoursTempo();
        else
          AfficheEcranPrincipal();
        break;
      case ECRAN_JOURS_TEMPO : 
        // On revient à l'écran principal
        AfficheEcranPrincipal();
        break;
      default :
        AfficheEcranPrincipal();
        break;
    }
  }
}

/* Affiche le message de Débug sur l'écran TFT (car parfois, c'est plus pratique ;D )*/
void AfficheDebugTFT (String message)
{
  // Sécurité : sauvegarde la valeur courante sur l'affichage principal, pour la remettre après l'affichage graphique du message débug. 
  // Car sinon cela peut poser des problèmes d'affichage sur d'autres textes
  uint8_t datumSvg = sprite.getTextDatum();

  sprite.setTextColor(TFT_YELLOW,TFT_BLACK);
  // Le message est cadré en bas à droite
  sprite.setTextDatum(BR_DATUM);
  sprite.drawString(message, 320, 170, 2);
  sprite.pushSprite(0, 0);

  sprite.setTextDatum(datumSvg);
}

/***************************************************************************************
**     Routine de test pour afficher toutes les icones sur l'écran
**     Décommenter la ligne correspondante "test()" pour l'activer
***************************************************************************************/
void test() {
  PV = "1234";
  CO = "4500";
  CU = "1290";
  TEMPCU = "55,5";
  tempExt = "3";
  sondeTemperatureECS = true;
  chauffageElectr = true;
  veille = false;
}
