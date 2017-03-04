#include <SoftwareSerial.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <FS.h>

#include "SerialNMEA.h"
#include "Conf-EEPROM.h"

void sendUDPpacket(SerialNMEA * source);

// DEBUG ne peut être défini que si on utilise le SoftwareSerial et non le port série normal
//#define DEBUG

SoftwareSerial ComPort1 (4, SW_SERIAL_UNUSED_PIN, false, 256);
//#define ComPort1 Serial
SoftwareSerial ComPort2 (14, SW_SERIAL_UNUSED_PIN, false, 256);

SerialNMEA Port1(&ComPort1, &sendUDPpacket);
SerialNMEA Port2(&ComPort2, &sendUDPpacket);

#ifdef DEBUG
#define VERBEUX
//#define CLIENT_WIFI
#define TRACE(msg) Serial.print(msg);
#define TRACE_LN(msg) Serial.println(msg);
#else
#define TRACE
#define TRACE_LN
#endif

ConfEEPROM ParamModule;

WiFiClient client;
WiFiUDP udp;

ESP8266WebServer webserver(80);  // this just sets portNo nothing else happens until begin() is called
ESP8266HTTPUpdateServer httpUpdater;

String strHTMLConfig;
IPAddress AdresseUDP;          // openCPN system server network broadcast

void setup() {
  EEPROM.begin(512);

  #ifdef DEBUG
  Serial.begin (74880);
  delay (10);
  Serial.println ();
  #endif

  // Start file system
  SPIFFS.begin();

  // Get module configuration
  ParamModule.LitDeEEPROM();

  // Initialize Com port
  TRACE ("Vitesse port NMEA: ");
  TRACE_LN (String(ParamModule.BaudRate));
  ComPort1.begin (ParamModule.BaudRate);
  ComPort2.begin (ParamModule.BaudRate);

  // set-up OTA firmaware updater on 10.0.01/update
  httpUpdater.setup(&webserver);

  // set-up Wifi connection
  if (!ParamModule.ConfOk) {
    setupAP("NMEAWifi", "");
  }
  else {
    setupAP(ParamModule.SSID, ParamModule.Password);
  }

  // Print the IP address
  TRACE("Adresse IP Module: ");
  TRACE_LN(WiFi.localIP());
  TRACE("UDP send address: ");
  TRACE(AdresseUDP.toString());
  TRACE(":");
  TRACE_LN(ParamModule.PortUDP);

  TRACE_LN("Starting UDP");
  udp.begin(ParamModule.PortUDP);
  TRACE("Local port: ");
  TRACE_LN(udp.localPort());

  TRACE_LN("Fin setup()"); // end of setup

}  // setup

   //===================================================================
