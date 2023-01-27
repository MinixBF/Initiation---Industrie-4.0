#include "DHT.h"
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <iostream>
#include <fstream>
#include <string>

using namespace std;

#define DHTPIN 4   // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)

#define wifi_ssid "Invite-ESIEA"
#define wifi_password "hQV86deaazEZQPu9a"

#define mqtt_server "10.8.128.250"
#define mqtt_user ""
#define mqtt_password ""

DHT dht(DHTPIN, DHTTYPE);
WiFiClient espClient;
ESP8266WebServer server(80);
PubSubClient client(espClient);

string read_html_file(string file_name) {
  string lines;
  string line;
  ifstream myfile(file_name);
  if (myfile.is_open()) {
      while (getline(myfile,line)) {
          cout << line << '\n';
          lines += line;
      }
      myfile.close();
  } else cout << "Unable to open file";
  return lines;
}

String SendHTML(float temperature, float humidity) {
  String html = "<!DOCTYPE html>";
  html += "<html lang=\"en\"> <head>";
  html += "<meta http-equiv=\"refresh\" content=\"2\"/>  <meta charset=\"UTF-8\"> <meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\"> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">";
  html += "<title>ESP8266 Weather Report</title>";
  html += "<style> body { background-color: #f1f1f1; font-family: Arial, Helvetica, sans-serif; text-align: center; }  h1, h2 { color: #333; margin: 8px 0; } section { width: 75%; margin: 20px auto; padding: 10px; background-color: #fff; border: 1px solid #ccc; border-radius: 16px; box-shadow: 0 0 10px rgba(0, 0, 0, 0.2); } div { display: flex; gap: 1rem; align-items: center; } </style>";
  html += "</head>";
  html += "<body> <h1>ESP8266 NodeMCU Weather Report</h1> <section> <div> <h2>Temperature:</h2> <span>";
  html += temperature;
  html += "°C</span> </div> <div> <h2>Humidity:</h2> <span>";
  html += humidity;
  html += "%</span> </div> </section> </body>";
  html += "</html>";

  return html;
}

void handle_OnConnect() {
  float t = dht.readTemperature(); // Gets the values of the temperature
  float h = dht.readHumidity(); // Gets the values of the humidity 
  server.send(200, "text/html", SendHTML(t, h)); 
}


void handle_NotFound() {
  server.send(404, "text/plain", "Not found");
}

void setupWifi() {
  Serial.println("");
  Serial.print("Connecting to ");
  Serial.print(wifi_ssid);

  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("WiFi connection established!");
  Serial.print("=> IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handle_OnConnect);
  server.onNotFound(handle_NotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void callback(char* topic, unsigned char* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  if (length > 0) {
    Serial.print(" : ");
    for (int i = 0; i < (int)length; i++) {
      Serial.print((char)payload[i]);
    }
    Serial.println();
  } else {
    Serial.println("No message");
  }
}

void setup() {
  Serial.begin(9600); 
  dht.begin();
  setupWifi();
  client.setServer(mqtt_server, 1883);    // Configuration de la connexion au serveur MQTT
  client.setCallback(callback);  // La fonction de callback qui est executée à chaque réception de message
  if (client.connect("ESP8266Client", mqtt_user, mqtt_password)) { // Connexion au serveur MQTT
    Serial.println("Connected to MQTT server");
    client.subscribe("ESIEA"); // Souscription au topic
    
  } else {
    Serial.print("failed with state ");
    Serial.println(client.state());
  }
}

//Reconnexion
void reconnect() {
  //Boucle jusqu'à obtenur une reconnexion
  while (!client.connected()) {
    Serial.print("Connecting to MQTT server...");
    if (client.connect("ESP8266Client", mqtt_user, mqtt_password)) {
      Serial.println("OK");
    } else {
      Serial.print("KO, error : ");
      Serial.println(client.state());
      Serial.println("Waiting 5 seconds before retrying...");
      delay(5000);
    }
  }
}

void loop() {
  // Wait a few seconds between measurements.
  delay(2000);

  server.handleClient();

  while(!client.connected()) {
    reconnect();
  }

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius
  float t = dht.readTemperature();

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  char json[50];
  snprintf(json, sizeof json, "{\"temperature\": %.2f, \"humidity\": %.2f}", t, h);

  client.publish("ESIEA/groupe4", json); // Publication d'un message sur le topic
  
  Serial.print("Humidity: "); 
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: "); 
  Serial.print(t);
  Serial.println(" °C\t");
}
