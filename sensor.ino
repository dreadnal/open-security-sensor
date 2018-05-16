#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

IPAddress    apIP(192, 168, 0, 1);

const char *ssid = "WebSecurity Sensor";
const char *password = "webSecSensor";

const int LED_PIN = D0;
const int CONFIG_PIN = D1;
const int SENSOR_PIN = D2;

ESP8266WebServer server(80);

boolean configMode = false;

int sensorStatus = LOW;

char remoteSSID[64];
char remotePassword[64];
char serverAddress[64];
char apiKey[64];
char sensorId[64];
char eventTypeId[64];

void handleRoot() {

  if (checkEEPROM()) {
    readString(2, 63, remoteSSID);
    readString(66, 63, remotePassword);
    readString(130, 63, serverAddress);
    readString(194, 63, apiKey);
    readString(258, 63, sensorId);
    readString(322, 63, eventTypeId);
  } else {
    remoteSSID[0] = 0;
    remotePassword[0] = 0;
    serverAddress[0] = 0;
    apiKey[0] = 0;
    sensorId[0] = 0;
    eventTypeId[0] = 0;
  }
  String content = "<html><head>";
  content += "<style>";
  content += "body{background: linear-gradient(180deg, rgb(5, 94, 88), rgb(245, 114, 33)); font-family: Arial, Helvetica, sans-serif;}";
  content += "form{background: white; display: grid; padding: 20px; border-radius: 10px;}";
  content += "label{color: rgb(5, 94, 88);}";
  content += "h1{color: white; text-align: center; margin-top: 20px;}";
  content += "input[type='submit']{background: white; border: 1px solid rgb(5, 94, 88); color: rgb(5, 94, 88); padding: 5px; cursor: pointer;}";
  content += "input[type='submit']:hover{background: rgb(5, 94, 88); color: white;}";
  content += "</style>";
  content += "</head><body>";
  content += "<h1>WebSecurity Sensor Configuration</h1>";
  content += "<form action='/save' method='POST'>";
  content += "<label>Network SSID: </label><input name='SSID' value='"+String(remoteSSID)+"'><br>";
  content += "<label>Network password: </label><input name='PASSWORD' value='"+String(remotePassword)+"'><br>";
  content += "<label>Server address: </label><input name='SERVER' value='"+String(serverAddress)+"'><br>";
  content += "<label>API key: </label><input name='API_KEY' value='"+String(apiKey)+"'><br>";
  content += "<label>Sensor id: </label><input name='SENSOR_ID' value='"+String(sensorId)+"'><br>";
  content += "<label>Event type id: </label><input name='EVENT_TYPE_ID' value='"+String(eventTypeId)+"'><br>";
  content += "<input type='submit' name='SUBMIT' value='Save'>";
  content += "</form>";
  content += "</body></html>";
  server.send(200, "text/html", content);
}

void handle404() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for ( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }

  server.send ( 404, "text/plain", message );
}

void handleSave() {

  if (server.args() == 7) {
    char tmp[64];

    EEPROM.write(0, 42);
    EEPROM.write(1, 24);

    server.arg(0).toCharArray(tmp, 63);
    writeString(2, 63, tmp);

    server.arg(1).toCharArray(tmp, 63);
    writeString(66, 63, tmp);

    server.arg(2).toCharArray(tmp, 63);
    writeString(130, 63, tmp);

    server.arg(3).toCharArray(tmp, 63);
    writeString(194, 63, tmp);

    server.arg(4).toCharArray(tmp, 63);
    writeString(258, 63, tmp);

    server.arg(5).toCharArray(tmp, 63);
    writeString(322, 63, tmp);
  }
  handleRoot();
}

bool checkEEPROM() {
  byte test = EEPROM.read(0);
  byte test2 = EEPROM.read(1);
  return test == 42 && test2 == 24;
}

void readString(int offset, int maxLength, char * dst) {
  int i;
  for (i = 0; i < maxLength; i++) {
    char c = EEPROM.read(i + offset);
    if (c == 0) {
      break;
    }
    dst[i] = c;
  }
  dst[i + 1] = 0;
}

void writeString(int offset, int maxLength, char * src) {
  int length = strlen(src);
  if (length > maxLength) {
    length = maxLength;
  }
  for (int i = 0; i < length; i++) {
    EEPROM.write(i + offset, src[i]);
  }
  EEPROM.write(length + offset, 0);
  EEPROM.commit();
}

void setupServer() {
  Serial.println("Starting up in configuration mode...");

  Serial.println("Configuring access point...");

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

  WiFi.softAP(ssid, password);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("Access Point IP address: ");
  Serial.println(myIP);

  server.on ( "/", handleRoot );
  server.on ( "/save", handleSave );
  server.onNotFound ( handle404 );

  server.begin();
  Serial.println("WebSecurity Sensor Configuration server started.");
  digitalWrite(LED_PIN, LOW);
}

void setupClient() {
  Serial.println("Starting up in sensor mode...");

  readString(2, 63, remoteSSID);
  readString(66, 63, remotePassword);
  readString(130, 63, serverAddress);
  readString(194, 63, apiKey);
  readString(258, 63, sensorId);
  readString(322, 63, eventTypeId);

  Serial.println("Connecting to access point...");
  WiFi.begin(remoteSSID, remotePassword);

  byte ledStatus = 0;
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_PIN, ledStatus);
    ledStatus ^= 0x01;
    delay(100);
  }

  Serial.print("Sensor IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("WebSecurity Sensor started.");
  digitalWrite(LED_PIN, LOW);
  sensorStatus = digitalRead(SENSOR_PIN);
}

void loopServer() {
  server.handleClient();
}

void loopClient() {
  if (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_PIN, HIGH);
    ESP.restart();
  }
  int newStatus = digitalRead(SENSOR_PIN);
  if (sensorStatus != newStatus) {
    Serial.println("Sensor change");
    sensorStatus = newStatus;
  }
  delay(100);
}

void setup() {

  delay(1000);
  Serial.begin(115200);
  EEPROM.begin(1024);
  pinMode(LED_PIN, OUTPUT);
  pinMode(CONFIG_PIN, INPUT_PULLUP);
  pinMode(SENSOR_PIN, INPUT_PULLUP);
  Serial.println();

  configMode = (digitalRead(CONFIG_PIN) == LOW) || !checkEEPROM();

  if (configMode) {
    setupServer();
  } else {
    setupClient();
  }
}

void loop() {
  if (configMode) {
    loopServer();
  } else {
    loopClient();
  }
}
