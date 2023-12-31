#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>
#include "HackPublisher.h"
#include "AM232X.h" 





//Declaration of OLED
Adafruit_SSD1306 oled(128,64,&Wire,-1);
//Wifi SSID and Password
const char *ssid="ASUS-F8";
const char *password="K33pi7$@f3%";
//Pin on esp32 connected to gas analog pin
const int gasPin=A3;
//Threshold number for dangerous gas readings
int gasLimit;
//Temperature limit (Celsius)
const int tempLimit=32;
//Humidity limit
const int humidityLimit=45;
//Pins providing current for LEDS
const int RED_LED=16;
const int GREEN_LED=17;
//Pin connected to speaker
const int speakerPin=18;
//Duration of notes for speaker
const int NOTE_DUR=60;
//Pin reading the state of the button
const int BUTTON_PIN=21;
//Previous button state
bool prevState=false;
//Number of LED on Neopixel Ring
const int LED_COUNT=12;
//Pin connected to Neopixel IN Port
const int LED_PIN=15;
//Pins for ultrasonic sensor
const int trigPin=27;
const int echoPin=33;
//Booleans for gas, temperature and humidity safety
bool gasSafe=false;
bool tempSafe=false;
bool humiditySafe=false;

//Neopixel instance for Neopixel ring
Adafruit_NeoPixel ring(LED_COUNT,LED_PIN,NEO_RGBW+NEO_KHZ800);
//Publisher instance for team Samueli Fist Fighters
HackPublisher publisher("Samueli Fist Fighters",true);
//Intialize Humidity and Temperature Sensor
AM232X AM2320;

void setup() {
  //Intialize serial communication
  Serial.begin(115200);
  //Set pin modes for various pins
  pinMode(RED_LED,OUTPUT);
  pinMode(GREEN_LED,OUTPUT);
  pinMode(gasPin, INPUT);
  pinMode(speakerPin, OUTPUT);
  pinMode(BUTTON_PIN,INPUT_PULLUP);
  pinMode(trigPin,OUTPUT);
  pinMode(echoPin,INPUT);
  pinMode(13,INPUT_PULLUP);
  
  while(!Serial)continue;
  //connect to wifi
  WiFi.begin(ssid,password);
  while(WiFi.status()!=WL_CONNECTED){
    delay(500);
    Serial.print("Local IP Address ");
    Serial.println(WiFi.localIP());
  }
  //intialize publisher
  publisher.begin();
  //intialize oled
  if(!oled.begin(SSD1306_SWITCHCAPVCC,0x3C)){
    Serial.println(F("SSD1306 allocation failed"));
    while (true);
  }
  //intialize neopixel ring
  ring.begin();
  ring.show();
  ring.setBrightness(100);
  delay(1000);

  // Humidity Sensor
  Serial.println(__FILE__);
  Serial.print("LIBRARY VERSION: ");
  Serial.println(AM232X_LIB_VERSION);
  Serial.println();

   if (! AM2320.begin() )
     {
      Serial.println("Sensor not found");
       while (1);
     }
     //intialize temperature/humidity sensor
  AM2320.wakeUp();
  gasLimit=analogRead(gasPin)+600;
  Serial.println(gasLimit);
  delay(2000);

  Serial.println("Type,\tStatus,\tHumidity (%),\tTemperature (C)");
}

//given code for playing speaker
void play(int note, int dur)
{
  tone(speakerPin, note);
  delay(dur*NOTE_DUR);
  noTone(speakerPin); 
  delay(dur*NOTE_DUR/3);
}

