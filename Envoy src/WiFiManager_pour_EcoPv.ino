#include <WiFiManager.h>   // https://github.com/tzapu/WiFiManager
#include <EEPROM.h>         //https://github.com/esp8266/Arduino/tree/master/libraries/EEPROM
#include <ESP8266WiFi.h>      //https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WiFi
#include <ArduinoJson.h>      // Ver 6.18.2
#include <ESP8266HTTPClient.h>  //https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266HTTPClient
#include "SSD1306AsciiWire.h"  //https://github.com/greiman/SSD1306Ascii/blob/master/src/SSD1306AsciiWire.h
#include <ESP8266Ping.h> 
         
          // Other library used  

          // Wire.h, 

// definition for envoy connection

#define userName "installer" 
#define domain "enphaseenergy.com"


const unsigned long             DATAEEPROM_MAGIC      = 2702090467;     
#define VERSION                 "0.1"
#define DEBUG_SERIAL            Serial
#define SERIAL_BAUD_DEBUG       74880
#define SERIAL_BAUD_NORMAL      74880
#define WAIT_TIME_FOR_RESETING  50   // = sec x 5


#define I2C_ADDRESS 0x3C                    // adresse I2C de l'écran oled
#define OLED_128X64_REFRESH_PERIOD  6       // période de raffraichissement des données à l'écran
#define OLED_RST_PIN -1

#define I2C_SCL   2
#define I2C_SDA   0


// definition of pins on the ESP8266

#define  RESET_PIN              2
//#define  syncroACPin          1   // a definir pas implementé a ce stade
//#define  pulseTriacPin        3   //   a definir pas impelmenté a ce stade
//#define  pinReadTeleinfo      4   //  a definir pas implemnté a ce stade



// defining here parameters for routing statistics



/******************Class definition for readin IP value */   // to review if necesary

class IPAddressParameter : public WiFiManagerParameter {
public:
    IPAddressParameter(const char *id, const char *placeholder, IPAddress ip)
        : WiFiManagerParameter("") {
        init(id, placeholder, ip.toString().c_str(), 16, "pattern='((^25[0-5])|(^2[0-4][0-9])|(^1[0-9][0-9])|(^[1-9][0-9])|(^[1-9]))\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$'", WFM_LABEL_BEFORE);
    }

    bool getValue(IPAddress &ip) {
        return ip.fromString(WiFiManagerParameter::getValue());
    }
};

/***********End of definitions of classes*******************/


struct Settings_EEPROM {
    unsigned long       magic;     //magic number
    char                s[13];     //serial Envoy
    uint32_t            ip;        // Envoy IP address
    char                passwd[9];
    // to put here all setting needed for functioning
} ;


char          envoy_serial[13];
IPAddress     envoyip;
char          envoy_password[9];


// parameters from envoy Stream

const char *username = "installer";

const char *uri = "/stream/meter";  //on Https://envoyip

float Power =0 ;
float AReal_power = 0;
float AReactive_power = 0;
float AApparent_power = 0;
float Avoltage = 0;
float Acurrent =0 ;
float pf = 0;
float freq = 0;

float BReal_power = 0;
float BReactive_power = 0;
float BApparent_power = 0;
float Bvoltage = 0;
float Bcurrent =0 ;

float CReal_power = 0;
float CReactive_power = 0;
float CApparent_power = 0;
float Cvoltage = 0;
float Ccurrent =0 ;

float tcAReal_power = 0;
float tcAReactive_power = 0;
float tcAApparent_power = 0;
float tcAvoltage = 0;
float tcAcurrent =0 ;

float tcBReal_power = 0;
float tcBReactive_power = 0;
float tcBApparent_power = 0;
float tcBvoltage = 0;
float tcBcurrent =0 ;

float tcCReal_power = 0;
float tcCReactive_power = 0;
float tcCApparent_power = 0;
float tcCvoltage = 0;
float tcCcurrent =0 ;

float Consumption = 0; 
float NetPower = 0;

// end of pârameters from envoy Stream


WiFiManager wm(DEBUG_SERIAL); // defining wm objectFiManager wm.(Serial1); 
SSD1306AsciiWire oled;
 
