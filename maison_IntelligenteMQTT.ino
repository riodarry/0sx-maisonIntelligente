#pragma region Librairies et définitions
#include <LCD_I2C.h>
#include <HCSR04.h>
#include <U8g2lib.h>
#include <WiFiEspAT.h>
#include <PubSubClient.h>
#include <DHT.h>
#include "Alarm.h"
#include "PorteAutomatique.h"

#define HAS_SECRETS 0
#if HAS_SECRETS
#include "arduino_secrets.h"
const char ssid[] = SECRET_SSID;
const char pass[] = SECRET_PASS;
#else
const char ssid[] = "TechniquesInformatique-Etudiant";
const char pass[] = "shawi123";
#endif

#define TRIGGER_PIN 12
#define ECHO_PIN 13
#define BUZZER_PIN 22
#define RED_PIN 11
#define GREEN_PIN 10
#define BLUE_PIN 9

#define IN_1 41
#define IN_2 37
#define IN_3 35
#define IN_4 39

#define CLK_PIN 29
#define DIN_PIN 25
#define CS_PIN 27

#define DHT_PIN 7
#define DHT_TYPE DHT11

#define MQTT_PORT 1883
#define MQTT_USER "etdshawi"
#define MQTT_PASS "shawi123"
#define DEVICE_NAME "2409626"
const char* mqttServer = "216.128.180.194";

WiFiClient wifiClient;
PubSubClient client(wifiClient);
#pragma endregion

#pragma region Objets globaux
float distance = 0;
unsigned long currentTime = 0;
char lcdBuff[2][16];
bool debug = true;
bool wifiOk = false;
bool mqttOk = false;

LCD_I2C lcd(0x27, 16, 2);
HCSR04 hc(TRIGGER_PIN, ECHO_PIN);
U8G2_MAX7219_8X8_F_4W_SW_SPI u8g2(U8G2_R0, CLK_PIN, DIN_PIN, CS_PIN, U8X8_PIN_NONE, U8X8_PIN_NONE);
DHT dht(DHT_PIN, DHT_TYPE);

Alarm alarm(RED_PIN, GREEN_PIN, BLUE_PIN, BUZZER_PIN, &distance);
PorteAutomatique porte(IN_1, IN_2, IN_3, IN_4, distance);
#pragma endregion

#pragma region Fonctions WiFi
void wifiInit() {
  Serial1.begin(115200);
  WiFi.init(&Serial1);

  int status = WiFi.begin(ssid, pass);
  if (status != WL_CONNECTED) {
    Serial.println(" Échec WiFi, tentative de reconnexion...");
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.print(".");
    }
  }
  wifiOk = true;
  Serial.println("\nConnecté au réseau WiFi.");
}
#pragma endregion

#pragma region MQTT couleur et motor 
void mqttEvent(char* topic, byte* payload, unsigned int length) {
  char message[length + 1];
  memcpy(message, payload, length);
  message[length] = '\0';

  if (strcmp(topic, "etd/26/color") == 0) {
    String msgStr = String(message);
    int idx = msgStr.indexOf('#');
    String hexColor = msgStr.substring(idx + 1, idx + 7);
    if (hexColor.length() != 6) {
      if (debug) Serial.println("Erreur : Couleur hexadécimale invalide.");
      return;
    }
    long hex = strtol(hexColor.c_str(), NULL, 16);
    int r = hex >> 16, g = (hex >> 8) & 0xFF, b = hex & 0xFF;
    alarm.setColourA(r, g, b);
    if (debug) Serial.println("Couleur RGB mise à jour via MQTT.");
  }else if (strcmp(topic, "etd/26/motor") == 0) {
  String messageStr = String((char*)message);
  int position = messageStr.indexOf(":");
  Serial.println(messageStr[position + 1]);
  if (messageStr[position + 1] == '1') {
    Serial.println("Tata");
    porte.activer();
    if (debug) Serial.println(" Moteur activé via MQTT.");
  } else if (messageStr[position + 1] == '0') {
    porte.desactiver();
    if (debug) Serial.println(" Moteur desactivé via MQTT.");
  } 
  
}

  // else if (strcmp(topic, "etd/26/motor") == 0) {
  //   if (message[0] == '1') porte.activer();
  //   else porte.desactiver();
  //   if (debug) Serial.println("Commande moteur reçue via MQTT.");
  // }
}

