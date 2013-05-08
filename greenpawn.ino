//This is the basic SDFat Webserver sketch, without any modifications
#include <SPI.h>
#include <SdFat.h>
#include <SdFatUtil.h>
#include <Ethernet.h>
#include "DHT.h"

byte mac[] = {
  0x90,0xA2,0xDA,0x00,0xFA,0x12};
byte ip[] = {
  192,168,1,5};
char rootFileName[] = "index.htm";  //root file name is what the homepage will be.
EthernetServer server = EthernetServer(80);

//Sensor Stuff
int NUMSENSORS = 4; //8 is max
prog_char sensor1[] PROGMEM = "nothing";
prog_char sensor2[] PROGMEM = "moisture";
prog_char sensor3[] PROGMEM = "light"; 
prog_char sensor4[] PROGMEM = "buffer";
prog_char sensor5[] PROGMEM = "dhttemp";
prog_char sensor6[] PROGMEM = "humidity";
prog_char sensor7[] PROGMEM = "bmptemp"; 
prog_char sensor8[] PROGMEM = "pressure";
PROGMEM const char *sensorArray[] =
{
    sensor1,
    sensor2,
    sensor3,
    sensor4,
    sensor5,
    sensor6,
    sensor7,
    sensor8 };
    
char buffer[16]; 

#define SENSORPOWERPIN 8  //Sensor Power Pin
#define MOISTUREPIN 2  //Moisture sensor pin
#define PHOTOPIN 3    //Photo Sensor Pin
#define DHTPIN 2     // what pin we're connected to
#define DHTTYPE DHT11   // DHT 11
DHT dht(DHTPIN, DHTTYPE);
int sensorStatus [] = {0,0,0,0,0,0,0,0};
unsigned long int loopCount = 0;
unsigned long int cycleInterval = 100000; // 100000 is reasonable 494967294 is max;

#define ZONE1WATERPIN 7
boolean wateringZone1 = false;

Sd2Card card;  //SD Stuff
SdVolume volume;
/*SdFile root;
SdFile file;*/

#define error(s) error_P(PSTR(s))  //SD card errors stored in Program memory
void error_P(const char* str) {  //Error function
  PgmPrint("error: ");
  SerialPrintln_P(str);
  if (card.errorCode()) {
    PgmPrint("SD error: ");
    Serial.print(card.errorCode(), HEX);
    Serial.print(',');
    Serial.println(card.errorData(), HEX);
  }
  while(1);
}

void setup() {  //setup stuff
  Serial.begin(9600); //256000);
 // PgmPrint("Free RAM: ");
 // Serial.println(freeRam());
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);
  if (!card.init(SPI_FULL_SPEED, 4)) error("card.init failed!");  //If you are having errors when reading from the SD card, change FULL to HALF
 // PgmPrintln("Done");
  initSensors();
  pinMode(ZONE1WATERPIN, OUTPUT); 
  digitalWrite(ZONE1WATERPIN, LOW);
  Ethernet.begin(mac, ip);
  server.begin();
}
#define BUFSIZ 100  //defines the buffer size.  100 gives plenty of room.  reduce size if more ram is needed.

void loop()
{
  char clientline[BUFSIZ];
  char *filename;
  int index = 0;
  int image = 0;
  EthernetClient client = server.available();
  
  if(loopCount == 0) { //It's time to read the sensors!
    readSensors();
    waterGardens();
  }
  loopCount++;
  if(loopCount > cycleInterval) { //Is it time to read the sensors?
    loopCount = 0;
  }
  
  if (client) {
    boolean current_line_is_blank = true;
    index = 0;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (c != '\n' && c != '\r') {
          clientline[index] = c;
          index++;
          if (index >= BUFSIZ)
            index = BUFSIZ -1;
          continue;
        }
        clientline[index] = 0;
        filename = 0;
        //Serial.println(clientline);
        if (strstr(clientline, "GET / ") != 0) {  //If you are going to the homepage, the filename is set to the rootFileName
          filename = rootFileName;
        }
        if (strstr(clientline, "GET /") != 0) {
          if (!filename) filename = clientline + 5;  //gets rid of the GET / in the filename
          (strstr(clientline, " HTTP"))[0] = 0;  //gets rid of everything from HTTP to the end of the domain.   
          if (strstr(filename, "status")) {
            readSensors();
            waterGardens();
           // client.println("Content-Type: application/json");
            client.print("{\"type\":\"EnvSensorStatus\",");
            client.print("\"status\":{");
      
            client.print("\"");
            client.print("watering");
            client.print("\":\"");
            client.print(wateringZone1);
            client.print("\",");

            for (int i = 0; i < NUMSENSORS; i++){
              client.print("\"");
              client.print(strcpy_P(buffer, (char*)pgm_read_word(&(sensorArray[i])))); // Necessary casts and dereferencing, just copy.);
              client.print("\":\"");
              client.print(sensorStatus[i]);
              client.print("\"");
              if(i+1 < NUMSENSORS){
               client.print(","); 
              }
            }
            client.println("}}");
          }
          if (strstr(filename, "index")) {
            client.println("<div class='pawn greeting'>Hello, I am an Arduino!</div>");
          }
        } 
        else {
          client.println();
          client.println("<div class='pawn error404'>File Not Found!</div>");
        }
        break;
      }
    }
    delay(1);
    client.stop();
  }
}

int initSensors() {
  // Do something here to set up or import a configuration table.
  // Maybe files with different modes
  // What pins? What settings for them? How often to poll and what actions to take as a result.
  // [PinNum, read mode (frequency?),  
  
  pinMode(SENSORPOWERPIN, OUTPUT);
  pinMode(MOISTUREPIN, INPUT);
  pinMode(PHOTOPIN, INPUT);
  pinMode(DHTPIN, INPUT);  
  //Serial.println("Pinmodes set.");
  digitalWrite(SENSORPOWERPIN, LOW);
  //Serial.println("Sensors loading.");
  /*if (!bmp.begin()) { 
	Serial.println("Could not find a valid BMP085 sensor, check wiring!");
	while (1) {}
  }
  Serial.println("BMP085 (pressure and humidity) loaded.");*/
  
  dht.begin();
  /*Serial.println("DHT (temp and humidity) loaded.");
  Serial.println("Sensors on line.");*/
}

int readSensors() { 
   digitalWrite(SENSORPOWERPIN, HIGH);
   sensorStatus[0] = 0;
   sensorStatus[1] = analogRead(MOISTUREPIN);
   sensorStatus[2] = analogRead(PHOTOPIN);
   sensorStatus[3] = analogRead(DHTPIN);
   //sensorStatus[4] = dht.readHumidity();
   //sensorStatus[5] = dht.readTemperature();
   sensorStatus[6] = 0; // = bmp.readPressure();
   
   digitalWrite(SENSORPOWERPIN, LOW);
   /*Serial.print("Sensor reads: ");

   for (int i = 0; i < NUMSENSORS; i++){
      Serial.println(sensorStatus[i]);    
    }  
    Serial.println("Sensors Read.");*/
}

int waterGardens() {
  if(sensorStatus[1] < 400) { //That's pretty dry! let's water!
    digitalWrite(ZONE1WATERPIN, HIGH);
    wateringZone1 = true;
  } 
  if(wateringZone1 && sensorStatus[1] > 500) {
   digitalWrite(ZONE1WATERPIN, LOW);
   wateringZone1 = false;
  } 
}

int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}
