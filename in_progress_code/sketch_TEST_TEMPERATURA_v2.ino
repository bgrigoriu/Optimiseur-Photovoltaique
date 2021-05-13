#include <OneWire.h>

// Defining digitalpins

#define switchpin  8           // switch vers charge 2 - cas ballon plein ( !! en  D8 actuellement sur PCB)
#define pin_1_wire_bus A2      // Pin mesure temperature
#define pin_hc 9               // Pin lecture heures creuse  active LOW
#define pin_source_appoint A1  // Commande source externe  PC1 ADC1 (analog 1) - source extene

// definitions deja ds EcoPV
#define pulseTriacPin     2
// Syncro pins            D3 et D4
// #define relayPin       5  (en D5 actuelement sur PCB  à modifier sur V2 pour 8)
// LEDs PINs D6 et D7

//Defining resolution of the temperature readings un comment one of the four options
//#define TEMPERATURE_PRECISION 0b00111111 //precision 10 bits implicite choice 180 ms
#define TEMPERATURE_PRECISION 0b00011111 //precision 9 bits temps de conversion 90 ms
//#define TEMPERATURE_PRECISION 0b01011111 //precision 11 bits temps de conversion 375 ms
//#define TEMPERATURE_PRECISION 0b01111111 //precision 12 bits temp de conversion  750 ms


// definition des  Macro's SWITCH SUR D5 SOIT PORT D
//#define SWITCH_ON  PORTD |=  ( 1 << switchpin)
//#define SWITCH_OFF PORTD &= ~( 1 << switchpin)

// definition pour pin D8 =PB0
#define SWITCH_ON  PORTB  |=0b00000001   // on pourrait les definir avec change byte
#define SWITCH_OFF PORTB &= 0b11111110


//definitions deja existentes dans EcoPV
#define TRIAC_ON               PORTD |=  ( 1 << pulseTriacPin )
#define TRIAC_OFF              PORTD &= ~( 1 << pulseTriacPin )

//Variables retournés apres initialisation de la sonde de temperature
enum DS18B20_RCODES {
  SENSOR_OK,                                                        // Lecture ok
  NO_SENSOR_FOUND,                                                // Pas de capteur
  INVALID_ADDRESS,                                                // Adresse reçue invalide
  INVALID_SENSOR                                                  // Capteur invalide (pas un DS18B20)
};


// Variables lié à fonction antilegionelle  a mettre dans l'EEPROM
byte            T_LEGIONELLE                        = 60;         //temperature minimale à atteindre pour asurer un protection antilegionelle
unsigned int    TEMPS_MINI_DESINF_LEGIONELLE        = 1800;       // temps mini en sec à temperature legionelle pour assurer une protection
unsigned long   TEMPS_ENTRE_DEUX_DESINF_LEGIONELLE  = 604800;     // temps en sec entre deux desinfections legionelle ( a mettre avec les parametres)
bool            PROCEDURE_LEGIONELLE                = false;

// Parametter d'hysteresis en cas de changement de temperature

byte   HYSTERESIS_TEMPS                    = 30;          // temps mini avant changement etat routeur (sec)

// Variables cumulus à mettre egalement dans l'EEPROM
byte    T_MAX                               = 55;          // temperature max cible , au dela switch vers la charge deux condition T_MAX > T _confort
byte    T_CONFORT                           = 45;          // Temp mini a atteindre pour assurer confortablement
byte    T_MINI                              = 40;          // Temp mini a assurer dans tous les cas
byte    T_SECURITE                          = 4;
bool    chauffe_necessaire                  = false;               


//  variables liés à la mesure de temperature

byte          adresse_sonde[8];
bool          sonde_temp_presente                     = false;
bool          heures_creuses                          = false;
bool          router_is_Off                           = false;
bool          mode_vacances_actif                     = false;
unsigned long temps_restant_en_mode_vacance           = 0;
bool          mesure_temperature_disponible           = false;
bool          need_to_reinitialise                    = true;
bool          need_for_legionella_desinfection        = false;
int8_t        temperature_cumulus;
unsigned int  compteur_Temps_desinf_legionelle        = 0;
unsigned long temps_depuis_derniere_desinf_legionelle = 0;
uint8_t       compteur_status                         = 0;

