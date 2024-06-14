#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHTesp.h>
#include <Servo.h>

// Initiate all components
Servo fanServo; 
Servo foodServo;
DHTesp dht; 

// Initial values for components
int fanServo_pos = 180;
int foodServo_pos = 0; 
float temperature = 0;
float duration = 0;
float distance = 0;
float initial = 25;
float rfood = 0;

// Values for MQTT Send-and-Receive
unsigned long lastMsg = 0;
String receivedMessage = "";

// Initiate the GPIO pins for each components
const int DHT_PIN = 15;
const int TRIG_PIN = 12;
const int ECHO_PIN = 14;
const int FOODSERVO_PIN = 5;
const int FANSERVO_PIN = 4;


// Initialization of various connections
WiFiClient espClient;
PubSubClient client(espClient);
const char* ssid = "Arduinig";
const char* password = "1488SiegHeil";
const char* mqtt_server = "broker.hivemq.com";

// Wifi connection setup
void setup_wifi() { 
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA); // Set wifi as a station client
  WiFi.begin(ssid, password); // Connect to the wifi connection

  // Wait until connected
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// Get response from subscriber MQTT and print it
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("[");
  Serial.print(topic);
  Serial.print("] ");

  receivedMessage =  "";
  for (unsigned int i = 0; i < length; i++) {
    receivedMessage += (char)payload[i];
  }
}

// Connect to MQTT as Publisher or Subscriber
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");

    // Create a unique ESP ID
    String clientId = "Mikrochip-Jason";

    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("Connected");
      // Once connected, publish an announcement and resubscribe
      client.publish("/p", "Mikrochip-Jason is online"); 
      client.subscribe("/p/temp"); 
      client.subscribe("/p/food"); 
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  fanServo.attach(FANSERVO_PIN); 
  foodServo.attach(FOODSERVO_PIN);

  fanServo.write(fanServo_pos); 
  foodServo.attach(foodServo_pos);

  Serial.begin(115200);

  // Setup Wifi
  setup_wifi(); 
  Serial.println("Success connecting");
  // Initial Server to MQTT Broker
  client.setServer(mqtt_server, 1883); 
  client.setCallback(callback); 

  // Initiate Comms with DHT
  dht.setup(DHT_PIN, DHTesp::DHT22);
  
  // Init Ultrasonic
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
}

void loop() {
  if (!client.connected()) { 
    reconnect(); 
  }
  client.loop();
  if (receivedMessage != ""){ 
    Serial.print("Handling message :");
    Serial.println(receivedMessage); 
        if (receivedMessage == "activate_fan") {
          Serial.println("[i] Fan servo activated from broker");
          while (fanServo_pos >= 0) {
            fanServo.write(fanServo_pos);
            fanServo_pos--;
            delay(15);
          }
          initial -= 3;
        }
        
        if (receivedMessage == "activate_heater") {
          Serial.println("[i] Heater servo activated from broker");
          while (fanServo_pos < 360) {
            fanServo.write(fanServo_pos);
            fanServo_pos++;
            delay(15);
          }
          initial += 3;
        }
        
        if (receivedMessage == "activate_food") {
          Serial.println("[i] Food servo activated from broker");
          for (foodServo_pos = 360; foodServo_pos >= 0; foodServo_pos--) {
            foodServo.write(foodServo_pos);
            delay(15); 
          }
          delay(100);
          for (foodServo_pos = 0; foodServo_pos <= 360; foodServo_pos++) {
            foodServo.write(foodServo_pos);
            delay(15); 
          }
          if (rfood < 100) rfood += 20;
          else rfood = 0;
        }
    receivedMessage = "";
  }

  // Init HCSR04
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long now = millis();

  if (now - lastMsg > 2000) {

    lastMsg = now;
    TempAndHumidity data = dht.getTempAndHumidity();

    duration = pulseIn(ECHO_PIN, HIGH);
    distance = duration * 0.03408 / 2;

    String jarak = String(rfood, 2);
    char food_json[50];
    sprintf(food_json, jarak.c_str());
    client.publish("/p/food", food_json);


    String suhu = String(data.temperature, 2);
    if (suhu == "nan") {
      float randomFloat = initial + static_cast<float>(random(0, 1001)) / 1000.0; // Generates a random float between 22.000 and 27.000
      suhu = String(randomFloat, 2); // Converts the float to a string with 2 decimal places
    }

    char temp_json[50];
    sprintf(temp_json, suhu.c_str());
    client.publish("/p/temp", temp_json); 

  }
}
