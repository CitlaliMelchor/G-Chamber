/**************************************************************************************************
Libraries used in the code
**************************************************************************************************/
#include "ThingSpeak.h"
#include "Wire.h"
#include <RTClib.h>
#include "DHT.h"
#include <NDIR_I2C.h>
#include <SPI.h>
#include <Ethernet.h>
#include "stdio.h"
#include "SD.h"
//#include <ArduinoUnit.h>

/*************************************************************
G-CHAMBER CONFIGURATION 
*************************************************************/
DateTime light=DateTime(0,0,0,11,0,0); //starts at 8 am
DateTime night=DateTime(0,0,0,23,00,0); //starts at 8 pm
float Td=20;  //Temperature setpoint during the light period
float Tn=20;  //Temperature setpoint during the dark period
float CO2SP=1000; //ppm
/**************************************************************************************************
Global variables definition
****************************************************************************************************/
File myFile; //Create a variable for the File

#define USE_ETHERNET_SHIELD
#define DHTPIN 8
#define DHTTYPE DHT22
 
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
EthernetClient client;

#ifdef ARDUINO_ARCH_AVR
  // On Arduino:  0 - 1023 maps to 0 - 5 volts
  #define VOLTAGE_MAX 5.0
  #define VOLTAGE_MAXCOUNTS 1023.0
#elif ARDUINO_SAMD_MKR1000
  // On MKR1000:  0 - 1023 maps to 0 - 3.3 volts
  #define VOLTAGE_MAX 3.3
  #define VOLTAGE_MAXCOUNTS 1023.0
#elif ARDUINO_SAM_DUE
  // On Due:  0 - 1023 maps to 0 - 3.3 volts
  #define VOLTAGE_MAX 3.3
  #define VOLTAGE_MAXCOUNTS 1023.0  
#elif ARDUINO_ARCH_ESP8266
  // On ESP8266:  0 - 1023 maps to 0 - 1 volts
  #define VOLTAGE_MAX 1.0
  #define VOLTAGE_MAXCOUNTS 1023.0
#endif

unsigned long myChannelNumber = 307352; //Channel number for Thingspeak 
const char * myWriteAPIKey = "HSHJRC9UAUG0YHE7"; //16X2I2D7QZ8DJQBW

DHT dht(DHTPIN, DHTTYPE);
NDIR_I2C mySensor(0x4D); //Adaptor's I2C address (7-bit, default: 0x4D)


/*********************************************************************************************************
Define where the relays are connected
**********************************************************************************************************/
int LAMPS=2;
int PUMP=3;
int FREEZER=5;
int CO2=6;
int HFAN=7;
int CO2SENSOR=9;  ///////////////////////////////////////////////////////////////////////////////////////////////////////
/***********************************************************************************************************
Set the lenght of the light period with the hours lamps turn on and off; and the watering hour
*************************************************************************************************************/
RTC_DS3231 rtc;
DateTime wHour=DateTime(0,0,0,8,00,0); //watering plants hour!!
DateTime NowYear=DateTime(2018,0,0,0,0);

 /*************************************************************************************************************
---------------------------------------------------------------------------------------------------------------
**************************************************************************************************************/