void setup() {
  WiFi.begin();
  EEPROM.begin(512);   // initialiser la fonction de l'EEPROM
  Wire.begin(I2C_SCL, I2C_SDA);
  oled.begin( &SH1106_128x64, I2C_ADDRESS );
//  oled.begin( &Adafruit128x64, I2C_ADDRESS );
  oled.setFont ( System5x7 );
  oled.clear ( );
  oled.set2X ( );
  oled.print ( F("EcoPV V") );
  oled.println ( F(VERSION) );
  oled.println ( );
  oled.println ( F("Starting...") );
  delay(3000);
  
   
  
  //initialisation des pins
 
  // pinMode      ( synchroACPin,  INPUT  );   // Entrée de synchronisation secteur
  // pinMode      ( pulseTriacPin,OUTPUT);     // Sortie triac
  // pinMode  ( pinReadTeleinfo, INPUT);        //  a definir pas implemnté a ce stade   seulement pour ESP12 

  // digitalWrite    ( pulseTriacPin, LOW);    // A implementer

  WiFi.mode(WIFI_STA);  //Put in station mode in case of reboot without getting parameters
  
  DEBUG_SERIAL.begin(SERIAL_BAUD_DEBUG);
  DEBUG_SERIAL.print("DEBUG_SERIAL activated at ");
  DEBUG_SERIAL.println(SERIAL_BAUD_DEBUG);
  DEBUG_SERIAL.setDebugOutput(false);
  DEBUG_SERIAL.setDebugOutput(true);
  DEBUG_SERIAL.println("Debug Activated on DEBUG_SERIAL");
  DEBUG_SERIAL.println("Activating Serial 0");

  if ( eepromConfigRead ( ) == false )  {    //sequence de lecture de parametres 
   
    DEBUG_SERIAL.println("Missing Configuration in memory");
    DEBUG_SERIAL.println("Activating Wifi Manager");
    oled.clear ( );
    oled.set2X();
    oled.println (" No Config" );
    oled.println ("Connect to" );
    oled.println (" WiFi AP" );
    
    wm.resetSettings(); 
    wm.setDebugOutput(true);

    envoy_serial[12] = '\0';   //add null terminator at the end cause overflow
     
    WiFiManagerParameter envoy_serial_str( "envoy_serial", "Envoi Serial Number",  envoy_serial, 12,"pattern='^\\d{12}$'");
    IPAddressParameter param_ip("EnvoyIP", "Envoy_IP",  envoyip);

    wm.addParameter( &envoy_serial_str );
    wm.addParameter( &param_ip );

       if (!wm.autoConnect("EcoPV WiFi AP")) {
        DEBUG_SERIAL.println("Failed to connect with available parameters......ESP.restarting");
        oled.clear ( );
        oled.set2X();
        oled.println ("Bad Config" );
        oled.println ("Restarting" );
        delay(2000);
        wm.resetSettings(); 
        eepromConfigErase();
        ESP.restart();
        } else {
            if (!Ping.ping(envoyip)) {
              DEBUG_SERIAL.print("Envoy IP  unreachable : ");
              DEBUG_SERIAL.print("Deleting Configuration");
              oled.print("EnvoyIP:");
              oled.print("Unreachable");
              oled.print("Deleting Config");
              delay(2000);
              wm.resetSettings(); 
              eepromConfigErase();
              ESP.restart();// testing if Ip exists and if yes write data to eeprom
              }
              else {
              DEBUG_SERIAL.print("EnvoyIP OK");
              oled.print("EnvoyIP OK");
                 //
              delay(1500);
              }
              
          // selected IP and serial number of Envoy
              strncpy(envoy_serial, envoy_serial_str.getValue(), 12);
              envoy_serial[12] = '\0'; 
              param_ip.getValue(envoyip);

          // generating Envoy passwd here 
              strcpy(envoy_password, emupwGetMobilePasswd(envoy_serial).c_str());
              
              DEBUG_SERIAL.print("WiFi Connected as :)");
              DEBUG_SERIAL.println(WiFi.localIP());
              
              DEBUG_SERIAL.print("Envoy serial numbber is: ");
              DEBUG_SERIAL.println(envoy_serial);
                            
              DEBUG_SERIAL.print("Envoy installer password is: ");
              DEBUG_SERIAL.println(envoy_password);
              
              DEBUG_SERIAL.print("Envoy IP  Address is : ");
              DEBUG_SERIAL.println(envoyip);
              
              oled.clear ( );
              oled.set2X();
              oled.println (" Config OK" );
              oled.set1X();
              
              oled.print ("IP: " );
              oled.println(WiFi.localIP());
              
              oled.print ("Serial:" );
              oled.println(envoy_serial);

              oled.print("EnvoyPWD: ");
              oled.println(envoy_password);

              oled.print("EnvoyIP:");
              oled.println(envoyip);
             
              eepromConfigWrite(); 
              
           }

  } else {  // Opening On demand reset procedure
  
          DEBUG_SERIAL.println(envoy_serial);
          DEBUG_SERIAL.println(envoyip);
          DEBUG_SERIAL.println(envoy_password);

          if (!Ping.ping(envoyip)) {
              DEBUG_SERIAL.print("Envoy IP  unreachable : ");
              DEBUG_SERIAL.print("Check Envoy");
              DEBUG_SERIAL.print("Reset Settings if needed");
              oled.clear();
              oled.println("EnvoyIP:");
              oled.println("Unreachable");
              oled.println("Reset if needed");
              delay(2000);
             }
              else {
              DEBUG_SERIAL.print("EnvoyIP OK");
              DEBUG_SERIAL.print("Reset Settings if needed");
              oled.clear();
              oled.println("EnvoyIP OK");
              oled.println("Reset if needed");
              delay(1500);
              }
           
            DEBUG_SERIAL.println("You have 10 seconds to reset");
      //    DEBUG_SERIAL.setDebugOutput(false); 
      //    DEBUG_SERIAL.end();

          oled.clear();
          oled.set2X();
          oled.println ("Reset ?");
          oled.println ("\n10sec left" );
          
          Wire.begin(4,5);   
        
        //Opening Reset Window
          pinMode(0,OUTPUT);    // Sequence specific for ESP-01 with sharing GPIO0 :prepare GPIO0 to be used as input for resetting configuration; 
          digitalWrite(0,LOW);  // need a GND reference poit i.e. GPIO0. Reset Swict between GPIO0 and GPIO2 READ https://www.forward.com.au/pfod/ESP8266/GPIOpins/
          pinMode(RESET_PIN,INPUT);  // activatingGPIO2 pin for resetting configuration
       
          for (int i=0; i< WAIT_TIME_FOR_RESETING; i++) {
            if( digitalRead(RESET_PIN) == LOW) {                                 
                i=WAIT_TIME_FOR_RESETING;
                DEBUG_SERIAL.println("Reseting");
                
                Wire.begin(I2C_SCL, I2C_SDA);
                oled.clear();
                oled.set2X();
                oled.println ("Reseting");
                delay(1500);
                wm.resetSettings(); 
                eepromConfigErase();
                ESP.restart();  
            } else  {
                  delay(200);
              }
          }
          if (!Ping.ping(envoyip)) {
              DEBUG_SERIAL.print("Envoy IP  unreachable : ");
              oled.print("EnvoyIP:");
              oled.print("Unreachable");
              oled.print("Restarting");
              delay(2000);
              ESP.restart();// testing if Ip exists and if yes write data to eeprom
              }
              else {
              DEBUG_SERIAL.print("EnvoyIP OK");
              oled.print("EnvoyIP OK");
              delay(1500);
              }
          
       Wire.begin(I2C_SCL, I2C_SDA); 
      }
   // end config parameters
  // testing here if IP is OK
  // 
  //connecting to envoy

randomSeed(RANDOM_REG32);
  
// starting printing
}

