// USB Hub w/ WiFi control of USB power
// Used to remotely turn on/off a Chromecast
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

// uses WiFiManager from https://github.com/tzapu/WiFiManager
#include <WiFiManager.h>

#define PWR_PIN  2

ESP8266WebServer server(80);


void handleNotFound(){
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET)?"GET":"POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for (uint8_t i=0; i<server.args(); i++){
        message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send(404, "text/plain", message);
}

void setup(void){
    pinMode(PWR_PIN, OUTPUT);
    digitalWrite(PWR_PIN, HIGH);

    Serial.begin(115200);
    delay(1000);

    WiFiManager wifiManager;
    wifiManager.autoConnect();

    WiFi.printDiag(Serial);

    if (MDNS.begin("esp8266")) {
        Serial.println("MDNS responder started");
    }

    server.on("/", []() {
        server.send(200, "text/plain", "Try /pwr to control the hub.");
    });

    server.on("/pwr", HTTP_GET, [](){
        server.send(200, "text/plain",
                    digitalRead(PWR_PIN) == HIGH ? "state=on" : "state=off");
    });

    server.on("/pwr", HTTP_POST, [](){
        if (!server.hasArg("state")) {
            server.send(503, "text/plain", "Missing state variable");
            return;
        }
        digitalWrite(PWR_PIN, server.arg("state") == "on" ? HIGH : LOW);
        server.send(200, "text/plain", "OK");
    });

    server.onNotFound(handleNotFound);

    server.begin();
    Serial.println("HTTP server started");
}

void loop(void){
  server.handleClient();
}