void setup() {
  Serial.begin(9600);
  //rtc.adjust(DateTime(2018,01,23,15,46,0)); //blocked keep hour settings
  /* Set the outputs*/
  dht.begin();
  pinMode (CO2SENSOR, OUTPUT);
  digitalWrite(CO2SENSOR, LOW);
  
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, lets set the time!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  pinMode (LAMPS, OUTPUT);
  pinMode (PUMP, OUTPUT);
  pinMode (FREEZER, OUTPUT);
  pinMode (CO2, OUTPUT);
  pinMode (HFAN, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(4, OUTPUT);
  
  
  digitalWrite(LAMPS, HIGH);
  digitalWrite(FREEZER, HIGH);
  digitalWrite(CO2, HIGH);
  digitalWrite(HFAN, HIGH);
  digitalWrite(PUMP, HIGH);
  digitalWrite(CO2SENSOR, LOW);

//  digitalWrite(10, HIGH);
//  digitalWrite(4, LOW);
//  
//  if(!SD.begin(4)){
//    Serial.println("initialization failed!");
//    return;}
//  char filename []="00000000.csv";
  digitalWrite(4, HIGH);
  digitalWrite(10,LOW);
  
  
  /*Internet connection*/

//Serial.println("conection loading...");  
Ethernet.begin(mac);
ThingSpeak.begin(client);
Serial.println("Connected to internet!");
Serial.println(Ethernet.localIP());



    if (mySensor.begin()) {
        Serial.println("Wait 10 seconds for sensor initialization...");
        delay(10000);
    } else {
        Serial.println("ERROR: Failed to connect to the sensor.");
        while(1);
    }
Serial.println("DATE;HOUR;T;RH;VOLTS;PPMC" ); 


}

/*****************************************************************************************************************
------------------------------------------------------------------------------------------------------------------
*****************************************************************************************************************/

void loop() {
    if (! mySensor.measure()) {
        Serial.println("Sensor communication error.");
    }
    
    DateTime now = rtc.now();
    if (now.year()==NowYear.year()){
    float t=dht.readTemperature();
        if (isnan(t)){
      digitalWrite(LAMPS, HIGH);
      digitalWrite(FREEZER, HIGH);
      digitalWrite(CO2, HIGH);
      digitalWrite(HFAN, HIGH);
      digitalWrite(PUMP, HIGH);
      Serial.println("DHT error!");}
    
    float h=dht.readHumidity();
    float sH=analogRead(A15); // Soil humidity sensor
      
    if (! mySensor.ppm){
      digitalWrite(LAMPS, HIGH);
      digitalWrite(FREEZER, HIGH);
      digitalWrite(CO2, HIGH);
      digitalWrite(HFAN, HIGH);
      digitalWrite(PUMP, HIGH);
      Serial.println("CO2 sensor error!");}
    float ppmCO2=float(mySensor.ppm);
    
    SetLampsOFF(t,h,Tn); 
    SetLampsON(t,h,Td,CO2SP,ppmCO2);
    sdWrite(t,h,sH,ppmCO2);
    //Serial.println(Ethernet.localIP());
      
   if (!ThingSpeak.setField(1,t)){
    Serial.println("no internet connection");
   }
   
   if (!ThingSpeak.setField(2,h)){
    Serial.println("no internet connection");
   }
   if (!ThingSpeak.setField(6,sH)){
    Serial.println("no internet connection");
   }
   if (!ThingSpeak.setField(7,ppmCO2)){
    Serial.println("no internet connection");
   }
   
    
        
    // Write the fields that you've set all at once.
    
   if (!ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey)){
    Serial.println("no internet connection"); 
   }

  }
  else {
    digitalWrite(LAMPS, HIGH);
    digitalWrite(FREEZER, HIGH);
    digitalWrite(CO2, HIGH);
    digitalWrite(HFAN, HIGH);
    digitalWrite(PUMP, HIGH);
    Serial.println("Clock error!");
    Serial.print(now.year(), DEC);
    }

  delay(60000); // ThingSpeak will only accept updates every 15 seconds. 
}
/*****************************************************************************************************************
 Some aditional Functions
*******************************************************************************************************************/
void SetLampsON(float t, float h, float Td,float CO2SP, float ppm){
  DateTime now=rtc.now();
 
  if (now.hour()==light.hour() && now.minute()==light.minute() && now.second()==light.second()){
    digitalWrite(LAMPS, LOW); //LOW to turn on with rele module
    if (!ThingSpeak.setField(3,1)){
      Serial.println("no internet connection");
    }
    LightTemp(t,h,Td);
    LightRH(t,h);
    CO2con(CO2SP,ppm);
  }
  
  else if (now.hour()>=light.hour() && now.hour()<night.hour()){
    digitalWrite(LAMPS,LOW);
    if (!ThingSpeak.setField(3,1)){
      Serial.println("no internet connection");
    }
    LightTemp(t,h,Td);
    LightRH(t,h);
    CO2con(CO2SP,ppm);
  }
 //watering();
}


void SetLampsOFF(float t, float h, float Tn){
  DateTime now=rtc.now();
  if (now.hour()==night.hour() && now.minute()==night.minute() && now.second()==night.second()){
    digitalWrite(LAMPS, HIGH);
    if (!ThingSpeak.setField(3,0)){
      Serial.println("no internet connection");
    }
    NightTemp(t,h,Tn);
    NightRH(t,h);
  }  
  else if (now.hour()>=night.hour()){
    digitalWrite(LAMPS,HIGH);
    if (!ThingSpeak.setField(3,0)){
      Serial.println("no internet connection");
    }
    
    NightTemp(t,h,Tn);
    NightRH(t,h);
  }
  else if (now.hour()<=light.hour()){
    digitalWrite(LAMPS,HIGH);
    if (!ThingSpeak.setField(3,0)){
      Serial.println("no internet connection!");
    }
    NightTemp(t,h,Tn);
    NightRH(t,h);
  }  
}

void LightTemp(float t, float h, float Td){
  if ((t-Td)>=.5){
    digitalWrite(FREEZER, LOW);
     if (!ThingSpeak.setField(4,1)){
      //create a channel that shows the freezer status (ON)
      Serial.println("no internet connection!");
     }
    }
  else if ((t-Td)<=-0.2){
    digitalWrite(FREEZER, HIGH);
     if (!ThingSpeak.setField(4,0)){
      //create a channel that shows the freezer status (OFF)
      Serial.println("no internet connection");
     }
    }}
    
