#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <ArduinoJson.h>  // Include the ArduinoJson library

ESP8266WebServer server(80);

const char* defaultSsid = "esp8266";
const char* defaultPassword = "1234567890";

#define EEPROM_SIZE 64
#define SSID_ADDR 0
#define PASS_ADDR 32

void saveCredentials(const char* ssid, const char* password) {
  if (strlen(ssid) > 32 || strlen(password) > 32) {
    Serial.println("SSID or password exceeds maximum length of 32 bytes.");
    return;
  }
  StaticJsonDocument<EEPROM_SIZE> doc;
  doc["ssid"] = ssid;
  doc["password"] = password;

  // Serialize the JSON object to a string
  char jsonBuffer[EEPROM_SIZE];
  serializeJson(doc, jsonBuffer);

  // Write the JSON string to EEPROM
  EEPROM.begin(EEPROM_SIZE);
  for (int i = 0; i < strlen(jsonBuffer); ++i) {
    EEPROM.write(i, jsonBuffer[i]);
  }
  EEPROM.commit();
  EEPROM.end();
  Serial.printf("\nsaveCredentials ==> SSID: %s, Password: %s\n", ssid, password);
}

void loadCredentials(char* ssid, char* password) {

  EEPROM.begin(EEPROM_SIZE);
  char jsonBuffer[EEPROM_SIZE];
  for (int i = 0; i < EEPROM_SIZE; ++i) {
    jsonBuffer[i] = EEPROM.read(i);
  }
  EEPROM.end();

  // Parse the JSON string
  StaticJsonDocument<EEPROM_SIZE> doc;
  DeserializationError error = deserializeJson(doc, jsonBuffer);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }

  // Extract SSID and password from JSON
  const char* storedSsid = doc["ssid"];
  const char* storedPassword = doc["password"];

  // Copy SSID and password to output variables
  strncpy(ssid, storedSsid, EEPROM_SIZE);
  strncpy(password, storedPassword, EEPROM_SIZE);
  Serial.printf("\nloadCredentials ==> SSID: %s, Password: %s\n", ssid, password);
}

void handleRoot() {
  server.send(200, "text/html", "<h1>Welcome to ESP8266 Configuration Page</h1><p><a href='/config'>Configure Wi-Fi</a></p>");
}

void handleConfig() {
  if (server.hasArg("ssid") && server.hasArg("password")) {
    String newSsid = server.arg("ssid");
    String newPassword = server.arg("password");
    saveCredentials(newSsid.c_str(), newPassword.c_str());
    server.send(200, "text/html", "<h1>Wi-Fi credentials updated successfully!</h1>");
    setup();
  } else {
    server.send(200, "text/html", "<h1>Configure Wi-Fi</h1><form method='POST' action='/config'><input type='text' name='ssid' placeholder='New SSID'><br><input type='password' name='password' placeholder='New Password'><br><input type='submit' value='Save'></form>");
  }
}

void setup() {
  Serial.begin(115200);

  char ssid[EEPROM_SIZE];
  char password[EEPROM_SIZE];

  loadCredentials(ssid, password);
  
  if (strlen(ssid)>1) {
    Serial.print("\nTring Connect to Wi-Fi with ssid: ");
    Serial.print(ssid);
    Serial.print(" password: ");
    Serial.println(password);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid,password);
    WiFi.setHostname("esp8266");
    int i=0;
    while (WiFi.status() != WL_CONNECTED && i < 5) {
    delay(5000);
    Serial.printf("\nConnecting to Wi-Fi %s %s\n",ssid,password);
    i++;
  }
  }
  if(WiFi.status() == WL_CONNECTED){
    Serial.printf("\nConnected to Wi-Fi %s\n",ssid);
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    // Proceed with normal operation
  } else {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(defaultSsid, defaultPassword);
    IPAddress softAPIP =WiFi.softAPIP();
    Serial.println("Failed to connect to Wi-Fi, Work as Access Point Mode");
    Serial.print("SoftAP IP Address: ");
    Serial.println(softAPIP);
    Serial.println("==================");
    
  }
    // Set up web server routes
    server.on("/", handleRoot);
    server.on("/config", handleConfig);
    server.begin();
    Serial.println("Web Server Startd");
}

void loop() {
  server.handleClient();
}