void loop() {

  // put your main code here, to run repeatedly:
 
  WiFiClient client;
  HTTPClient http;                                  

  DEBUG_SERIAL.print("[HTTP] begin...\n");  
  
  if (http.begin(client, String("http://"+ envoyip.toString())+ String(uri))) {
    DEBUG_SERIAL.print("Begin OK");
    oled.clear();
    oled.set2X();
    oled.println("Begin OK");
    } else {
      DEBUG_SERIAL.print("Begin failed");
    oled.clear();
    oled.set2X();
    oled.println("Begin Failed");
      }

  const char *keys[] = {"WWW-Authenticate"};
  http.collectHeaders(keys, 1);

  DEBUG_SERIAL.print("[HTTP] GET...\n");                    // Starting first GET - authentication only
  oled.println("HTTP GET.");
  
  // start connection and send HTTP header
  
  int httpCode = http.GET();
    if (httpCode > 0) {
      oled.println("Responded");
      String authReq = http.header("WWW-Authenticate");
      String authorization = getDigestAuth(authReq, String(username), String(envoy_password), "GET", String(uri), 1);
      http.end();
    
      http.begin(client, String("http://"+envoyip.toString()) + String(uri));
      http.addHeader("Authorization", authorization);

      int httpCode = http.GET();           // Second GET - Actually getting the payload here
      if (httpCode == 200) {
         oled.print("GET OK");
         WiFiClient *cl = http.getStreamPtr();
        int error = 0;
        do {
          cl->find("data: ");
          String payload = cl->readStringUntil('\n');
          error = processingJsondata(payload);
          client.flush();
           oled.clear();
           oled.set2X();
           oled.println ("PROD  CONS");
           oled.print (round(Power)); 
           oled.print(" ");
           oled.print(round(Consumption)); 
        } while (error);
      } else {
            DEBUG_SERIAL.print("[HTTP] GET.2.. failed, error: ");
            DEBUG_SERIAL.println(http.errorToString(httpCode).c_str());
           oled.clear();
           oled.set2X();
           oled.println ("GET failed");
           oled.print ("Error: ");
           oled.println(http.errorToString(httpCode).c_str());
        }
    } else {
          DEBUG_SERIAL.print("[HTTP] GET.1.. failed, error: ");
          DEBUG_SERIAL.println(http.errorToString(httpCode).c_str());
           oled.clear();
           oled.set2X();
           oled.println ("GET failed");
           oled.print ("Error: ");
           oled.println(http.errorToString(httpCode).c_str());
      }

  http.end();
  delay(2000);
}



