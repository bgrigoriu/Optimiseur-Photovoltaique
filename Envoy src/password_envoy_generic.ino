
#define serialNumber "YourEnvoySerialnumner"
#define userName "installer"

#define realm "enphaseenergy.com"

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
//---------------------------------------------------------------

// fonction de calcul du mot de passe
String emupwGetMobilePasswd(String serial){

String digest;
int countZero = 0;
int countOne =  0;
digest = emupwGetPasswdForSn(serial,userName,realm);
int len = digest.length();
for (int i=0 ; i<len; i++) {
  if (digest.charAt(i) == char(48)) countZero++;
  if (digest.charAt(i) == char(49)) countOne++;
  }
  String password ="";
 for (int i=1; i < 9; ++i) {
  Serial.println(i);
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


void setup() {
  // put your setup code here, to run once:
delay(1000);
Serial.begin(74880);
delay(1000);
Serial.println("Begin");
Serial.println(emupwGetPasswdForSn(serialNumber, userName, realm));
Serial.println(emupwGetMobilePasswd(serialNumber));

}

void loop() {
  // put your main code here, to run repeatedly:

}