void setupAP(String ssid_wifi, String password_wifi) {

#ifndef CLIENT_WIFI
  TRACE_LN(F("Creation Serveur Wifi"));
  TRACE("AP: ");
  TRACE(ssid_wifi);
  TRACE(" Pw: ");
  TRACE_LN( password_wifi);

  if (password_wifi.length () < 8) {
    // on supprime le mot de passe sinon on ne change pas le nom réseau
    WiFi.softAP(ssid_wifi.c_str());
  } else {
    WiFi.softAP(ssid_wifi.c_str(), password_wifi.c_str());  
  }
  WiFi.begin ();
  
  IPAddress local_ip = IPAddress(10, 0, 0, 1);
  IPAddress gateway_ip = IPAddress(10, 0, 0, 1);
  IPAddress subnet_ip = IPAddress(255, 255, 255, 0);
  WiFi.softAPConfig(local_ip, gateway_ip, subnet_ip);

  AdresseUDP = WiFi.softAPIP(); // find my local IP address
  AdresseUDP[3] = 255;         // Set last octet to 255 for Class C broadcast address of this subnet

  #ifdef DEBUG
  TRACE_LN("OK");
  IPAddress myIP = WiFi.softAPIP();
  TRACE(F("Address serveur Wifi: "));
  TRACE_LN(myIP);
  #endif
#else
  const char * ReseauWifi= "SSID of your network";
  const char * mdpReseauWifi = "PW of your network";
  TRACE(F("Connexion au reseau "));
  TRACE_LN(ReseauWifi);

  WiFi.mode(WiFiMode::WIFI_STA);    // Arrête le mode AP au cas où au boot précédent, il aurait été en AP
  WiFi.begin(ReseauWifi, mdpReseauWifi);
  for (int C = 0; (C < 60) && (WiFi.status() != WL_CONNECTED); C++) {
    delay(500);
    TRACE(".");
  }
  TRACE_LN(String("Connecte, adresse IP donne par le DHCP: ") + WiFi.localIP().toString());
  AdresseUDP = WiFi.localIP(); // find my local IP address
  AdresseUDP[3] = 255;         // Set last octet to 255 for Class C broadcast address of this subnet
#endif

  strHTMLConfig = "<html>"
    "<head>"
    "<title>Paramètrage NMEA-Wifi</title>"
    "<meta charset=\"utf-8\" />"
    "<meta name=viewport content=\"width=device-width, initial-scale=1\">"
    "</head>"
    "<body>"
    "<h2>Paramètrage NMEA-Wifi</h2>"
    "<p>Cette page permet de paramétrer le connexion au pont NMEA-Wifi<p>"
    "Normalement, le pont est bien configuré pour le bateau, il ne faut donc pas changer sa configuration.</p>";
  strHTMLConfig += String("<p>Note: UDP packets will be sent to the broadcast IP of your class C network. ie.: ") + AdresseUDP.toString() + ":" + ParamModule.PortUDP + "</p>";

  if (ParamModule.ConfOk) {
    strHTMLConfig += "<p>Configuration du Module Ok";
  }
  else {
    strHTMLConfig += "<p>Configuration enregistrée du Module <b>en Erreur</b>";
  }

  strHTMLConfig +=
    "<form class=\"form\" method=\"post\" action=\"/storeconfig\" >"
    "<p class=\"name\">"
    "<label for=\"name\">SSID réseau Wifi</label><br>";
  strHTMLConfig += String("<input type=\"text\" name=\"SSID\" pattern=\"{1,32}\" title=\"Nom réseau Wifi entre 1 et 32 acractères\" id=\"ssid\" size=\"32\" placeholder=\"Nom du réseau Wifi\" value=\"") + ParamModule.SSID + "\" required />";

  strHTMLConfig +=
    "</p>"
    "<p class=\"password\">"
    "<label for=\"password\">Mot de Passe</label><br>";

  strHTMLConfig += String("<input type=\"text\" name=\"PassWord\" id=\"password\" pattern=\"{0,32}\" title=\"Mot de passe Wifi soit vide, soit entre 8 et 32 caractères\" size=\"32\" placeholder=\"Mot de passe Wifi\" autocomplete=\"off\" value=\"") + 
          ParamModule.Password + "\"onblur='var toto=document.getElementById(\"password\");if ((toto.value.length != 0) && ((toto.value.length <8) || (toto.value.length >32))) {alert (\"Le mot de passe doit être vide ou avec un nombre de caractères entre 8 et 32\")}'/>";

  strHTMLConfig += "</p>"
    "<p class=\"portNo\">"
    "<label for=\"portNo\">Port de diffusion UDP.</label><br>";
  strHTMLConfig += String("<input type=\"text\" name=\"UDPPort\" id=\"portNo\" placeholder=\"Port UDP\"  value=\"") + ParamModule.PortUDP + "\" required"
    " pattern=\"\\b([0-9]{1,4}|[1-5][0-9]{4}|6[0-4][0-9]{3}|65[0-4][0-9]{2}|655[0-2][0-9]|6553[0-5])\\b\" />"
    "</p>"
    "<p class=\"baudRate\">"
    "<label for=\"baudRate\">Vitesse port Série NMEA.</label><br>"
    "<select name=\"baudRate\" id=\"baudRate\" required>";

  const char * BaudsSupportes[] = { "4800", "9600", "38400", "115200" };

  for (int C = 0; C < 4; C++) {
    strHTMLConfig += String("<option value=\"") + BaudsSupportes[C] + "\"";
    if (atoi(BaudsSupportes[C]) == ParamModule.BaudRate) {
      strHTMLConfig += " selected";
    }
    strHTMLConfig += String(">") + BaudsSupportes[C] + "</option>";
  }
  strHTMLConfig += "</select>"
    "</p>"
    "<p class=\"submit\">"
    "<input type=\"submit\" value=\"Enregistre\"  />"
    "</p>"
    "</form>"
    "</body>"
    "</html>";

  delay(100);

  webserver.on("/", handleRoot);
  webserver.on("/config", handleConfig);
  webserver.on("/storeconfig", handleStoreConfig);
  webserver.on("/reboot", HTTP_GET, []() {
    webserver.send(200, "text/html", "<html><head><meta http-equiv='Refresh' content='2; url=/'><meta http-equiv=\"Content-type\" content=\"text/html; charset=utf-8\">" \
      "<title>Reboot lancé</title></head>"\
      "<body><h1 style='text-align:center'>Reboot en cours</h1></body></html>");
    ESP.restart();
    delay(100);
  });

  webserver.onNotFound([]() {
    if (!handleFileRead(webserver.uri()))
      handleNotFound ();
  });

  webserver.begin();

  TRACE_LN("HTTP webserver started");
}  //end of setupAp


