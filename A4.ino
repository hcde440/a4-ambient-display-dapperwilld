#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>// includes library for ESP8266 web browsing capabilities
#include <PubSubClient.h>
#include <Wire.h>
#include <ArduinoJson.h>   

//#define ssid "University of Washington"
//#define pass ""

#define ssid "Penthouse"
#define pass "61421226"
#define mqtt_server "mediatedspaces.net"
#define mqtt_name "hcdeiot"
#define mqtt_pass "esp8266"
String weatherKey = "2fec1143f43e5bc01d052646a2cb8e1c"; //API key for open weather API
boolean timeStart = false;

int greenPin = 15;
int redPin = 12;
int bluePin = 13;
int humidity;
long lastMillis;
char mac[18];

WiFiClient espClient;
PubSubClient mqtt(espClient);

//function to do something when a message arrives from mqtt server
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");


  if (!timeStart)
  {
    // start 1-hour timer;
    lastMillis = millis();
    timeStart = true;
  }
  
  if (timeStart && (millis() - lastMillis >= 3600L * 1000))
  {
   humidity = getHum("Seattle");
   timeStart = false;
  }
  DynamicJsonBuffer  jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(payload); //parse it!

  outputReading(root["temp"],humidity, root["light"]);
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

//function to reconnect if we become disconnected from the server
void reconnect() {
  // Loop until we're reconnected
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqtt.connect(mac, mqtt_name, mqtt_pass)) { //<<---using MAC as client ID, always unique!!!
      Serial.println("connected");
      mqtt.subscribe("will/A4"); //we are subscribing to 'theTopic' and all subtopics below that topic
    } else {                        //please change 'theTopic' to reflect your topic you are subscribing to
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);//for debugging code, comment out for production

  pinMode(greenPin, OUTPUT);
  pinMode(redPin, OUTPUT);
  pinMode(bluePin, OUTPUT);

  digitalWrite(greenPin, HIGH);
  digitalWrite(redPin, HIGH);
  digitalWrite(bluePin, HIGH);


  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println(); Serial.println("WiFi connected"); Serial.println();
  Serial.print("Your ESP has been assigned the internal IP address ");
  Serial.println(WiFi.localIP());
   WiFi.macAddress().toCharArray(mac,18);
   
  humidity = getHum("Seattle");
  //connects to MQTT server
  mqtt.setServer(mqtt_server, 1883);
  mqtt.setCallback(callback);
}

void loop() {
  // put your main code here, to run repeatedly:
  mqtt.loop();
  if (!mqtt.connected()) {
    reconnect();
  }
}

void outputReading(int temperature, float humidity, float light){
  
  Serial.print(temperature);
  Serial.print(humidity);
  Serial.println(light);
  if (temperature < 60){
    digitalWrite(redPin,HIGH);
    Serial.println(redPin);
  }else{
    digitalWrite(redPin,LOW);
  }   
  if (humidity > 80){
    digitalWrite(bluePin,LOW);
    Serial.println(bluePin);
    }else{
    digitalWrite(greenPin,HIGH);
    }
  if (light < 200){
    digitalWrite(greenPin,HIGH);
    Serial.println(greenPin);
  }else{
    digitalWrite(greenPin,LOW);
  }
}

int getHum(String city) {
  HTTPClient theClient; // initializes browser
  String apiCall = "http://api.openweathermap.org/data/2.5/weather?q=" + city; //Assembles the URL for the 
  apiCall += "&units=imperial&appid=";                                         //Openweathermap API request and stores
  apiCall += weatherKey;                                                       //it in the apiCall variable
  theClient.begin(apiCall);//navigates to weather data API
  int httpCode = theClient.GET();//
  if (httpCode > 0) {

    if (httpCode == HTTP_CODE_OK) {//Checks whether the GET request was successful
      String payload = theClient.getString();
      DynamicJsonBuffer jsonBuffer;//initializs JSON parser
      JsonObject& root = jsonBuffer.parseObject(payload);//Parses String payload as a JSON object stored in root variable
      if (!root.success()) {//Checks whether parse was successful, prints error message to Serial
        Serial.println("parseObject() failed in getHum().");
      }
      Serial.print("humidity obtained: ");
      Serial.println(root["main"]["humidity"].as<String>());
      return root["main"]["humidity"].as<int>();      
    }
  }
  else {
    Serial.printf("Something went wrong with connecting to the endpoint in getMet().");//Checks whether parse was successful, prints error message to Serial
  }
}
