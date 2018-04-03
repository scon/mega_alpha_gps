#include <Arduino.h>


#define Fona3G Serial1
#define UltGps Serial2

enum State_enum {INIT, WHAIT_GPS, MEASURING, SLEEP};

uint8_t state = INIT;


// GPS
#include <Adafruit_GPS.h>
Adafruit_GPS GPS(&UltGps);
boolean usingInterrupt = true;
#define GPSECHO  false


// Time calculations
#include <Time.h>
#include <TimeLib.h>

// DISPLAY
//#include "ssd1306.h"
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);




//#include "sova.h"
#include "nano_gfx.h"
#include "font6x8.h"

//ADS 1115 libs
#include <Adafruit_ADS1015.h>

//I2C libs
#include <Wire.h>

//GeoHash libs
#include <arduino-geohash.h>
GeoHash hasher_coarse(7);
GeoHash hasher_normal(8);
GeoHash hasher_fine(9);

Adafruit_ADS1115 ads_A(0x48);
Adafruit_ADS1115 ads_B(0x49);

float Umrechnungsfaktor;

float SN1_value, SN2_value, SN3_value, Temp_value;      // globale ADC Variablen
float SN1_AE_value,SN2_AE_value,SN3_AE_value;           // fuer Ausgabe am Display

// Config Messung
const int MessInterval = 20; // Zeit in ms zwischen den einzelnen gemittelten Messwerten
const int Messwerte_Mittel = 5; // Anzahl der Messungen, die zu einem einzelnen Messwert gemittelt werden
const int MessDelay = MessInterval / Messwerte_Mittel; /* Pause zwischen Messungen, die zu einem Messwert gemittelt werden -> alle "MessDelay" ms ein Messwert,
bis Messwerte_Mittel mal gemessen wurde, dann wird ein Mittelwert gebildet und ausgegeben. */

unsigned long time;
unsigned long millis_time;
unsigned long counter = 0;

// Sensor Config
const char* SN1    = "NO2_WE";
const char* SN2    = "O3_WE";
const char* SN3    = "NO_WE";
const char* TEMP   = "PT1000";
const char* SN1_AE = "NO2_AE";
const char* SN2_AE = "O3_AE";
const char* SN3_AE = "NO_AE";

// Server Config
const char* InfluxDB_Server_IP = "130.149.67.141";
const int InfluxDB_Server_Port = 8086;
const char* InfluxDB_Database = "ALPHASENSE";
char MEASUREMENT_NAME[34] = "alphasense_mobil_1" ; //(+ Sensornummer)

String DisplayState = "";
String Uploadstring = "";
// PROTOTYPES


float getUmrechnungsfaktor(){
  float Faktor;
  if(ads_A.getGain()==GAIN_TWOTHIRDS){
    Faktor = 0.1875;
  }
  if(ads_A.getGain()==GAIN_ONE){
    Faktor = 0.125;
  }
  if(ads_A.getGain()==GAIN_TWO){
    Faktor = 0.0625;
  }
  if(ads_A.getGain()==GAIN_FOUR){
    Faktor = 0.03125;
  }
  return Faktor;
}


void generateUploadString(){
  // Messwerte in String zusammenbauen
  Uploadstring =
  String(MEASUREMENT_NAME) + "," +

  //Tags
  "host=alphasense_mobil_1" + ","    ; /* +
  "hour=" + String(gpsHour) + "," +
  "minute=" + String(gpsMinute) + "," +
  "geohash="        + Geohash_normal + "," +
  "geohash_fine="   + Geohash_fine   + "," +
  "geohash_normal=" + Geohash_normal + "," +
  "geohash_coarse=" + Geohash_coarse +
   " " + // Leerzeichen trennt Tags und Fields

  //Messwerte
  SN1 +    "=" + String(SN1_value , 4) + "," +
  SN2 +    "=" + String(SN2_value, 4) + "," +
  SN3 +    "=" + String(SN3_value,4) + "," +
  TEMP +   "=" + String(Temp_value, 4) + "," +
  SN1_AE + "=" + String(SN1_AE_value, 4) + "," +
  SN2_AE + "=" + String(SN2_AE_value, 4) + "," +
  SN3_AE + "=" + String(SN3_AE_value, 4) + "," +

  //Position & Speed
  "lng=" + longitude + "," +
  "lat=" + latitude + "," +
  "speed=" + gpsSpeed; */
}



