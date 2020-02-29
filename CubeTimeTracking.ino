/*  ********************************************* 
 *  WIFI rotation switch based on ADXL345 
 *  *********************************************/
#include <SparkFun_ADXL345.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <EEPROM.h>

/*********** COMMUNICATION SELECTION ***********/
ADXL345 adxl = ADXL345();  // USE FOR I2C COMMUNICATION

/****************** LOGIC VARIABLES ******************/
/*WIFI Connection*/
const char* ssid = "";
const char* password = "";

/*Logic variables*/
int range = 2; //possible values are 2,4,8,16 -> see adxl345 specs for details
float normalizeval = 256; //possible values are 256,128,64,32 -> see adxl345 specs for details
int prev_status = 0;
float x_previous = 0,y_previous = 0,z_previous = 0;
float threshold = 0.2;
bool start = true;
bool first = true;
bool changed = false;
int count = 0;
int ledDelay = 2000;
int connectionDelay = 0;

/*Message variables*/
String host = "";
const uint8_t fingerprint[20] = {};
char * mtd = "POST";
String payload;

/*Pin configuration*/
int redPin = D7;
int greenPin = D6;
int bluePin = D5;
const int wakeUpPin = D3;

/*Debug*/
bool debug = false;

/******************** SETUP ********************/
void setup(){
  EEPROM.get(0, prev_status);
  Serial.println(prev_status);
  
  // Start the serial terminal
  if( debug ){
    Serial.begin(115200);                 
    Serial.println("Serial communication up!\n");
  }  

  //set up LEDs
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);

  wifiSleep();
  
  
  /*Setup the ADXL345*/
  adxl.powerOn();                     // Power on the ADXL345

  // Turn on Interrupts for each mode (1 == ON, 0 == OFF)
  adxl.InactivityINT(1);
  adxl.ActivityINT(1);
  adxl.FreeFallINT(1);
  adxl.doubleTapINT(1);
  adxl.singleTapINT(1);

  adxl.setRangeSetting(range);           // Give the range settings
                                      // Accepted values are 2g, 4g, 8g or 16g
                                      // Higher Values = Wider Measurement Range
                                      // Lower Values = Greater Sensitivity

  adxl.setSpiBit(0);                  // Configure the device to be in 4 wire SPI mode when set to '0' or 3 wire SPI mode when set to 1
                                      // Default: Set to 1
                                      // SPI pins on the ATMega328: 11, 12 and 13 as reference in SPI Library 
   
  adxl.setActivityXYZ(1, 1, 1);       // Set to activate movement detection in the axes "adxl.setActivityXYZ(X, Y, Z);" (1 == ON, 0 == OFF)
  adxl.setActivityThreshold(50);      // 62.5mg per increment   // Set activity   // Inactivity thresholds (0-255)
 
  adxl.setInactivityXYZ(1, 1, 1);     // Set to detect inactivity in all the axes "adxl.setInactivityXYZ(X, Y, Z);" (1 == ON, 0 == OFF)
  adxl.setInactivityThreshold(75);    // 62.5mg per increment   // Set inactivity // Inactivity thresholds (0-255)
  adxl.setTimeInactivity(10);         // How many seconds of no activity is inactive?

  adxl.setTapDetectionOnXYZ(0, 0, 1); // Detect taps in the directions turned ON "adxl.setTapDetectionOnX(X, Y, Z);" (1 == ON, 0 == OFF)
 
  // Set values for what is considered a TAP and what is a DOUBLE TAP (0-255)
  adxl.setTapThreshold(50);           // 62.5 mg per increment
  adxl.setTapDuration(15);            // 625 ÃƒÅ½Ã‚Â¼s per increment
  adxl.setDoubleTapLatency(80);       // 1.25 ms per increment
  adxl.setDoubleTapWindow(200);       // 1.25 ms per increment
 
  // Set values for what is considered FREE FALL (0-255)
  adxl.setFreeFallThreshold(7);       // (5 - 9) recommended - 62.5mg per increment
  adxl.setFreeFallDuration(30);       // (20 - 70) recommended - 5ms per increment
 
  // Setting interupts to take place on INTx pin
  adxl.setImportantInterruptMapping(0, 0, 0, 0, 0s);     // Sets "adxl.setEveryInterruptMapping(single tap, double tap, free fall, activity, inactivity);" 
                                                        // Accepts only 1 or 2 values for pins INT1 and INT2. This chooses the pin on the ADXL345 to use for Interrupts.
                                                        // This library may have a problem using INT2 pin. Default to INT1 pin.
}