unsigned long int momentDernierEnvoiUDP = 0;
unsigned long int nbUDPPaketSent = 0;           // the number of UDP packets sent

//********************************************************
// send the unidirectional UDP buffer to the specific address and preconfigured port
void sendUDPpacket(SerialNMEA * source)
{
  #ifdef DEBUG
  #ifdef VERBEUX
  TRACE("sending NTP packet length=");
  TRACE_LN(source->len);
  #else
  TRACE_LN(String (nbUDPPaketSent) + " pck env");
  #endif
  if (source->len > 100) {
    TRACE ("Packet trop long");
    TRACE_LN (String ((char *)source->packetBuffer));
  }
  #endif
  udp.beginPacket(AdresseUDP, ParamModule.PortUDP);
  udp.write(source->packetBuffer, source->len);
  udp.endPacket();
  nbUDPPaketSent++;
  momentDernierEnvoiUDP = millis ();
}


//*******************************************************************
//===================================================================
void loop() {
  
  Port1.LitPort ();          // check for more NMEA data
  Port2.LitPort ();          // check for more NMEA data

  webserver.handleClient();  // check if client wants a webpage
  yield();
}


//==============================================================================
void handleRoot () {
  unsigned long int Maintenant = millis();

  String strHTMLRoot =
    "<html>"
    "<head>"
        "<title>Etat pont NMEA-Wifi</title>"
        "<meta charset=\"utf-8\" />"
        "<meta name=viewport content=\"width=device-width, initial-scale=1\">"
    "</head>"
    "<body>";

  strHTMLRoot +=  String ("<center> <h3> Pour que votre logiciel de navigation se connecte choisir : <ul><li>une connexion réseau UDP</li> <li>une adresse localhost</li> <li>un port ") + ParamModule.PortUDP + "</li></ul></h3>";
  strHTMLRoot +=  "<br/>";
  strHTMLRoot +=  "<a href='ConfigurationMaxsea.pdf'> Pour la configuation Maxsea TimeZero cliquer ici</a><br/>";
  strHTMLRoot +=  "<a href='ConfigurationOpenCPN.pdf'> Pour la configuation OpenCPN cliquer ici</a><br/>";
  strHTMLRoot +=  "<br/><br/> <h3>Etat module</h3>";
  if (Port1.momentDernierReceptionNMEA == 0) {
    strHTMLRoot +=  String ("<p> Jamais rien reçu du port NMEA");
  } else {
    strHTMLRoot +=  String ("<p> Derniers caractères NMEA reçus il y a ") + (Maintenant - Port1.momentDernierReceptionNMEA) + "ms";
  }
  if (momentDernierEnvoiUDP == 0) {
    strHTMLRoot +=  String ("<p> Jamais rien envoyé sur le Wifi");
  } else {
    strHTMLRoot +=  String ("<p> Dernier envoir Wifi il y a ") + (Maintenant - momentDernierEnvoiUDP) + "ms";
    strHTMLRoot +=  String ("<p> Nb packets envoyés par Wifi ") + nbUDPPaketSent;
  }

  if ((Port1.nbTramesNMEATropLongues + Port2.nbTramesNMEATropLongues) == 0) {
    strHTMLRoot += "<p>Pas d'erreur";
  } else {
    strHTMLRoot += String ("<p>") + (Port1.nbTramesNMEATropLongues + Port2.nbTramesNMEATropLongues) + " trames trop longues";
  }
  strHTMLRoot += 
     "</center> </body>"
     "</html>";   

   webserver.send(200, "text/html", strHTMLRoot);
}

