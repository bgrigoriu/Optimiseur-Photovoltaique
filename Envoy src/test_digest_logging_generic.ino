
#include <ESP8266WiFi.h>

#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

#ifndef STASSID
#define STASSID "YourSSID"
#define STAPSK  "YourNetworkpassword"
#endif

const char* ssid = STASSID;
const char* ssidPassword = STAPSK;

const char *username = "installer";
const char *password = "YourEnvoyPassword";

const char *server = "http://192.168.1.4";  // Put here the IP of your envoy
const char *uri = "/stream/meter";

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

  WiFiClient client;
  HTTPClient http;                                  

void setup() {
  
  Serial.begin(74880);
  Serial.println("Begin");
  randomSeed(RANDOM_REG32);

  WiFi.mode(WIFI_STA);                      // define conection mode
  WiFi.begin(ssid, ssidPassword);             // initialize wifi network   
  
  while (WiFi.status() != WL_CONNECTED) {  // wait to be connected
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {

  Serial.print("[HTTP] begin...\n");  
  http.begin(client, String(server) + String(uri));   // configure traged server and url

  const char *keys[] = {"WWW-Authenticate"};
  http.collectHeaders(keys, 1);

  Serial.print("[HTTP] GET...\n");                    // Starting first GET - authentication only
  // start connection and send HTTP header
  
  int httpCode = http.GET();
    if (httpCode > 0) {
      String authReq = http.header("WWW-Authenticate");
      String authorization = getDigestAuth(authReq, String(username), String(password), "GET", String(uri), 1);
      http.end();
    
      http.begin(client, String(server) + String(uri));
      http.addHeader("Authorization", authorization);

      int httpCode = http.GET();           // Second GET - Actually getting the payload here
      if (httpCode == 200) {
         WiFiClient *cl = http.getStreamPtr();
        int error = 0;
        do {
          cl->find("data: ");
          String payload = cl->readStringUntil('\n');
          error = processingJsondata(payload);
          client.flush();
        } while (error);
      } else {
            Serial.print("[HTTP] GET... failed, error: ");
            Serial.println(http.errorToString(httpCode).c_str());
        }
    } else {
          Serial.print("[HTTP] GET... failed, error: ");
          Serial.println(http.errorToString(httpCode).c_str());
      }

  http.end();
  delay(3000);
}


// Functions for Digest authorisation

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

//  Serial.println(authorization);

  return authorization;
}
// End of functions for digest authorisation





// Routine de gestion et impression des donnes JSON

int processingJsondata(String payload) {

DynamicJsonDocument doc(2000);          // Added from envoy exemple
     
// Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, payload);
      
  // Test if parsing does not succeeds.
  if (error!=DeserializationError::Ok) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
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
              
                  Serial.print("\nPhase A - ");
                  Serial.print("Real Power: ");
                  Serial.print(AReal_power);
                  Serial.print("\tReact Power: ");
                  Serial.print(AReactive_power);
                  Serial.print("\tApparent Power: ");
                  Serial.print(AApparent_power);
                  Serial.print("\tVoltage: ");
                  Serial.print(Avoltage);
                  Serial.print("\tCurrent: ");
                  Serial.print(Acurrent);  
                  Serial.print("\tPower Factor: ");
                  Serial.print(pf);  
                  Serial.print("\tFrequency: ");
                  Serial.println(freq);
              
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
                  Serial.print("Phase B - ");
                  Serial.print("Real Power: ");
                  Serial.print(BReal_power);
                  Serial.print("\tReact Power: ");
                  Serial.print(BReactive_power);
                  Serial.print("\tApparent Power: ");
                  Serial.print(BApparent_power);
                  Serial.print("\tVoltage: ");
                  Serial.print(Bvoltage);
                  Serial.print("\tCurrent: ");
                  Serial.print(Bcurrent);  
                  Serial.print("\tPower Factor: ");
                  Serial.print(pf);  
                  Serial.print("\tFrequency: ");
                  Serial.println(freq);
              }
              else Serial.println("Phase B non connectée");
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
                  Serial.print("Phase C - ");
                  Serial.print("Real Power: ");
                  Serial.print(CReal_power);
                  Serial.print("\tReact Power: ");
                  Serial.print(CReactive_power);
                  Serial.print("\tApparent Power: ");
                  Serial.print(CApparent_power);
                  Serial.print("\tVoltage: ");
                  Serial.print(Cvoltage);
                  Serial.print("\tCurrent: ");
                  Serial.print(Ccurrent);  
                  Serial.print("\tPower Factor: ");
                  Serial.print(pf);  
                  Serial.print("\tFrequency: ");
                  Serial.println(freq);
              }
              else Serial.println("Phase C non connectée");
          }
          
          Power = AReal_power+BReal_power+CReal_power;
                    
              Serial.print("\n  Power Produced:");
              Serial.print(Power);
              Serial.print("\tCurrent: ");
              Serial.print((Acurrent+Bcurrent+Ccurrent));
              Serial.print("\tPowerFactor:");
              Serial.print(pf);
              Serial.print("\tFrequency:");
              Serial.print(freq);
              Serial.println();
          
      } else {
            Serial.println("No production data");
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
              
                  Serial.print("\nPhase A - ");
                  Serial.print("Real Power: ");
                  Serial.print(tcAReal_power);
                  Serial.print("\tReact Power: ");
                  Serial.print(tcAReactive_power);
                  Serial.print("\tApparent Power: ");
                  Serial.print(tcAApparent_power);
                  Serial.print("\tVoltage: ");
                  Serial.print(tcAvoltage);
                  Serial.print("\tCurrent: ");
                  Serial.print(tcAcurrent);  
                  Serial.print("\tPower Factor: ");
                  Serial.print(pf);  
                  Serial.print("\tFrequency: ");
                  Serial.println(freq);
              
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
                  Serial.print("Phase B - ");
                  Serial.print("Real Power: ");
                  Serial.print(tcBReal_power);
                  Serial.print("\tReact Power: ");
                  Serial.print(tcBReactive_power);
                  Serial.print("\tApparent Power: ");
                  Serial.print(tcBApparent_power);
                  Serial.print("\tVoltage: ");
                  Serial.print(tcBvoltage);
                  Serial.print("\tCurrent: ");
                  Serial.print(tcBcurrent);  
                  Serial.print("\tPower Factor: ");
                  Serial.print(pf);  
                  Serial.print("\tFrequency: ");
                  Serial.println(freq);
              }
              else Serial.println("Phase B non connectée");
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
                  Serial.print("Phase C - ");
                  Serial.print("Real Power: ");
                  Serial.print(tcCReal_power);
                  Serial.print("\tReact Power: ");
                  Serial.print(tcCReactive_power);
                  Serial.print("\tApparent Power: ");
                  Serial.print(tcCApparent_power);
                  Serial.print("\tVoltage: ");
                  Serial.print(tcCvoltage);
                  Serial.print("\tCurrent: ");
                  Serial.print(tcCcurrent);  
                  Serial.print("\tPower Factor: ");
                  Serial.print(pf);  
                  Serial.print("\tFrequency: ");
                  Serial.println(freq);
              }
              else Serial.println("Phase C non connectée");
          }
          
            tcAReal_power *= -1;
            tcBReal_power *= -1;
            tcCReal_power *= -1;
            Consumption= tcAReal_power + tcBReal_power + tcCReal_power;
          
          
              Serial.print("  Power Consumed: \t");
              Serial.println(Consumption);
              Serial.print("\t\t Net Power: \t");
              Serial.println(Power + Consumption);
              Serial.println();                      
          NetPower=Power+Consumption;
      }
      else {
        Serial.println("No Consumption Data");
        return 0;
        }
  return 1;
 }