bool eepromConfigRead ( void ) {
  
  Settings_EEPROM settings;
  EEPROM.get (0, settings );

  if ( settings.magic != DATAEEPROM_MAGIC ) {
    return false;
    }
  else {
   // initialise here all variables read from EEPROM
    strncpy(envoy_serial, settings.s, 12);
    envoyip         =   settings.ip; 
    strncpy(envoy_password, settings.passwd, 8);
    return true;
  }
}

bool eepromConfigWrite (void) {
 
  Settings_EEPROM settings;
  
  settings.magic    =  DATAEEPROM_MAGIC;
  strncpy(settings.s, envoy_serial, 12);
  strncpy(settings.passwd, envoy_password,8);
  settings.ip       =  envoyip;

  EEPROM.put(0,settings);
  if (EEPROM.commit()) {
      return true;
   } else {
      return false;
     }
}


bool eepromConfigErase (void) {
 
  Settings_EEPROM settings;
  
  settings.magic    =  0;

  EEPROM.put(0,settings);
  if (EEPROM.commit()) {
      return true;
   } else {
      return false;
     }
}

// Fonction calcul Hash MD5
String emupwGetPasswdForSn(String serial, const String user, String srealm) {

 MD5Builder md5;
 String hash1;
  md5.begin();
  md5.add("[e]" + user + "@" + srealm + "#" + serial + " EnPhAsE eNeRgY ");
  md5.calculate();
  hash1 = md5.toString();
  return hash1;
  }
  
//Fin de fonction


// fonction de calcul du mot de passe
String emupwGetMobilePasswd(String serial){

String digest;
int countZero = 0;
int countOne =  0;
digest = emupwGetPasswdForSn(serial,userName,domain);
int len = digest.length();
for (int i=0 ; i<len; i++) {
  if (digest.charAt(i) == char(48)) countZero++;
  if (digest.charAt(i) == char(49)) countOne++;
  }
  String password ="";
 for (int i=1; i < 9; ++i) {
  if (countZero == 3 || countZero == 6 || countZero == 9) countZero -= 1;
  if (countZero > 20) countZero = 20; 
  if (countZero < 0) countZero = 0;

  if (countOne == 9 ||countOne == 15) countOne--;
  if (countOne > 26) countOne = 26;
  if (countOne < 0) countOne = 0;
  char cc = digest.charAt(len-i);
  if (cc == char(48)) {
    password += char(102 + countZero);  // "f" =102
            countZero --;
    }
  else if (cc == char(49)) {
     password += char(64 + countOne);  // "@" = 64
            countOne--; 
      }
   else password += cc;
  }

    return password;
}

// End of password related routines


// Digest authorization routines


String exractParam(String& authReq, const String& param, const char delimit) {
  int _begin = authReq.indexOf(param);
  if (_begin == -1) {
    return "";
  }
  return authReq.substring(_begin + param.length(), authReq.indexOf(delimit, _begin + param.length()));
}