void Messung(){

  float SN1_Integral = 0;
  float SN2_Integral = 0;
  float SN3_Integral = 0;
  float TEMP_Integral = 0;

  float SN1_AE_Integral = 0;
  float SN2_AE_Integral = 0;
  float SN3_AE_Integral = 0;

  millis_time = millis();
  //Messung über i = Messwerte_Mittel Messwerte
  for (int i = 0; i < Messwerte_Mittel; i++) {
    // Abrufen der Analogeinganswerte am ADS1115
    // Abrufen der an den Pins anliegenden Werte &
    //Integration über Anzahl der zu mittelnden Messwerte

    //ADC_A
    SN1_AE_Integral+=ads_A.readADC_SingleEnded(1);  // NO2   Aux 1
    SN2_AE_Integral+=ads_A.readADC_SingleEnded(2);  // O3/NO Aux 2
    SN3_AE_Integral+=ads_A.readADC_SingleEnded(3);  // NO    Aux 3
    TEMP_Integral  +=ads_A.readADC_SingleEnded(0);  // PT+

    //ADC_B
    SN1_Integral+=ads_B.readADC_SingleEnded(1); // NO2   Work 1
    SN2_Integral+=ads_B.readADC_SingleEnded(2); // O3/NO Work 2
    SN3_Integral+=ads_B.readADC_SingleEnded(3); // NO    Work 3

    // delay(15);
  }

   millis_time = millis() - millis_time;

   SN1_Integral = SN1_Integral / Messwerte_Mittel; // Bildung des arithmetischen Mittels
   SN2_Integral = SN2_Integral / Messwerte_Mittel;
   SN3_Integral = SN3_Integral / Messwerte_Mittel;
   TEMP_Integral = TEMP_Integral / Messwerte_Mittel;
   SN1_AE_Integral = SN1_AE_Integral / Messwerte_Mittel; // Bildung des arithmetischen Mittels
   SN2_AE_Integral = SN2_AE_Integral / Messwerte_Mittel;
   SN3_AE_Integral = SN3_AE_Integral / Messwerte_Mittel;

   Umrechnungsfaktor = getUmrechnungsfaktor();

   SN1_value = Umrechnungsfaktor * SN1_Integral;
   SN2_value = Umrechnungsfaktor * SN2_Integral;
   SN3_value = Umrechnungsfaktor * SN3_Integral;
   Temp_value = Umrechnungsfaktor * TEMP_Integral;

   SN1_AE_value = Umrechnungsfaktor * SN1_AE_Integral;
   SN2_AE_value = Umrechnungsfaktor * SN2_AE_Integral;
   SN3_AE_value = Umrechnungsfaktor * SN3_AE_Integral;
}

void STATE_INIT(){

Serial.println("Init_State:");
Serial.println("Nothing to do so far...starting Measurement");

state = MEASURING;
}

void STATE_MEASURING(){

  Serial.println("Measuring...");
  Messung();
  Serial.println(String(SN1_value));

  if (GPS.newNMEAreceived()) {
  // a tricky thing here is if we print the NMEA sentence, or data
  // we end up not listening and catching other sentences!
  // so be very wary if using OUTPUT_ALLDATA and trytng to print out data
  //Serial.println(GPS.lastNMEA());   // this also sets the newNMEAreceived() flag to false

  if (!GPS.parse(GPS.lastNMEA())){   // this also sets the newNMEAreceived() flag to false
    Serial.println("NoSentence");
    return;  // we can fail to parse a sentence in which case we should just wait for another#
  }
}

Serial.print("\nTime: ");
    Serial.print(GPS.hour, DEC); Serial.print(':');
    Serial.print(GPS.minute, DEC); Serial.print(':');
    Serial.print(GPS.seconds, DEC); Serial.print('.');
    Serial.println(GPS.milliseconds);
    Serial.print("Date: ");
    Serial.print(GPS.day, DEC); Serial.print('/');
    Serial.print(GPS.month, DEC); Serial.print("/20");
    Serial.println(GPS.year, DEC);
    Serial.print("Fix: "); Serial.print((int)GPS.fix);
    Serial.print(" quality: "); Serial.println((int)GPS.fixquality);
    if (GPS.fix) {
      Serial.print("Location: ");
      Serial.print(GPS.latitude, 4); Serial.print(GPS.lat);
      Serial.print(", ");
      Serial.print(GPS.longitude, 4); Serial.println(GPS.lon);
      Serial.print("Location (in degrees, works with Google Maps): ");
      Serial.print(GPS.latitudeDegrees, 4);
      Serial.print(", ");
      Serial.println(GPS.longitudeDegrees, 4);

      Serial.print("Speed (knots): "); Serial.println(GPS.speed);
      Serial.print("Angle: "); Serial.println(GPS.angle);
      Serial.print("Altitude: "); Serial.println(GPS.altitude);
      Serial.print("Satellites: "); Serial.println((int)GPS.satellites);

      setTime(GPS.hour,GPS.minute,GPS.seconds,GPS.day,GPS.month,GPS.year);

      Serial.print("NOW:"); Serial.println (now());
}



}