//==============================================================================
void handleConfig() {
  webserver.send(200, "text/html", strHTMLConfig);
}

//==============================================================================
void handleNotFound() {
  String strHTML = "Page inexistante\n\n";
  strHTML += "URI: ";
  strHTML += webserver.uri();
  strHTML += "\nMethod: ";
  strHTML += (webserver.method() == HTTP_GET) ? "GET" : "POST";
  strHTML += "\nArguments: ";
  strHTML += webserver.args();
  strHTML += "\n";

  for (uint8_t i = 0; i < webserver.args(); i++) {
    strHTML += " " + webserver.argName(i) + ": " + webserver.arg(i) + "\n";
  }

  webserver.send(404, "text/plain", strHTML);
}

//==============================================================================
void handleStoreConfig() {

  if (webserver.args() == 0) {
    webserver.send(200, "text/html", strHTMLConfig);
    return;
  }

  #ifdef DEBUG
  String strConfRecue = "Config results\n\n";
  strConfRecue += "URI: ";
  strConfRecue += webserver.uri();
  strConfRecue += "\nMethod: ";
  strConfRecue += (webserver.method() == HTTP_GET) ? "GET" : "POST";
  strConfRecue += "\nArguments: ";
  strConfRecue += webserver.args();
  strConfRecue += "\n";

  for (uint8_t i = 0; i < webserver.args(); i++) {
    strConfRecue += " " + webserver.argName(i) + ": " + webserver.arg(i) + "\n";
  }
  TRACE_LN(strConfRecue);
  #endif

  ConfEEPROM confTemp;

  char ssid[ConfEEPROM::MAX_SSID_LEN + 1];
  char password[ConfEEPROM::MAX_PASSWORD_LEN + 1];
  long tmpUDPPort;
  long tmpBaudRate;

  uint8_t numOfArgs = webserver.args();
  for (uint8_t i = 0; (i < numOfArgs); i++) {
    if (webserver.argName(i) == "SSID") {
      strncpy_safe(confTemp.SSID, webserver.arg(i).c_str(), ConfEEPROM::MAX_SSID_LEN);
      urldecode2(confTemp.SSID, confTemp.SSID); // result length is always <= source so just copy over
    }
    else if (webserver.argName(i) == "PassWord") {
      String Pwd = String (webserver.arg(i));
      if ((Pwd.length () < 8) || (Pwd.length () > 32)) {
        Pwd = "";
      }
      strncpy_safe(confTemp.Password, Pwd.c_str(), ConfEEPROM::MAX_PASSWORD_LEN);
      urldecode2(confTemp.Password, confTemp.Password); // result length is always <= source so just copy over
    }
    else if (webserver.argName(i) == "UDPPort") {
      confTemp.PortUDP = webserver.arg(i).toInt();
    }
    else if (webserver.argName(i) == "baudRate") {
      confTemp.BaudRate = webserver.arg(i).toInt();
    }
  }

  String rtnMsg = "<html>"
    "<title>NMEA WiFi Bridge Setup</title>"
    "<meta charset=\"utf-8\" />"
    "<meta name=viewport content=\"width=device-width, initial-scale=1\">"
    "</head>"
    "<body>";

  if (confTemp.EstValide()) {
    ParamModule = confTemp;
    ParamModule.EcritDansEEPROM();
    rtnMsg += "<h3>Configuration sauvegardée:</h3><center>";
    rtnMsg += String("<br>SSID: ") + ParamModule.SSID;
    rtnMsg += String("<br>Mot de passe: ") + ParamModule.Password;
    rtnMsg += String("<br>Port UDB: ") + ParamModule.PortUDP;
    rtnMsg += String("<br>vitesse port NMEA: ") + ParamModule.BaudRate;
    rtnMsg += "</center>";
    rtnMsg += "<br><br><br><a href='./reboot'>Relancer le module pour prendre en compte la nouvelle configuration</a>";

  }
  else {
    rtnMsg += "<h1> Configuration invalide !</h1><br>"
      "il faut la refaire"
      "<a href=\"javascript:history.back()\">Retour</a>";
  }
  rtnMsg +=
    "</body>"
    "</html>";
  webserver.send(200, "text/html", rtnMsg);
}