OneWire sonde(pin_1_wire_bus);          // creation de sonde comme objet OneWire

void setup() {
              
  DDRB  &=  0b11111101;                 //a.k.a  pinMode   (pin_hc, INPUT);   // on defini ou on doit lire le status heures creuses
  DDRB  |=  0b00000001;                 //a.k.a  pinMode   (switchpin, OUTPUT);          // pin qui change l'orientattion de SSR1 vers une deuxieme sortie option  
  DDRC  |=  0b00000010;                 //a.k.a  pinMode   (pin_source_appoint, OUTPUT); // pin activation source apoint  option  
  PORTB &=  0b11111110;                 //a.k.a  digitalWrite (switchpin,LOW)
  PORTC &=  0b11111101;                 //a.k.a  digitalWrite (pin_source_appoint,LOW);   // 
  
  if (initialisesondetemperature (adresse_sonde, TEMPERATURE_PRECISION)== SENSOR_OK) {     //apeller l'initialisation de sonde de temp 
      sonde_temp_presente = true;
      startMesureTemperature (adresse_sonde);                                   // initier la mesure de temperature
      delay(1000);                                                              // atendre la lecture
      if (readMeasuredTemperature (&temperature_cumulus, adresse_sonde)) {      // si lecture OK  
      need_to_reinitialise = false;
      mesure_temperature_disponible = false;                                     //signale que le prochain cycle on redemare la mesure de temperature
      }
      }
  else {                                                                          // si erreur d'initailisation
      sonde_temp_presente = false;                                                // on coupe la boucle de gestion de temperature
      need_to_reinitialise = true;                                               // on va ressayer toutes les secondes esnsuite
      }
}    // End Setup


