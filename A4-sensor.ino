//Libraries to include
#include <ESP8266WiFi.h>
#include <PubSubClient.h>             
#include <Wire.h>               
#include <ArduinoJson.h>            
#include <Adafruit_Sensor.h>        
#include <Adafruit_TSL2591.h>       
#include <Adafruit_MPL115A2.h>    

//INTERNET ACCESS POINT//
//#define ssid "University of Washington"
//#define pass ""
#define ssid "Penthouse"
#define pass "61421226"
//MQTT INFORMATION//
#define mqtt_server "mediatedspaces.net"
#define mqtt_name "hcdeiot"
#define mqtt_pass "esp8266"

Adafruit_MPL115A2 mpl115a2; // defines the mp115a2 sensor object

//Some objects to instantiate
Adafruit_TSL2591 tsl = Adafruit_TSL2591(2591); // pass in a number for the sensor identifier (for your use later)
WiFiClient espClient;
PubSubClient mqtt(espClient);

unsigned long previousMillis;
char mac[18] = "ESP8602";

//function to configure the light sensor prior to use
void configureSensor(void)
{
  // You can change the gain on the fly, to adapt to brighter/dimmer light situations
  //tsl.setGain(TSL2591_GAIN_LOW);    // 1x gain (bright light)
  tsl.setGain(TSL2591_GAIN_MED);      // 25x gain
  //tsl.setGain(TSL2591_GAIN_HIGH);   // 428x gain
  
  // Changing the integration time gives you a longer time over which to sense light
  // longer timelines are slower, but are good in very low light situtations!
  //tsl.setTiming(TSL2591_INTEGRATIONTIME_100MS);  // shortest integration time (bright light)
  // tsl.setTiming(TSL2591_INTEGRATIONTIME_200MS);
  tsl.setTiming(TSL2591_INTEGRATIONTIME_300MS);
  // tsl.setTiming(TSL2591_INTEGRATIONTIME_400MS);
  // tsl.setTiming(TSL2591_INTEGRATIONTIME_500MS);
  // tsl.setTiming(TSL2591_INTEGRATIONTIME_600MS);

}

//function to reconnect if we become disconnected from the server
void reconnect() {
  // Loop until we're reconnected
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqtt.connect(mac, mqtt_name, mqtt_pass)) { //<<---using MAC as client ID, always unique!!!
      Serial.println("connected");
    } else {                        
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  // put your setup code here, to run once:
 WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println(); Serial.println("WiFi connected"); Serial.println();
  Serial.print("Your ESP has been assigned the internal IP address ");
  Serial.println(WiFi.localIP());
  WiFi.macAddress().toCharArray(mac,18);// stores the unique MAC address 

  /* Initialise the sensor */
  if (!tsl.begin())
  {
    /* There was a problem detecting the TSL2561 ... check your connections */
    Serial.print("Ooops, no TSL2561 detected ... Check your wiring or I2C ADDR!");
    while (1);
  }
  // initializes mp1115a2 sensor object
  mpl115a2.begin();
  // calls configure function, initializes TSL2591
  configureSensor();
  //connects to MQTT server
  mqtt.setServer(mqtt_server, 1883);
}


void loop() {
 
 unsigned long currentMillis = millis();//start timer

  mqtt.loop();// Loops MQTT connection
  if (!mqtt.connected()) {
    reconnect();
  }
  
  if (currentMillis - previousMillis > 5000) { //a periodic report, every 5 seconds
    sensors_event_t event;
    tsl.getEvent(&event);
    int celsius = mpl115a2.getTemperature(); // assigns temperature to celsius variable
    int fahrenheit = (celsius * 1.8) + 32; // celsius to fahrenheit conversion
    float light = event.light;
    float pressure = mpl115a2.getPressure();

    char str_press[6];    
    char str_light[7];
    char message[50];         // 50 length char array for whole MQTT message


    dtostrf(pressure, 4, 2, str_press);   // Convert's float readings to char arrays and stores them in
    dtostrf(light, 4, 2, str_light);     // variables
    //Serial.println(temperature); //debug line to check content of char array variables

    sprintf(message, "{\"mac\": \"%s\", \"temp\": \"%i F\", \"pressure\": \"%s kPa\", \"light\": \"%s lux\" }", mac, fahrenheit, str_press, str_light); //sprintf function concatinating all readings into the MQTT message
    Serial.println("publishing");// Serial message indicating publishing
    mqtt.publish("will/A4", message);//MQTT publish to the server
    //Serial.println(message);//debug line to check content of message char array
    previousMillis = currentMillis;// reset timer
  }
}
  