String getCNonce(const int len) {
  static const char alphanum[] =
    "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz";
  String s = "";

  for (int i = 0; i < len; ++i) {
    s += alphanum[rand() % (sizeof(alphanum) - 1)];
  }

  return s;
}


String getDigestAuth(String& authReq, const String& username, const String& password, const String& method, const String& uri, unsigned int counter) {
  // extracting required parameters for RFC 2069 simpler Digest
  String realm = exractParam(authReq, "realm=\"", '"');
  String nonce = exractParam(authReq, "nonce=\"", '"');
  String cNonce = getCNonce(8);

  char nc[9];
  snprintf(nc, sizeof(nc), "%08x", counter);

  // parameters for the RFC 2617 newer Digest
  MD5Builder md5;
  md5.begin();
  md5.add(username + ":" + realm + ":" + password);  // md5 of the user:realm:user
  md5.calculate();
  String h1 = md5.toString();

  md5.begin();
  md5.add(method + ":" + uri);
  md5.calculate();
  String h2 = md5.toString();

  md5.begin();
  md5.add(h1 + ":" + nonce + ":" + String(nc) + ":" + cNonce + ":" + "auth" + ":" + h2);
  md5.calculate();
  String response = md5.toString();

  String authorization = "Digest username=\"" + username + "\", realm=\"" + realm + "\", nonce=\"" + nonce +
                         "\", uri=\"" + uri + "\", algorithm=\"MD5\", qop=auth, nc=" + String(nc) + ", cnonce=\"" + cNonce + "\", response=\"" + response + "\"";

  return authorization;
}
// end of digest autorization routines


// JSon routines

// Routine de gestion et impression des donnes JSON

