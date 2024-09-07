#include <DHT.h>
#include <ESP_Mail_Client.h>
#include <WiFi.h>
#include <PubSubClient.h>


#define WIFI_SSID "Airties_Air4960R_RN77"
#define WIFI_PASSWORD "ytddfc7888"
#define SMTP_server "smtp.gmail.com"
#define SMTP_Port 465
#define sender_email "g.server0403@gmail.com"
#define sender_password "kmfatmgdzaamzglk"
#define Recipient_email "g.server0403@gmail.com"
#define Recipient_name ""

// Set Node-Red attributes
/*Mosquitto protocol is used in this project*/
const char* PUBLISH_TOPIC = "esp32/dht";
const char*  MQTT_SERVER = "test.mosquitto.org";

SMTPSession smtp;

long MAIL_DELTA_TIME;
long LOOP_DELTA_TIME;
DHT dht(26, DHT11);

WiFiClient espClient;
PubSubClient client(espClient);
Session_Config config;

void setup() {
  Serial.begin(115200);
  // Declare dht sensor
  dht.begin();
  // Connect to the WiFi
  connectWiFi();
  // Connect to the mqtt server
  client.setServer(MQTT_SERVER, 1883);
  client.setCallback(callback);
  // Connect to the email server
  configEmailConnection();
  // Set delta time
  MAIL_DELTA_TIME = millis();
  LOOP_DELTA_TIME = millis() + 20000;
}

void loop() {
  // Reconnect when the connection is not available
  if (!client.connected()) 
    connectClient();
  // Make the client work
  client.loop();

  if(!checkDeltaTime(LOOP_DELTA_TIME,10000))
    return;

  // Read temperature and humidity values
  float temperature = readDHTTemperature();
  float humidity = readDHTHumidity(); 

  // Check the sensor values whether they are normal or not
  checkTemperatureAndHumidityValues(temperature, humidity);

  char publish_str[100];
  snprintf(publish_str,100,"{\"temperature\":%.2f, \"humidity\":%.2f}", temperature,humidity);

  client.publish(PUBLISH_TOPIC, publish_str);

  LOOP_DELTA_TIME = millis();

  // Allow cpu run other tasks
  delay(10);
}

void callback(char* pubTopic, byte* payload, unsigned int length) {
  // Process the recieved message from the protocol
  Serial.print("Message received: ");
  Serial.println (pubTopic);
  Serial.print("payload: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void connectWiFi(){
  Serial.print("Connecting to "); 
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  // Connect to the wifi
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  } 
  Serial.println(""); 
  Serial.print("WiFi connected, IP address: "); 
  Serial.println(WiFi.localIP());
}

void connectClient(){
    Serial.print("Reconnecting client to ");
        Serial.println(MQTT_SERVER);
        // Connect to the client
        while (!client.connect("ESPClient")) {
            Serial.print(".");
            delay(500);
        }
        client.setCallback(callback);
        // Subscribe to the topic created on node-red mqtt node
        if (client.subscribe("esp32/dht")) {
            Serial.println("subscribe to cmd OK");
        } else {
            Serial.println("subscribe to cmd FAILED");
        }       
        Serial.println("Bluemix connected");
}


float readDHTTemperature() {
  // Read temperature as Celsius
  float t = dht.readTemperature();
  if (isnan(t))  
    t = -1;
  return t;
}

float readDHTHumidity() {
  // Read the humidity value as percent
  float h = dht.readHumidity();
  if (isnan(h))
    h = 0;
  return h;
}

char* floatToString(float num) {
    // Allocate memory for the string
    char* str = (char*)malloc(20 * sizeof(char));   
    if (str == NULL) {
        // Handle memory allocation failure
        return NULL;
    }  
    // Convert float to string with 2 decimal places
    snprintf(str, 20, "%.2f", num);  
    return str;
}

void sendEmail(char* recipientName, char* recipientEmail, float temperature, float humidity){
  // Declare the message class
  SMTP_Message message;
  message.sender.name = "ESP 32";
  message.sender.email = sender_email;
  message.subject = "ESP32 DHT Sensor Report Test Email";
  message.addRecipient(recipientName,recipientEmail);

  // Set the message content
  char textMessage[100];
  snprintf(textMessage,100, "Alert!!!\n\nTemperature is %.2f Celcius and Humidity is %.2f percent.",temperature,humidity);
  message.text.content = textMessage;
  message.text.charSet = "us-ascii";
  message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

  // Send the email
  if (!MailClient.sendMail(&smtp, &message))
    Serial.println("Error sending Email, " + smtp.errorReason());
  else
    Serial.println("Email is sent!");
}

void configEmailConnection(){
  MailClient.networkReconnect(true);
  smtp.debug(1);

  // Set the session config
  config.server.host_name = SMTP_server;
  config.server.port = SMTP_Port;
  config.login.email = sender_email;
  config.login.password = sender_password;
  config.login.user_domain = "";

  // Connect to the server
  if (!smtp.connect(&config)){
    ESP_MAIL_PRINTF("Connection error, Status Code: %d, Error Code: %d, Reason: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
    return;
  }
  if (!smtp.isLoggedIn()){
    Serial.println("\nNot yet logged in.");
  }
  else{
    if (smtp.isAuthenticated())
      Serial.println("\nSuccessfully logged in.");
    else
      Serial.println("\nConnected with no Auth.");
  }
}

void checkTemperatureAndHumidityValues(float temperature, float humidity){
  if (temperature > 40.0 || humidity > 90 && checkDeltaTime(MAIL_DELTA_TIME,60000)){
    sendEmail(Recipient_name, Recipient_email, temperature, humidity);
    MAIL_DELTA_TIME = millis();
  }
}

bool checkDeltaTime(long deltaTime, long timePassed){
  bool result = false;
  long difference = millis() - deltaTime;
  if(difference >= timePassed)
    result = true;

  return result;
}