void loop() {

 //initialisation variables locales
 // a mettre ici a partir de celles globales

 
//routine de gestion de la temp a mettre dans la boucle de 1 sec.

if (need_to_reinitialise) {                                                               // si erreur de lecture reinitialiser la sonde - permet la conexion à chaud
 compteur_Temps_desinf_legionelle        = 0;                                           // on remet à zero les compteur legionelose et celui de changement d'etat
 temps_depuis_derniere_desinf_legionelle = 0;
 compteur_status                         = 0;

 if (initialisesondetemperature (adresse_sonde, TEMPERATURE_PRECISION)== SENSOR_OK) {     //apeller l'initialisation de sonde de temp si erreur
      sonde_temp_presente = true;
      need_to_reinitialise = false;
      mesure_temperature_disponible = false;                                            //signale que la temp est disponible au prochain cycle
      temperature_cumulus = 20;                                                         // on charge une valeur arbitraire pour le premier cycle
      }
      else {
        sonde_temp_presente = false;
        need_to_reinitialise = true;                                                    // on reinitialise le cycle suivant ( ds une seconde)
        router_On();                                                                    // si on n'arrive pas a initialiser la sonde on redemarre le routeur;
        }
  }

  
if (sonde_temp_presente) {                                                            // Si la sonde est presente 
 
   compteur_status++;                                                    // incrementation du compteur dhysteresis de temps pour chaque changement d'etat du routeur
    if (compteur_status > 240) compteur_status = 240;                    // on limite le compteur à 240 secondes max
      
   if (PROCEDURE_LEGIONELLE == true) {                                        // On gere la desinfection legionelle si demandée
      temps_depuis_derniere_desinf_legionelle++;                              // incrementation des compteurs legionelose a chaque seconde
      if (temperature_cumulus > T_LEGIONELLE) {                               // si la temperature legionelle est atteinte (utilise la mesure du cycle precedent
          compteur_Temps_desinf_legionelle ++;                                // incrementer compteur desinfection legionelle
          if (temperature_cumulus > (T_LEGIONELLE+4))  {                       // Si temp au dela de 65°C cela compte double
              compteur_Temps_desinf_legionelle ++;  
          }
          if (temperature_cumulus > (T_LEGIONELLE+6))  {                       // Si temp au dela de 67°C cela compte triple
              compteur_Temps_desinf_legionelle ++;  
          }
          if (temperature_cumulus > (T_LEGIONELLE+9))  {                       // Si temp au dela de 70°C cela compte x4 < 10 minutes sufisent
              compteur_Temps_desinf_legionelle ++;  
          }
          if (compteur_Temps_desinf_legionelle > 1800) {                      // si compteur > 30 minutes equivalent le limiter à 30 min
              compteur_Temps_desinf_legionelle = 1800;
              temps_depuis_derniere_desinf_legionelle = 0;                    // et mettre a zero compteur temps depuis desinfection legionelle
              need_for_legionella_desinfection= false;
          }
      }
      else {
        compteur_Temps_desinf_legionelle = 0;                                 // sinon on remet a zero le temps de desinfection si la temp passe endessous de la temp necessaire pour deinfecter
        }
   }
    
       // Initialisation des variables sur lequelles on regle le routage
      
 if (digitalRead(pin_hc) == LOW) {                                               // lecture de l'état heures creuses peut etre remplace par heures_creuses= (PINB & 0b00000010)
     heures_creuses = true;
          }
 else { 
     heures_creuses = false;
     }
     
 if (temps_depuis_derniere_desinf_legionelle > TEMPS_ENTRE_DEUX_DESINF_LEGIONELLE) { // on decide si on a besoin de lancer une desinfection legionelle
    need_for_legionella_desinfection = true; 
    }
 else {
    need_for_legionella_desinfection = false;                           
 }

if (mode_vacances_actif) temps_restant_en_mode_vacance--;                    // on decremente d'une seconde le temps restant en mode vacance si actif 

if (temps_restant_en_mode_vacance == 0) {                                     // si le temps a passer en mode vacances est fini sortir du mode vacances
    mode_vacances_actif = false;
  }
        
if (!mesure_temperature_disponible) {
    startMesureTemperature (adresse_sonde);                                                // initier une nouvelle mesure si temp non deja lue et passer au reste
    mesure_temperature_disponible = true;                                                  //signale que la temp est disponible au prochain cycle
    }
else {                                                                                     // debut lecture de la temperature lancée le cycle précedent                                                                      
    if (!readMeasuredTemperature (&temperature_cumulus, adresse_sonde)) {                  // si erreur de mesure 
        need_to_reinitialise = true;                                                       // la sonde sera reinitialisée au prochain cycle et une demande de mesure sera faite  
       if (router_is_Off) router_On();                                                     // si perte de sonde pendant la chauffe forcee on revient en routage normal
           compteur_status = 0;                                                            // en remet à zero le compteur
       }                                                                                   // une relecture de temperature sera faite dans 2 cycles                           
    else {                                                                                 // Debut gestion temperature
          mesure_temperature_disponible = false;                                           //signale que la temp mesuree a été lue et relance au cycle suivant
                                                                                           // et demarer la gestion de temperature
    
           //  On determine si on doit arreter le routage proportionel et allumer à 100% le SSR

          if ((temperature_cumulus < T_SECURITE) ||                                         // On est en dessous de la temeprature de securite  
              (need_for_legionella_desinfection && heures_creuses) ||                       // On a besoin de desinfecter pour la legionelle et on est en heures creuses
              (!mode_vacances_actif && (temperature_cumulus < T_MINI))  ||                  // On est à la maison et on est en dessous de la temperature minimale requise
              (!mode_vacances_actif && (temperature_cumulus < T_CONFORT) && heures_creuses)) // On est à la maison et on n'a pas atteint la temperature de confort et on est en heures creuses
              { chauffe_necessaire = true; }
          else 
              {chauffe_necessaire = false;} 

    // Gestion du TRIAC 
    
          if ((!router_is_Off) && (chauffe_necessaire == true) && (compteur_status > HYSTERESIS_TEMPS)) {    // si on est en train de router normalement et on a besoin de chauffer 100%: On attends le temps d'hysteresis et on alume le triac
                 compteur_status =0;                                                            // on remets le compteur à zero
                 router_is_Off = true;                                                          // on signale qu'on arrete le routeur
                 router_Off();                                                                  // et on l'arrete et on commence a chauffer
             }
          else if ((router_is_Off) && (chauffe_necessaire == false) && (compteur_status > HYSTERESIS_TEMPS)) {  // si le routage proportionel est arrété pn attends le temps d'hysteresis et on eteint le triac
                 compteur_status =0;                                                            // on remets le compteur à zero
                 router_is_Off = false;                                                          // on signale qu'on redemammre le routage
                 router_On();                                                                      // et on le redemarre
                 }

    // Gestion de la deuxieme voie en cas de cumulus plein
          if ((temperature_cumulus >= T_MAX) && (!need_for_legionella_desinfection) && (digitalRead(switchpin) == LOW)) {         // Si on depasse la telperature cible maximale et on n'as pas besoin de legionelle
             if (compteur_status > HYSTERESIS_TEMPS) {                                         // on verifie qu'in n'as pas fait de changement depuis 4 min
                compteur_status = 0;
                SWITCH_ON;
             }
          }

          if ((digitalRead(switchpin) == HIGH) && ((temperature_cumulus +4  < T_MAX ) || need_for_legionella_desinfection )) { //Si on est en train de router vers la sortie 2 et la temp du cumulus descend de 3 degre ou on a besoinde chaufefr pour la legionelle
               compteur_status = 0;
               SWITCH_OFF;                                                                       // on arrete de deriver vers la sortie 2
             }
      }                                                                                       // fin de gestion temperature
  }                                                                                          //Fin du Block de mesure de la temperature
}                                                                                            //Fin du  if bloc avec sonde de tempertature

// ici On continue si on n'a pas de sonde de temperature

} // Fin de loop

