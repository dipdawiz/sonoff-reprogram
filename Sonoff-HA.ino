#include <FS.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncTCP.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

//--------------------------------------------------------------------------------
char ssid[40];
char pwd[40];

int restore_state = 1; //restore state flag
char savedstate[2] = "0"; //saved state of 1 pin

int gpio13Led = 13;
int gpio12Relay = 12;

WiFiClient client;
IPAddress myIP; //wemos ip address
const char *ap_ssid = "RelayBoard";
const char *ap_pwd = "";
AsyncWebServer server(80);

//--------------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  delay(100);
  initGPIO();
  delay(100);
  Serial.println("\n\n\nRelay Program Started");

  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");
          if(json.containsKey("ssid") )
            strcpy(ssid, json["ssid"]);
          if(json.containsKey("pwd") )  
            strcpy(pwd, json["pwd"]);
          if(json.containsKey("restore_state") )  
            restore_state = json["restore_state"];                         
          if(json.containsKey("savedstate") )  
            strcpy(savedstate, json["savedstate"]);
        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }

  //restore state or OFF
  if(restore_state) {
    restoreState();
  } else {
    saveConfig();
    digitalWrite(gpio13Led, HIGH);
    digitalWrite(gpio12Relay, LOW);
  }

  //if no ssid, start as AP mode
  if(ssid == "" ) {
    Serial.println("Starting Access Point ...");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ap_ssid);
    myIP = WiFi.softAPIP();
    Serial.print("Access Point IP address: ");
    Serial.println(myIP);    
    Serial.println("Access the relay board at  http://relay.local/");
  } else {
    // Connect to WiFi access point.
    Serial.println(); Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pwd);
    int ctr = 0;
    while (WiFi.status() != WL_CONNECTED && ctr < 30) {
      delay(500);
      Serial.print(".");
      ctr++;
    }
    if(WiFi.status() == WL_CONNECTED) {
      myIP = WiFi.localIP();
      Serial.println();
      Serial.println("WiFi connected");
      Serial.println("IP address: "); Serial.println(myIP);
    } else {
      //failed to connect first time, so retry once more
      Serial.println(".");
      Serial.print("Could not connect to WiFi: "); Serial.println(ssid);
      Serial.println("*** Retrying ***");
      WiFi.mode(WIFI_OFF);
      delay(3000);
      WiFi.mode(WIFI_STA);
      WiFi.begin(ssid, pwd);
      int ctr = 0;
      while (WiFi.status() != WL_CONNECTED && ctr < 30) {
        delay(500);
        Serial.print(".");
        ctr++;
      }
      if(WiFi.status() == WL_CONNECTED) {
        myIP = WiFi.localIP();
        Serial.println();
        Serial.println("WiFi connected");
        Serial.println("IP address: "); Serial.println(myIP);
        
      } else {
        //could not connect for second time also, probable issue in ssid/network, start in AP mode
        Serial.println();
        Serial.print("Could not connect to WiFi: "); Serial.println(ssid);
        Serial.println("Starting Access Point ...");
        WiFi.mode(WIFI_AP);
        WiFi.softAP(ap_ssid);
        myIP = WiFi.softAPIP();
        Serial.print("Access Point IP address: ");
        Serial.println(myIP);
        Serial.println("Access the relay board at  http://relay.local/");
      }
    }
    
  }
 

  if (!MDNS.begin("relay")) {
    Serial.println("Error setting up MDNS responder!");
    while(1) { 
      delay(1000);
    }
  }


  //http server routes - UI
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String html = ""
    "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Wi-Fi Relay Board</title>"
    "<script>"
    "loading = true;"
    "var xhttp = new XMLHttpRequest();"
    "xhttp.onreadystatechange = function() {"
    "    if (this.readyState == 4 && this.status == 200) {"
    "       var resp = JSON.parse(xhttp.responseText);"
    "       document.getElementById('ssid').value = resp['ssid'];"
    "       document.getElementById('pwd').value = resp['pwd'];"
    "       if(resp['restore_state'] === '1') document.getElementsByName('restore_state')[0].checked = true;"
    "       else document.getElementsByName('restore_state')[1].checked = true;"
    "       document.getElementById('msg').innerHTML = resp['msg'];"
    "    }"
    "};"
    
    "function getConfig() {"
      "xhttp.open('GET', '/getconfig', true);"
      "xhttp.send();"
    "}"

    "function restart() {"
      "xhttp.open('GET', '/restart', true);"
      "xhttp.send();"
    "} "
    "function on() {"
      "xhttp.open('GET', '/on', true);"
      "xhttp.send();"
    "} "
    "function off() {"
      "xhttp.open('GET', '/off', true);"
      "xhttp.send();"
    "} "
    "function clearmsg() {"
    "  document.getElementById('msg').innerHTML = '';" 
    "} "
    "function save() {"
    "  loading=true;"  
    "  var ssid = document.getElementById('ssid').value;"
    "  var pwd  = document.getElementById('pwd').value;"
    "  var restore_state  = document.getElementsByName('restore_state')[0].checked;"
    "  xhttp.open('GET', '/save?ssid=' + ssid "
    "+ '&pwd=' + pwd "
    "+ '&restore_state=' + (restore_state? '1' : '0') "
    ", true);"
    "  xhttp.send();"
    "}"
    
    
    "</script>"
    "</head><body onLoad='getConfig()'><h3 style='color:green'>Wi-Fi Relay Board</h3>"
    "<table>"
    "<tr><td><B>SSID</B></td><td colspan='2'> : <input type='text' name='ssid' id='ssid' onChange='clearmsg();'></td></tr>"
    "<tr><td><B>Password</B></td><td colspan='2'> : <input type='password' name='pwd' id='pwd' onChange='clearmsg();'></td></tr>"
    "<tr><td><B>State on Boot</B></td><td>: <input type='radio' name='restore_state' value='1' checked onClick='clearmsg();'> RESTORE  &nbsp;</td><td><input type='radio' name='restore_state' value='0' onClick='clearmsg();'> OFF </td></tr>"
    "<tr><td>&nbsp;</td><td>&nbsp;</td></tr>"
    "<tr><td></td><td><input type='button' value='Restart' onClick='restart()'>&nbsp;&nbsp;&nbsp;&nbsp;<input type='button' value='Save' onClick='save()'></td><td id='msg'></td></td></tr>"
    "</table><BR>"
    "<HR>"
    "<table>"
    "<tr><td><input type='button' name='alloff' value='ON' onClick='on();'> &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; <input type='button' name='allon' value='OFF' onClick='off();'> </td></tr>"
    "</table><BR>"
    "<HR>"
    
    "</body></html>";

    request->send(200, "text/html", html);
  });

  //relay-control route takes query string l=load,state or t=load (load is the pin index)
  server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request){
    
    digitalWrite(gpio13Led, LOW);
    digitalWrite(gpio12Relay, HIGH);
    delay(500);
    
    String resp = "{ \"ssid\" : \"" + String(ssid) 
    + "\" , \"pwd\" : \"" + String(pwd) 
    + "\" , \"restore_state\" : \"" + String(restore_state) 
    + "\" , \"savedstate\" : \"" + "1" 
    + "\" , \"msg\" : \""  
    + "\" }";          
    request->send(200, "application/json", resp);
    saveConfig();
  });
  
    server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request){
    
    digitalWrite(gpio13Led, HIGH);
    digitalWrite(gpio12Relay, LOW);
    delay(500);
    
    String resp = "{ \"ssid\" : \"" + String(ssid) 
    + "\" , \"pwd\" : \"" + String(pwd) 
    + "\" , \"restore_state\" : \"" + String(restore_state) 
    + "\" , \"savedstate\" : \"" + "0" 
    + "\" , \"msg\" : \""  
    + "\" }";          
    request->send(200, "application/json", resp);
    saveConfig();
  });

  //save route to save config
  server.on("/save", HTTP_GET, [](AsyncWebServerRequest *request){
    if(request->hasParam("ssid")) {
      AsyncWebParameter* p = request->getParam("ssid");
      String ssid1 = String(p->value()).c_str();
      ssid1.toCharArray(ssid, 1+ssid1.length());
    }
    if(request->hasParam("pwd")) {
      AsyncWebParameter* p = request->getParam("pwd");
      String pwd1 = String(p->value()).c_str();
      pwd1.toCharArray(pwd, 1+pwd1.length());
    }
    if(request->hasParam("restore_state")) {
      AsyncWebParameter* p = request->getParam("restore_state");
      String restore_state1 = String(p->value()).c_str();
      restore_state = restore_state1.equals("1");
    }
    String msg = "<font color='red'>Saved!</font>";
    //String currentstate = getStatus();
    String st = "{ \"ssid\" : \"" + String(ssid) 
    + "\" , \"pwd\" : \"" + String(pwd) 
    + "\" , \"restore_state\" : \"" + String(restore_state) 
    + "\" , \"savedstate\" : \"" + String(savedstate) 
    + "\" , \"msg\" : \"" + msg 
    + "\" }";          
    request->send(200, "application/json", st);
    saveConfig();
  });

  //restart the wemos d1 mini
  server.on("/restart", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(gpio13Led, HIGH);
    digitalWrite(gpio12Relay, LOW);
    delay(5000);
    ESP.restart();  
  });

  //getconfig gets you the current configuration as well current state of relays/pins
  server.on("/getconfig", HTTP_GET, [](AsyncWebServerRequest *request){
    //String currentstate = getStatus();
    String st = "{ \"ssid\" : \"" + String(ssid) 
    + "\" , \"pwd\" : \"" + String(pwd) 
    + "\", \"restore_state\" : \"" + String(restore_state) 
    + "\", \"savedstate\" : \"" + String(savedstate) 
    + "\", \"msg\" : \"" 
    + "\" }";
    request->send(200, "application/json", st);
  });

  //reset the config file to an empty file
  server.on("/resetconfig", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "application/json", "Config Reset");
    Serial.println("deleting config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }
    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    ESP.reset();   
  });

  //handle file not found error
  server.onNotFound([](AsyncWebServerRequest *request){
    Serial.printf("NOT_FOUND: ");
    Serial.printf(" http://%s%s\n", request->host().c_str(), request->url().c_str());
    request->send(404);
  });

  //start the http server
  server.begin();
  delay(1000);
  Serial.end();  // Have to end() it as RX and TX are being used for Digital IO 
  delay(100);
}

//--------------------------------------------------------------------------------

void restoreState() {
    if(String(savedstate).toInt() == 0){
      digitalWrite(gpio13Led, HIGH);
      digitalWrite(gpio12Relay, LOW);
    } else {
      digitalWrite(gpio13Led, LOW);
      digitalWrite(gpio12Relay, HIGH);
    }
}

//--------------------------------------------------------------------------------

void saveConfig() {
  String sts = getStatus();
  strcpy(savedstate, sts.c_str());
  Serial.println("saving config");
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  json["ssid"] = ssid;
  json["pwd"] = pwd;
  json["restore_state"] = restore_state;
  json["savedstate"] = savedstate;
  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("failed to open config file for writing");
  }
  json.printTo(Serial);
  json.printTo(configFile);
  configFile.close();
}

//--------------------------------------------------------------------------------

void initGPIO() {
  pinMode(gpio13Led, OUTPUT);
  digitalWrite(gpio13Led, HIGH);
  
  pinMode(gpio12Relay, OUTPUT);
  digitalWrite(gpio12Relay, HIGH);
}

//--------------------------------------------------------------------------------

void loop(){

}


//--------------------------------------------------------------------------------



String getStatus() {
  return String(digitalRead(gpio12Relay)) ;

}