void NightTemp(float t, float h,float Tn){
  if ((t-Tn)>=2){
    digitalWrite(FREEZER, LOW);
    if (!ThingSpeak.setField(4,1)){ //create a channel that shows the freezer status (ON){
    Serial.println("no internet connection");
    }}
  else if ((t-Tn)<=0){
    digitalWrite(FREEZER, HIGH);
    if (!ThingSpeak.setField(4,0)){ //create a channel that shows the freezer status (OFF)
    Serial.println("no internet connection");
  }}}


void LightRH(float t, float h){

  //float Wsp=8; //Water ratio during the light period
  //float w=W(t,h);
  float RHsp=50;
  if ((h-RHsp)>=1){
 //   digitalWrite(DFAN, HIGH);
    digitalWrite(HFAN, HIGH);
    if (!ThingSpeak.setField(5,0)){
      Serial.println("no internet connection");
    }
 }
  else if ((h-RHsp)<=-5){

    digitalWrite(HFAN,LOW);
    if(!ThingSpeak.setField(5,1)){
      Serial.println("no internet connection");
    }

    }}

void NightRH(float t, float h){
  float RHsp=80;
  if ((h-RHsp)>=1){
    digitalWrite(HFAN, HIGH);
    //digitalWrite(VAPOR, LOW);
    if (!ThingSpeak.setField(5,-1)){
      Serial.println("no internet connection");
    }
    }
  else if ((h-RHsp)<=-5){
    digitalWrite(HFAN,LOW);
    //digitalWrite(VAPOR,HIGH);
    if (!ThingSpeak.setField(5,1)){
      Serial.println("no internet connection");
    }

    }}


//*****************************************Watering function***************************************

//***************************************************************************************************
float watering(){
  DateTime now=rtc.now();
  if (now.hour()==wHour.hour() && now.minute()==wHour.minute()){
    digitalWrite(PUMP, LOW);
    digitalWrite(CO2SENSOR, HIGH);
    Serial.println("Watering plants");
    delay (10000);
    digitalWrite(PUMP, HIGH);
    digitalWrite(CO2SENSOR, LOW);
 }
  else{
    digitalWrite(PUMP,HIGH); 
    digitalWrite(CO2SENSOR, LOW);
 }
}


float sdWrite(float t,float h,float sH,float ppmCO2){
    //digitalWrite(10,HIGH);
    //digitalWrite(4,LOW);
    
    DateTime now=rtc.now();

//    String FileName=String(now.year());
//    FileName+=String(now.month());
//    FileName+=String(now.day());
//    FileName+=".csv";
    
    String dataString = ""; //Creates an empty string
    dataString += String(now.year()); //Add the Time variable to the empty dataString
    dataString += "/"; //Add the Time variable to the empty dataString
    dataString += String(now.month()); //Add the Time variable to the empty dataString
    dataString += "/"; //Add the Time variable to the empty dataString
    dataString += String(now.day()); //Add the Time variable to the empty dataString
    dataString += " "; //Add the Time variable to the empty dataString
    dataString += String(now.hour()); //Add the Time variable to the empty dataString
    dataString += ":"; //Add the Time variable to the empty dataString
    dataString += String(now.minute()); //Add the Time variable to the empty dataString
    dataString += ":"; //Add the Time variable to the empty dataString
    dataString += String(now.second()); //Add the Time variable to the empty dataString
    dataString += ";"; //Add a comma  to the dataString
    dataString += String(t); //Add the temperature variable to the dataString
    dataString += ";"; //Add a comma  to the dataString
    dataString += String(h); //Add the RH variable  to the dataString
    dataString += ";"; //Add a comma  to the dataString
    dataString += String(sH); 
    dataString += ";"; //Add a comma  to the dataString
    dataString += String(ppmCO2); //Add the CO2 variable  to the dataString
  if (!Serial.println(dataString)){
    digitalWrite(LAMPS, HIGH);
    digitalWrite(FREEZER, HIGH);
    digitalWrite(CO2, HIGH);
    digitalWrite(HFAN, HIGH);
    digitalWrite(PUMP, HIGH);
    Serial.end();
    delay(1000);
    Serial.begin(9600);  
  }
//    File myFile=SD.open(FileName,FILE_WRITE);
//    if(myFile){
//      myFile.println(dataString);
//      myFile.close();}
//    else{
//      Serial.println("error opening datalog");}
//  digitalWrite(4,HIGH);
//  digitalWrite(10,LOW);
//  Ethernet.begin(mac);
//  delay(2000);
}


void CO2con(float CO2SP, float ppm){
 digitalWrite(CO2,HIGH); //TURN OFF CO2 VALVE  
 float delta=CO2SP-ppm;
 if (delta>=200){
   int pulse=1.779*delta;
   digitalWrite(CO2, LOW); //Turn on CO2 valve
   delay(pulse);
   digitalWrite(CO2,HIGH); }
 if (delta<=-200){
  digitalWrite(CO2, HIGH); }
 }