//==============================================================================
// Tools
//==============================================================================
#include <stdlib.h>
#include <ctype.h>

//==============================================================================
size_t strncpy_safe(char* dest, const char* src, size_t maxLen) {
  size_t rtn = 0;
  if (src == NULL) {
    dest[0] = '\0';
  }
  else {
    strncpy(dest, src, maxLen);
    rtn = strlen(src);
    if (rtn > maxLen) {
      rtn = maxLen;
    }
  }
  dest[maxLen] = '\0';
  return rtn;
}

void urldecode2(char *dst, const char *src) {
  char a, b, c;
  if (dst == NULL) return;
  while (*src) {
    if (*src == '%') {
      if (src[1] == '\0') {
        // don't have 2 more chars to handle
        *dst++ = *src++; // save this char and continue
                 // next loop will stop
        continue;
      }
    }
    if ((*src == '%') &&
      ((a = src[1]) && (b = src[2])) &&
      (isxdigit(a) && isxdigit(b))) {
      // here have at least src[1] and src[2] (src[2] may be null)
      if (a >= 'a')
        a -= 'a' - 'A';
      if (a >= 'A')
        a -= ('A' - 10);
      else
        a -= '0';
      if (b >= 'a')
        b -= 'a' - 'A';
      if (b >= 'A')
        b -= ('A' - 10);
      else
        b -= '0';
      *dst++ = 16 * a + b;
      src += 3;
    }
    else {
      c = *src++;
      if (c == '+')c = ' ';
      *dst++ = c;
    }
  }
  dst = '\0'; // terminate result
}

//==============================================================================

bool handleFileRead(String path) {
  TRACE_LN("handleFileRead: " + path);
  //if (path.endsWith("/")) path += "index.htm";
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) {
    if (SPIFFS.exists(pathWithGz))
      path += ".gz";
    File file = SPIFFS.open(path, "r");
    webserver.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

String getContentType(String filename) {
  if (webserver.hasArg("download")) return "application/octet-stream";
  else if (filename.endsWith(".htm")) return "text/html";
  else if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".gif")) return "image/gif";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".xml")) return "text/xml";
  else if (filename.endsWith(".pdf")) return "application/x-pdf";
  else if (filename.endsWith(".zip")) return "application/x-zip";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  else if (filename.endsWith(".pdf")) return "application/pdf";
  
  return "text/plain; charset=UTF-8";
}