int processingJsondata(String payload) {

DynamicJsonDocument doc(2000);          // Added from envoy exemple
     
// Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, payload);
      
  // Test if parsing does not succeeds.
  if (error!=DeserializationError::Ok) {
    DEBUG_SERIAL.print(F("deserializeJson() failed: "));
    DEBUG_SERIAL.println(error.f_str());
    return 0;
  }

   JsonObject obj = doc.as<JsonObject>();

      if (obj.containsKey("production"))
      {
          if (obj["production"].containsKey("ph-a"))
          {
              AReal_power = obj["production"]["ph-a"]["p"]; 
              AReactive_power = obj["production"]["ph-a"]["q"];  
              AApparent_power = obj["production"]["ph-a"]["s"];
              Avoltage = obj["production"]["ph-a"]["v"];
              Acurrent = obj["production"]["ph-a"]["i"];
              pf = obj["production"]["ph-a"]["pf"];
              freq = obj["production"]["ph-a"]["f"];
              
                  DEBUG_SERIAL.print("\nPhase A - ");
                  DEBUG_SERIAL.print("Real Power: ");
                  DEBUG_SERIAL.print(AReal_power);
                  DEBUG_SERIAL.print("\tReact Power: ");
                  DEBUG_SERIAL.print(AReactive_power);
                  DEBUG_SERIAL.print("\tApparent Power: ");
                  DEBUG_SERIAL.print(AApparent_power);
                  DEBUG_SERIAL.print("\tVoltage: ");
                  DEBUG_SERIAL.print(Avoltage);
                  DEBUG_SERIAL.print("\tCurrent: ");
                  DEBUG_SERIAL.print(Acurrent);  
                  DEBUG_SERIAL.print("\tPower Factor: ");
                  DEBUG_SERIAL.print(pf);  
                  DEBUG_SERIAL.print("\tFrequency: ");
                  DEBUG_SERIAL.println(freq);
          }
          if (obj["production"].containsKey("ph-b"))
          {
              BReal_power = obj["production"]["ph-b"]["p"]; 
              BReactive_power = obj["production"]["ph-b"]["q"];  
              BApparent_power = obj["production"]["ph-b"]["s"];
              Bvoltage = obj["production"]["ph-b"]["v"];
              Bcurrent = obj["production"]["ph-b"]["i"];
              pf = obj["production"]["ph-b"]["pf"];
              freq = obj["production"]["ph-b"]["f"];
              
              if (Bvoltage>0) {
                
                  DEBUG_SERIAL.print("Phase B - ");
                  DEBUG_SERIAL.print("Real Power: ");
                  DEBUG_SERIAL.print(BReal_power);
                  DEBUG_SERIAL.print("\tReact Power: ");
                  DEBUG_SERIAL.print(BReactive_power);
                  DEBUG_SERIAL.print("\tApparent Power: ");
                  DEBUG_SERIAL.print(BApparent_power);
                  DEBUG_SERIAL.print("\tVoltage: ");
                  DEBUG_SERIAL.print(Bvoltage);
                  DEBUG_SERIAL.print("\tCurrent: ");
                  DEBUG_SERIAL.print(Bcurrent);  
                  DEBUG_SERIAL.print("\tPower Factor: ");
                  DEBUG_SERIAL.print(pf);  
                  DEBUG_SERIAL.print("\tFrequency: ");
                  DEBUG_SERIAL.println(freq);
              }
              else DEBUG_SERIAL.println("Phase B non connectée");
              
          }
          if (obj["production"].containsKey("ph-c"))
          {
              CReal_power = obj["production"]["ph-c"]["p"]; 
              CReactive_power = obj["production"]["ph-c"]["q"];  
              CApparent_power = obj["production"]["ph-c"]["s"];
              Cvoltage = obj["production"]["ph-c"]["v"];
              Ccurrent = obj["production"]["ph-c"]["i"];
              pf = obj["production"]["ph-c"]["pf"];
              freq = obj["production"]["ph-c"]["f"];
              
              if (Cvoltage >0)  {
                  DEBUG_SERIAL.print("Phase C - ");
                  DEBUG_SERIAL.print("Real Power: ");
                  DEBUG_SERIAL.print(CReal_power);
                  DEBUG_SERIAL.print("\tReact Power: ");
                  DEBUG_SERIAL.print(CReactive_power);
                  DEBUG_SERIAL.print("\tApparent Power: ");
                  DEBUG_SERIAL.print(CApparent_power);
                  DEBUG_SERIAL.print("\tVoltage: ");
                  DEBUG_SERIAL.print(Cvoltage);
                  DEBUG_SERIAL.print("\tCurrent: ");
                  DEBUG_SERIAL.print(Ccurrent);  
                  DEBUG_SERIAL.print("\tPower Factor: ");
                  DEBUG_SERIAL.print(pf);  
                  DEBUG_SERIAL.print("\tFrequency: ");
                  DEBUG_SERIAL.println(freq);
              }
              else DEBUG_SERIAL.println("Phase C non connectée");
              
          }
          
          Power = AReal_power+BReal_power+CReal_power;
                        
              DEBUG_SERIAL.print("\n  Power Produced:");
              DEBUG_SERIAL.print(Power);
              DEBUG_SERIAL.print("\tCurrent: ");
              DEBUG_SERIAL.print((Acurrent+Bcurrent+Ccurrent));
              DEBUG_SERIAL.print("\tPowerFactor:");
              DEBUG_SERIAL.print(pf);
              DEBUG_SERIAL.print("\tFrequency:");
              DEBUG_SERIAL.print(freq);
              DEBUG_SERIAL.println();
          
      } else {
             DEBUG_SERIAL.println("No production data");
             return 0;
        }

      if (obj.containsKey("total-consumption"))
      {
          if (obj["total-consumption"].containsKey("ph-a"))
          {
              tcAReal_power = obj["total-consumption"]["ph-a"]["p"]; 
              tcAReactive_power = obj["total-consumption"]["ph-a"]["q"];  
              tcAApparent_power = obj["total-consumption"]["ph-a"]["s"];
              tcAvoltage = obj["total-consumption"]["ph-a"]["v"];
              tcAcurrent = obj["total-consumption"]["ph-a"]["i"];
              pf = obj["total-consumption"]["ph-a"]["pf"];
              freq = obj["total-consumption"]["ph-a"]["f"];
              
                  DEBUG_SERIAL.print("\nPhase A - ");
                  DEBUG_SERIAL.print("Real Power: ");
                  DEBUG_SERIAL.print(tcAReal_power);
                  DEBUG_SERIAL.print("\tReact Power: ");
                  DEBUG_SERIAL.print(tcAReactive_power);
                  DEBUG_SERIAL.print("\tApparent Power: ");
                  DEBUG_SERIAL.print(tcAApparent_power);
                  DEBUG_SERIAL.print("\tVoltage: ");
                  DEBUG_SERIAL.print(tcAvoltage);
                  DEBUG_SERIAL.print("\tCurrent: ");
                  DEBUG_SERIAL.print(tcAcurrent);  
                  DEBUG_SERIAL.print("\tPower Factor: ");
                  DEBUG_SERIAL.print(pf);  
                  DEBUG_SERIAL.print("\tFrequency: ");
                  DEBUG_SERIAL.println(freq);
              
          }
          if (obj["total-consumption"].containsKey("ph-b"))
          {
              tcBReal_power = obj["total-consumption"]["ph-b"]["p"]; 
              tcBReactive_power = obj["total-consumption"]["ph-b"]["q"];  
              tcBApparent_power = obj["total-consumption"]["ph-b"]["s"];
              tcBvoltage = obj["total-consumption"]["ph-b"]["v"];
              tcBcurrent = obj["total-consumption"]["ph-b"]["i"];
              pf = obj["total-consumption"]["ph-b"]["pf"];
              freq = obj["total-consumption"]["ph-b"]["f"];
              
              if (tcBvoltage > 0) {
                  DEBUG_SERIAL.print("Phase B - ");
                  DEBUG_SERIAL.print("Real Power: ");
                  DEBUG_SERIAL.print(tcBReal_power);
                  DEBUG_SERIAL.print("\tReact Power: ");
                  DEBUG_SERIAL.print(tcBReactive_power);
                  DEBUG_SERIAL.print("\tApparent Power: ");
                  DEBUG_SERIAL.print(tcBApparent_power);
                  DEBUG_SERIAL.print("\tVoltage: ");
                  DEBUG_SERIAL.print(tcBvoltage);
                  DEBUG_SERIAL.print("\tCurrent: ");
                  DEBUG_SERIAL.print(tcBcurrent);  
                  DEBUG_SERIAL.print("\tPower Factor: ");
                  DEBUG_SERIAL.print(pf);  
                  DEBUG_SERIAL.print("\tFrequency: ");
                  DEBUG_SERIAL.println(freq);
              }
              else DEBUG_SERIAL.println("Phase B non connectée");
              
          }
          if (obj["total-consumption"].containsKey("ph-c"))
          {
              tcCReal_power = obj["total-consumption"]["ph-c"]["p"]; 
              tcCReactive_power = obj["total-consumption"]["ph-c"]["q"];  
              tcCApparent_power = obj["total-consumption"]["ph-c"]["s"];
              tcCvoltage = obj["total-consumption"]["ph-c"]["v"];
              tcCcurrent = obj["total-consumption"]["ph-c"]["i"];
              pf = obj["total-consumption"]["ph-c"]["pf"];
              freq = obj["total-consumption"]["ph-c"]["f"];
              
              if (tcCvoltage >0) {
                  DEBUG_SERIAL.print("Phase C - ");
                  DEBUG_SERIAL.print("Real Power: ");
                  DEBUG_SERIAL.print(tcCReal_power);
                  DEBUG_SERIAL.print("\tReact Power: ");
                  DEBUG_SERIAL.print(tcCReactive_power);
                  DEBUG_SERIAL.print("\tApparent Power: ");
                  DEBUG_SERIAL.print(tcCApparent_power);
                  DEBUG_SERIAL.print("\tVoltage: ");
                  DEBUG_SERIAL.print(tcCvoltage);
                  DEBUG_SERIAL.print("\tCurrent: ");
                  DEBUG_SERIAL.print(tcCcurrent);  
                  DEBUG_SERIAL.print("\tPower Factor: ");
                  DEBUG_SERIAL.print(pf);  
                  DEBUG_SERIAL.print("\tFrequency: ");
                  DEBUG_SERIAL.println(freq);
              }
              else DEBUG_SERIAL.println("Phase C non connectée");
              
          }
          
            tcAReal_power *= -1;
            tcBReal_power *= -1;
            tcCReal_power *= -1;
            Consumption= tcAReal_power + tcBReal_power + tcCReal_power;
            NetPower=Power+Consumption;
                    
              DEBUG_SERIAL.print("  Power Consumed: \t");
              DEBUG_SERIAL.println(Consumption);
              DEBUG_SERIAL.print("\t\t Net Power: \t");
              DEBUG_SERIAL.println(Power + Consumption);
              DEBUG_SERIAL.println();                      
          
      }
      else {
        DEBUG_SERIAL.println("No Consumption Data");
        return 0;
        }
  return 1;
 }