// Definition des fonctions


// Fonction d'initialisation de la sonde de temperature
byte initialisesondetemperature (byte *addr, byte precision) {
      //  Cherche sonde temperature
      sonde.reset_search();  /* Recherche le prochain capteur 1-Wire disponible */
 
      if (!sonde.search(addr)) {  // Pas de capteur
          return NO_SENSOR_FOUND;
          }
      if (OneWire::crc8(addr, 7) != addr[7]) { // Verifie que l'adresse est valide
        return INVALID_ADDRESS;
          }
      if (addr[0] != 0x28) {  // Verifie le type de capteur
        return INVALID_SENSOR;
          }
      else {
        sonde.reset();
        sonde.select(addr);
        sonde.write(0x4E);
        sonde.write(0x00);
        sonde.write(0x00);
        sonde.write(precision);
        sonde.reset();
      return SENSOR_OK;
        }
}


//Fonction de demande de mesure de la temperature
void startMesureTemperature (byte *adresse_sonde) {
  sonde.reset();
  sonde.select(adresse_sonde);
  sonde.write(0x44, 1);
}
//fin de la demande de lesure de temperature


//fonction de lecture de la temperature mesurée
bool readMeasuredTemperature (int8_t *temperature_cumulus, byte *adresse_sonde) {
  byte data[9]; // données lues du capteur
  sonde.reset();
  sonde.select(adresse_sonde);
  sonde.write(0xBE);
  for (byte i = 0; i < 9; i++)  {
    data[i] = sonde.read();
      }
  if (OneWire::crc8(data, 8) == data[8]) {                             // Verifie que la lecture est bien faite 
    *temperature_cumulus = (int8_t) ((data[1] << 4) | (data[0] >> 4));
    return true;
    }
  else {
    return false;
  }  
  } //fin de la fonction de lecture de la temperature


// Fonction d'arret du routeur et allumage 100%% triac

  void router_Off (void) {                                                 
     TCCR1B = 0x00;                                                      // on arrete le timer 
     TCNT1  = 0x00;                                                      // on mets le comteur a zero
     TRIAC_ON;                                                           // on allume le triac
    }

// Fiunction redemarrage du routeur et extinction du triac

void router_On (void) {
   TRIAC_OFF;                                                     // On eteint le triac
   OCR1A  = 30000;                                                // on charge un delai inataignable par securite  
   TCCR1B = 0x05;                                                 // on redemarre le Timer 1
  }