void loop() {
  // Read gas level detected by gas sensor
  int gasLevel=analogRead(gasPin);
  //Clear oled display for new data
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(WHITE,BLACK);
  oled.setCursor(0,0);
  //Display gas data to OLED
  oled.print("Gas Level ");
  oled.println(gasLevel);
  //Read Data from ultrasonic sensor
  digitalWrite(trigPin,LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin,HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin,LOW);
  long duration=pulseIn(echoPin,HIGH);
  //Formula to convert time it takes for the waves to travel to distance
  double distance=(0.034*duration)/2;
  //Print Data from ultrasonic sensor
  oled.print("Distance: ");
  oled.print(distance);
  oled.println(" cm");
  Serial.print("Distance: ");
  Serial.println(distance);

  // READ DATA (Temperature/Humidity)
  int status = AM2320.read();
  float temperature = AM2320.getTemperature();
  temperature=round(temperature);
  float humidity = AM2320.getHumidity();
  humidity=round(humidity);
  
  switch (status)
  {
    case AM232X_OK:
      Serial.print("OK,\t");
      break;
    default:
      Serial.print(status);
      Serial.print("\t");
      break;
  }
  // DISPLAY DATA (Temperature, Humidity), sensor only returns one decimal.
  Serial.print("Temperature ");
  Serial.print(temperature);
  Serial.println(" C");
  Serial.print("Humidity: ");
  Serial.println(humidity);
  
  oled.print("Temperature: ");
  oled.println(temperature);
  oled.print("Humidity: ");
  oled.println(humidity);
  //Check if gas level, temperature and humidity are safe or not
  //In case it isn't corresponding boolean value is changed
  if(gasLevel>=gasLimit){
    gasSafe=false;
  }
  else{
    gasSafe=true;
  }
  if(temperature>=tempLimit){
    tempSafe=false;
  }
  else{
    tempSafe=true;
  }
  if(humidity<=humidityLimit){
    humiditySafe=false;
  }
  else{
    humiditySafe=true;
  }
  //Flashlight 
  int buttonState=digitalRead(BUTTON_PIN);
  if (buttonState == LOW) {
    if(!prevState){
    ring.fill(4294967295, 0, 0);
    
    ring.show();
    prevState=true;
    }
    else{
      ring.fill();
      ring.show();
      prevState=false;
     } 
  }
  //Store data to be sent by publisher
  publisher.store("Gas_Level",gasLevel);
  //If else statements to determine what message should be stored
  //to be sent back to the website
  if(gasSafe&&tempSafe&&humiditySafe){
    digitalWrite(RED_LED, LOW);
    digitalWrite(GREEN_LED, HIGH);
    publisher.store("Danger_Level", "Area Safe");
    oled.println("Area Safe");
    oled.display();
  }
  else if(!gasSafe&&!tempSafe&&!humiditySafe){
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, HIGH);
    play(3000, 3);
    publisher.store("Danger_Level", "Hazardous Gas Temperature and Humidity");
    oled.println("Hazardous Gas Temperature and Humidity");
    oled.display();
  }
  else if(!gasSafe&&!tempSafe){
    play(3000, 3);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, HIGH);
    publisher.store("Danger_Level", "Hazardous Gas and Temperature");
    oled.println("Hazardous Gas and Temperature");
    oled.display();
  }
  else if(!gasSafe&&!humiditySafe){
    play(3000, 3);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, HIGH);
    publisher.store("Danger_Level", "Hazardous Gas and Humidity");
    oled.println("Hazardous Gas and Humidity");
    oled.display();
  }
  else if(!tempSafe&&!humiditySafe){
    play(3000, 3);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, HIGH);
    publisher.store("Danger_Level", "Hazardous Temperature and Humidity");
    oled.println("Hazardous Temperature and Humidity");
    oled.display();
  }
  else if(!gasSafe){
    play(3000, 3);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, HIGH);
    publisher.store("Danger_Level", "Hazardous Gas");
    oled.println("Hazardous Gas");
    oled.display();
  }
  else if(!tempSafe){
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, HIGH);
    play(3000, 3);
    publisher.store("Danger_Level", "Hazardous Temperature");
    oled.println("Hazardous Temperature");
    oled.display();
  }
  else if(!humiditySafe){
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, HIGH);
    play(3000, 3);
    publisher.store("Danger_Level", "Hazardous Humidity");
    oled.println("Hazardous Humidity");
    oled.display();
  }
//  else{
//    digitalWrite(GREEN_LED, LOW);
//    digitalWrite(RED_LED, HIGH);
//    play(3000, 3);
//    publisher.store("Danger_Level", "Hazardous Gas Temperature and Humidity");
//    oled.println("Hazardous Gas Temperature and Humidity");
//    oled.display();
//  }
  publisher.store("Temperature",temperature);
  publisher.store("Gas_Limit",gasLimit);
  publisher.store("Humidity",humidity);
  publisher.store("Distance",distance);
  //Send all stored data to website
  publisher.send();
  
  delay(100);
}