void state_machine_run()
{
  switch(state)
  {
    case INIT:
    STATE_INIT();
      break;

    case MEASURING:
    STATE_MEASURING();

      break;

  }
}


void UpdateDisplay(){

  display.clearDisplay();
  display.setCursor(0,0);

  display.print("State: ");
  display.println(String(state));
  display.display();

}

// Interrupt is called once a millisecond, looks for any new GPS data, and stores it
SIGNAL(TIMER0_COMPA_vect) {
  //Serial.println("Interrrupt");
  char c = GPS.read();
  // if you want to debug, this is a good time to do it!
#ifdef UDR0
  if (GPSECHO)
    if (c) UDR0 = c;
    // writing direct to UDR0 is much much faster than Serial.print
    // but only one character can be written at a time.
#endif
}

void useInterrupt(boolean v) {
  if (v) {
    // Timer0 is already used for millis() - we'll just interrupt somewhere
    // in the middle and call the "Compare A" function above
    OCR0A = 0xAF;
    TIMSK0 |= _BV(OCIE0A);
    usingInterrupt = true;
  } else {
    // do not call the interrupt function COMPA anymore
    TIMSK0 &= ~_BV(OCIE0A);
    usingInterrupt = false;
  }
}



void setup(){
// SETUP SERIAL
Serial.begin(115200);
Serial.println("Setup...");

// SETUP ADC's
  ads_A.setGain(GAIN_TWO);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
  ads_B.setGain(GAIN_TWO);

  ads_A.begin();
  ads_B.begin();

// SETUP DISPLAY


display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
display.setTextSize(1);
display.setTextColor(WHITE);
display.setCursor(0,0);
display.clearDisplay();

display.println("Boot...");
display.display();
  /* Select the font to use with menu and all font functions */
//  ssd1306_setFixedFont(ssd1306xled_font6x8);
  /* Do not init Wire library for Attiny controllers */
//  ssd1306_128x64_i2c_init();

//  ssd1306_clearScreen();

// Setup GPS


Serial.println("Adafruit GPS library basic test!");

// 9600 NMEA is the default baud rate for Adafruit MTK GPS's- some use 4800
GPS.begin(9600);

// uncomment this line to turn on RMC (recommended minimum) and GGA (fix data) including altitude
GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
// uncomment this line to turn on only the "minimum recommended" data
//GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
// For parsing data, we don't suggest using anything but either RMC only or RMC+GGA since
// the parser doesn't care about other sentences at this time

// Set the update rate
GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);   // 1 Hz update rate
// For the parsing code to work nicely and have time to sort thru the data, and
// print it out we don't suggest using anything higher than 1 Hz

// Request updates on antenna status, comment out to keep quiet
GPS.sendCommand(PGCMD_ANTENNA);

// the nice thing about this code is you can have a timer0 interrupt go off
// every 1 millisecond, and read data from the GPS for you. that makes the
// loop code a heck of a lot easier!
useInterrupt(true);

OCR0A = 0xAF;
TIMSK0 |= _BV(OCIE0A);

delay(1000);
// Ask for firmware version
UltGps.println(PMTK_Q_RELEASE);


}

void loop() {

state_machine_run();
Serial.println("MainLoop.");
UpdateDisplay();

}