bool reconnect() {
  bool success = client.connect(DEVICE_NAME, MQTT_USER, MQTT_PASS);
  if (success) {
    mqttOk = true;
    if (debug) Serial.println(" Reconnexion MQTT réussie");
    client.subscribe("etd/26/color");
    client.subscribe("etd/26/motor");
  } else {
    if (debug) Serial.println(" Reconnexion MQTT échouée");
  }
  return success;
}
#pragma endregion

#pragma region Fonctions principales
void setup() {
  Serial.begin(115200);
  lcd.begin(); lcd.backlight();
  u8g2.begin(); u8g2.setFont(u8g2_font_4x6_tr); u8g2.setContrast(5);
  dht.begin();

  wifiInit();
  client.setServer(mqttServer, MQTT_PORT);
  client.setCallback(mqttEvent);
  if (!client.connect(DEVICE_NAME, MQTT_USER, MQTT_PASS)) {
    Serial.println("Échec MQTT");
  } else {
    mqttOk = true;
    Serial.println(" Connecté au serveur MQTT");
    client.subscribe("etd/26/color");
    client.subscribe("etd/26/motor");
  }

  porte.setAngleFerme(10);
  porte.setAngleOuvert(170);
  porte.setDistanceOuverture(20);
  porte.setDistanceFermeture(30);

  alarm.setColourA(255, 0, 0);
  alarm.setColourB(0, 0, 255);
  alarm.setDistance(15);
  alarm.setTimeout(3000);
  alarm.setVariationTiming(150);
  alarm.turnOn();

  lcd.setCursor(0, 0); lcd.print("2409626");
  lcd.setCursor(0, 1); lcd.print("Labo final");
  delay(2000); lcd.clear();
}
#pragma endregion

#pragma region Loop
void loop() {
  currentTime = millis();
  client.loop();

  updateDistance();
  porte.update();
  alarm.update();
  afficherLCD();
// afficherMatrice();
  periodicTask();
  periodicTaskLCD();
  serialEventTask();

  if (debug && (!wifiOk || !mqttOk)) {
    Serial.println("⏳ En attente de connexion WiFi/MQTT...");
  }
}
#pragma endregion

#pragma region lecture distance
void updateDistance() {
  static unsigned long last = 0;
  if (tempsEcoule(last, 50)) {
    float d = hc.dist();
    if (d > 0) distance = d;
  }
}

void afficherLCD() {
  static unsigned long last = 0;
  if (tempsEcoule(last, 1000)) {
    lcd.setCursor(0, 0);
    lcd.print("Dist: ");
    lcd.print(distance);
    lcd.print(" cm   ");

    lcd.setCursor(0, 1);
    lcd.print("Porte: ");
    lcd.print(porte.getEtatTexte());
    lcd.print("     ");
  }
}

void afficherMatrice() {
  static unsigned long last = 0;
  if (!tempsEcoule(last, 3000)) return;

  u8g2.clearBuffer();
  u8g2.drawLine(1, 5, 3, 7);
  u8g2.drawLine(3, 7, 7, 1);
  u8g2.sendBuffer();
}
#pragma endregion

#pragma region MQTT Periodic
void periodicTask() {
  static unsigned long last = 0;
  if (!tempsEcoule(last, 2500)) return;

  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  char payload[256], dStr[6], tStr[6], hStr[6], aStr[6];
  dtostrf(distance, 4, 1, dStr);
  dtostrf(temp, 4, 1, tStr);
  dtostrf(hum, 4, 1, hStr);
  dtostrf(porte.getAngle(), 4, 1, aStr);

  sprintf(payload,
    "{\"number\":\"2409626\",\"name\":\"Rio\",\"uptime\":%lu,"
    "\"dist\":%s,\"angle\":%s,\"temp\":%s,\"hum\":%s}",
    millis() / 1000, dStr, aStr, tStr, hStr); 

  if (!client.publish("etd/26/data", payload)) {
    reconnect();
  } else if (debug) {
    Serial.println("MQTT : données envoyées");
    Serial.println(payload);
  }
}

#pragma endregion