/****************** MAIN CODE ******************/
void loop(){
  
  // Accelerometer Readings
  int x,y,z;   
  adxl.readAccel(&x, &y, &z);  // Read the accelerometer values and store them in variables declared above x,y,z

  if (debug) {
    Serial.print(CorrectedInt(x));
    Serial.print(", ");
    Serial.print(CorrectedInt(y));
    Serial.print(", ");
    Serial.println(CorrectedInt(z));
  }

  //turn off led
  if(ledDelay == 2000){
    setColor(0,0,0);
    ledDelay++;
  }else if (ledDelay < 2000){
    ledDelay++;
  }
 
  StatusCheckFunction(x, y ,z);
  
  EmitStatusChanged(); 
}

/********************* STATUS CHECK FUNCTIONS *********************/
void StatusCheckFunction(int x, int y, int z){
  if(start){
    start = false;
    x_previous = NormalizedVal(x);
    y_previous = NormalizedVal(y);
    z_previous = NormalizedVal(z);
  }else{
    if(HasChanged(NormalizedVal(x), x_previous, threshold) || HasChanged( NormalizedVal(y), y_previous, threshold) || HasChanged( NormalizedVal(z), z_previous, threshold)){
      x_previous = NormalizedVal(x);
      y_previous = NormalizedVal(y);
      z_previous = NormalizedVal(z);
      count = 0;
      changed = true;
      if( debug ){
        Serial.print("changing...\n");
      }  
    }else{
      count++;
    }
  } 
}

void EmitStatusChanged(){
  if(changed and count>1000){
    if( debug ){
      Serial.print(x_previous);
      Serial.print(", ");
      Serial.print(y_previous);
      Serial.print(", ");
      Serial.println(z_previous);
    }   
    changed = false;

    int current_status = GetStatus(x_previous, y_previous, z_previous);
    EEPROM.put(0, current_status);

    if(current_status !=0 && current_status != prev_status){
      if( debug ){
        Serial.print("status: ");
        Serial.print(current_status);
        Serial.print("prev status: ");
        Serial.println(prev_status);
      }
      prev_status = current_status;
      sendRequest(host, mtd, payload);
    }
  }
}

/********************* UTILITY *********************/
bool HasChanged(float val, float prev, float trs){
  return (val < prev - trs || val > prev + trs);
}

bool IsInRange(float val, float target, float trs){
  return (val >= target - trs && val <= target + trs);
}

float NormalizedVal(int val){
  return (val/normalizeval);
}

int GetStatus(float x, float y, float z){
  int response = 0;
  
  if(IsInRange(z, 1, threshold)){
    mtd = "POST";
    payload = "{\"id\": \"0\"}";
    response =  1;
    setColor(255, 255, 255);
  }
  if(IsInRange(z, -1, threshold)){
    mtd = "POST";
    payload = "{\"id\": \"1\"}";
    response =  2;
    setColor(0, 255, 0);
  }
  if(IsInRange(y, 1, threshold)){
    mtd = "POST";
    payload = "{\"id\": \"2\"}";
    response =  3;
    setColor(0, 0, 255);
  }
  if(IsInRange(y, -1, threshold)){
    mtd = "POST";
    payload = "{\"id\": \"3\"}";
    response =  4;
    setColor(80, 0, 80);
  }
  if(IsInRange(x, 1, threshold)){
    mtd = "POST";
    payload = "{\"id\": \"4\"}";
    response =  5;
    setColor(0, 255, 255);
  }
  if(IsInRange(x, -1, threshold)){
    mtd = "POST";
    payload = "{\"id\": \"5\"}";
    response =  6;
    setColor(220, 200, 0);
  }

  return response;
}

void setColor(int red, int green, int blue)
{
  analogWrite(redPin, red);
  analogWrite(greenPin, green);
  analogWrite(bluePin, blue);
  ledDelay = 0;
}

void sendRequest(String url, char *mtd, String payload){
    HTTPClient http;
    std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);

    //Used to set the fingerprint
    //client->setFingerprint(fingerprint);
    client->setInsecure();
    
    WiFi.forceSleepWake();
    delay(10);

    if(password){
      WiFi.begin(ssid, password);
    }else{
      WiFi.begin(ssid); 
    }

    connectionDelay = 0;
 
    while (WiFi.status() != WL_CONNECTED && connectionDelay<60) {
      delay(500);
      connectionDelay++;
      if( debug ){
        Serial.print("Connecting..\n");
        Serial.print(connectionDelay);
      }  
    }

    if(connectionDelay > 58){
      ledDelay = 0;
      setColor(255, 0, 0);
    }
    
    http.begin(*client, url);
    http.addHeader("Content-Type", "application/json");
    http.sendRequest(mtd, payload);
    http.end();
    wifiSleep();
}

void wifiSleep(){
  WiFi.disconnect( true );
  while (WiFi.status() != WL_DISCONNECTED) {
    delay(500);
      if( debug ){
        Serial.print("Disconnect..\n");
      }
  }
    
  if( debug ){  
    Serial.print("Disconnected\n");
  }
  
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();
  delay(10);  
}