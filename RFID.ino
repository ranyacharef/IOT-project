#include <SPI.h>
#include <MFRC522.h>
#include <ThingSpeak.h>
#define SS_PIN 21
#define RST_PIN 22
    
#define AccesFlag_PIN 32
#define Gate_PIN 12
#define Max_Acces 3

#if defined(ESP32)
#include <WiFi.h>
#include <FirebaseESP32.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#endif


// Provide the token generation process info.
#include <addons/TokenHelper.h>

// Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>

/* 1. Define the WiFi credentials */
#define WIFI_SSID "ISITcom_wifi_etud"
#define WIFI_PASSWORD "@isitcom@2022"
#define API_KEY "AIzaSyAWGqDeq3JHHL0VRHRQfA9LR9rXk89Pwrk"

/* 3. Define the user Email and password that already registerd or added in your project */
#define USER_EMAIL "sabbaghsarah1998@gmail.com"
#define USER_PASSWORD "sarah1998"

/* 4. If work with RTDB, define the RTDB URL */
#define DATABASE_URL "https://rfid-87f91-default-rtdb.europe-west1.firebasedatabase.app/" //<databaseName>.firebaseio.com or <databaseName>.<region>.firebasedatabase.app

#define DATABASE_SECRET "puttxe5R7zKPon300RsMfzhgEeiMrJvCSaAGq9I6"

// ThingSpeak information
/*char* writeAPIKey = "OPQ0368T9M0OQIUE";
char* readAPIKey = "21XMR3ZDYL85SHX5"; 
const long channelID = 1709430; */

/* 6. Define the Firebase Data object */
FirebaseData fbdo;

/* 7. Define the FirebaseAuth data for authentication data */
FirebaseAuth auth;

/* 8. Define the FirebaseConfig data for config data */
FirebaseConfig config;

unsigned long dataMillis = 0;
int count = 0;

byte Count_acces=0; 
byte CodeVerif=0; 
byte Code_Acces[4]={0x83, 0x1C, 0x67, 0xD};


MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class

// Init array that will store new NUID 
byte nuidPICC[4];

void setup() 
{ 
  // Init RS232
  Serial.begin(9600);

  // Init SPI bus
  SPI.begin(); 

  
   WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(300);
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();

    Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

    /* Assign the api key (required) */
    config.api_key = API_KEY;

    /* Assign the user sign in credentials */
    auth.user.email = USER_EMAIL;
    auth.user.password = USER_PASSWORD;

    /* Assign the RTDB URL */
    config.database_url = DATABASE_URL;

    Firebase.reconnectWiFi(true);
    fbdo.setResponseSize(4096);

    String base_path = "/UsersData/";

    /* Assign the callback function for the long running token generation task */
    config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

    /** Assign the maximum retry of token generation */
    config.max_token_generation_retry = 5;

    /* Initialize the library with the Firebase authen and config */
    Firebase.begin(&config, &auth);
    
    String var = "$userId";
    String val = "($userId === auth.uid && auth.token.premium_account === true && auth.token.admin === true)";
    Firebase.setReadWriteRules(fbdo, base_path, var, val, val, DATABASE_SECRET);


  // Init MFRC522 
  rfid.PCD_Init(); 

  // Init LEDs 
  pinMode(AccesFlag_PIN, OUTPUT);
  pinMode(Gate_PIN, OUTPUT);
  
  digitalWrite(AccesFlag_PIN, LOW);
  digitalWrite(Gate_PIN, LOW);
}
 
void loop() 
{
  /* if (millis() - dataMillis > 5000 && Firebase.ready())
    {
        dataMillis = millis();
        String path = "/UsersData/";
        path += auth.token.uid.c_str(); //<- user uid of current user that sign in with Emal/Password
        path += "/test/int";
        Serial.printf("Set int... %s\n", Firebase.setInt(fbdo, path, count++) ? "ok" : fbdo.errorReason().c_str());
    }*/
    /*int c=0; 
    c++;
    ThingSpeak.writeField(channelID, 1, c, writeAPIKey);*/
  // Initialisé la boucle si aucun badge n'est présent 
  if ( !rfid.PICC_IsNewCardPresent())
    return;

  // Vérifier la présence d'un nouveau badge 
  if ( !rfid.PICC_ReadCardSerial())
    return;

  // Afffichage 
  Serial.println(F("Un badge est détecté"));

  // Enregistrer l’ID du badge (4 octets) 
  for (byte i = 0; i < 4; i++) {
    nuidPICC[i] = rfid.uid.uidByte[i];
  }

  // Vérification du code 
  CodeVerif= GetAccesState(Code_Acces,nuidPICC); 
  if (CodeVerif!=1)
  {
    Count_acces+=1;
    if(Count_acces==Max_Acces)
    {
     // Dépassement des tentatives (clignotement infinie) 
     byte counte = 0;
     while(counte<30)
     {
      counte++;
      digitalWrite(AccesFlag_PIN, HIGH);
      delay(200); 
      digitalWrite(AccesFlag_PIN, LOW);
      delay(200); 
      // Affichage 
      Serial.println("Alarme!");
     }
    }
    else
    {
      // Affichage 
      Serial.println("Code érroné");
    
      // Un seul clignotement: Code erroné 
      digitalWrite(AccesFlag_PIN, HIGH);
      delay(1000); 
      digitalWrite(AccesFlag_PIN, LOW);
    }
  }
  else
  {
    
    // Affichage 
    Serial.println("Ouverture de la porte");
    String path2 = "/UsersData/";
    path2 += auth.token.uid.c_str(); //<- user uid of current user that sign in with Emal/Password
    path2 += "/test/badge";    // Ouverture de la porte & Initialisation 
    Firebase.setString(fbdo, path2, "83 1C 67 D");
    digitalWrite(Gate_PIN, HIGH);
    delay(3000); 
    digitalWrite(Gate_PIN, LOW);
    Count_acces=0; 
  }

  // Affichage de l'identifiant 
  Serial.println(" L'UID du tag est:");
  for (byte i = 0; i < 4; i++) 
  {
    Serial.print(nuidPICC[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  // Re-Init RFID
  rfid.PICC_HaltA(); // Halt PICC
  rfid.PCD_StopCrypto1(); // Stop encryption on PCD
}

byte GetAccesState(byte *CodeAcces,byte *NewCode) 
{
  byte StateAcces=0; 
  if ((CodeAcces[0]==NewCode[0])&&(CodeAcces[1]==NewCode[1])&&
  (CodeAcces[2]==NewCode[2])&& (CodeAcces[3]==NewCode[3]))
    return StateAcces=1; 
  else
    return StateAcces=0; 
}