#pragma region Commandes série
String tampon = "";

void serialEventTask() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      tampon.trim();
      handleCommand(tampon);
      tampon = "";
    } else {
      tampon += c;
    }
  }
}
void afficherMatriceTemporaire(String texte, unsigned long duree = 2000);

void handleCommand(String cmd) {
  cmd.trim(); cmd.toLowerCase();

  if (cmd == "g_dist") {
    Serial.println(distance);
    afficherCheck();
  } else if (cmd.startsWith("cfg;alm;")) {
    int val = cmd.substring(8).toInt();
    if (val > 0) {
      alarm.setDistance(val);
      afficherCheck();
    } else {
      afficherErreur();
    }
  } else if (cmd.startsWith("cfg;lim_inf;")) {
    int val = cmd.substring(12).toInt();
    if (val >= porte.getDistanceFermeture()) {
      Serial.println(" Erreur – Limite inf >= sup");
      afficherErreurLimite();
    } else {
      porte.setDistanceOuverture(val);
      afficherCheck();
    }
  } else if (cmd.startsWith("cfg;lim_sup;")) {
    int val = cmd.substring(12).toInt();
    if (val <= porte.getDistanceOuverture()) {
      Serial.println(" Erreur – Limite sup <= inf");
      afficherErreurLimite();
    } else {
      porte.setDistanceFermeture(val);
      afficherCheck();
    }
  } else if (cmd == "turn_on") {
    alarm.turnOn();
    afficherCheck();
  } else if (cmd == "turn_off") {
    alarm.turnOff();
    afficherCheck();
  } else if (cmd == "ouvrir") {
    porte.activer();
    distance = 0;
    afficherCheck();
  } else if (cmd == "fermer") {
    porte.activer();
    distance = 100;
    afficherCheck();
  } else {
    Serial.println("Commande inconnue");
    afficherErreur();
  }
}




void periodicTaskLCD() {
  static unsigned long last = 0;
  static String lastL1 = "";
  static String lastL2 = "";

  if (!tempsEcoule(last, 1100)) return;

  String l1 = String("Dist: ") + String(distance) + "cm";
  String l2 = String("Porte: ") + porte.getEtatTexte();

  if (l1 != lastL1 || l2 != lastL2) {
    lastL1 = l1;
    lastL2 = l2;

    char message[200];
    sprintf(message, "{\"line1\":\"%s\", \"line2\":\"%s\"}", l1.c_str(), l2.c_str());
    if (!client.publish("etd/26/data", message)) {
      reconnect();
    } else if (debug) {
      Serial.println("MQTT : LCD envoyé");
      Serial.println(message);
    }
  }
}
#pragma endregion

#pragma region temps ecoule
bool tempsEcoule(unsigned long& dernier, unsigned long intervalle) {
  if (millis() - dernier >= intervalle) {
    dernier = millis();
    return true;
  }
  return false;
}
#pragma endregion
#pragma region Affichage temporaire matrice

void afficherMatriceTemporaire(String texte, unsigned long duree = 2000) {
  static unsigned long debutAffichage = 0;
  static bool enCours = false;

  if (!enCours) {
    u8g2.clearBuffer();
    u8g2.drawStr(0, 7, texte.c_str());
    u8g2.sendBuffer();
    debutAffichage = millis();
    enCours = true;
  }

  if (enCours && millis() - debutAffichage > duree) {
    u8g2.clear();
    enCours = false;
  }
}

#pragma endregion

void afficherCheck() {
  u8g2.clearBuffer();
  u8g2.drawLine(1, 5, 3, 7);
  u8g2.drawLine(3, 7, 7, 1);
 
  u8g2.sendBuffer();
  delay(3000);
  u8g2.clear();
}

void afficherErreur() {
  u8g2.clearBuffer();
 u8g2.drawCircle(3, 3, 3);
  u8g2.drawLine(0, 0, 7, 7);

  u8g2.sendBuffer();
  delay(3000);
  u8g2.clear();
}

void afficherErreurLimite() {
  u8g2.clearBuffer();
  u8g2.drawLine(7, 7, 0, 0);
   u8g2.drawLine(0, 7, 7, 0);

  u8g2.sendBuffer();
  delay(3000);
  u8g2.clear();
}

