#include <Arduino.h>

#define Fona3G Serial1
#define UltGps Serial2

#define RST_FONA 5
#define KEY_FONA 7
#define PS_FONA  3

#define GPS_ENABLE 4
#define GPS_FIX 2

#define PUMP 46

enum State_enum {INIT, WHAIT_GPS, MEASURING, SEND_DATA, SLEEP};

uint8_t state = INIT;

//I2C libs
#include <Wire.h>


// SD-Card-Reader

#include <SPI.h>
#include <SD.h>
File myFile;

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

//BME280 Sensor
#include <BME280I2C.h>

BME280I2C bme;    // Default : forced mode, standby time = 1000 ms
                  // Oversampling = pressure ×1, temperature ×1, humidity ×1, filter off,


//ADS 1115 libs
#include <Adafruit_ADS1015.h>

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


// Batteryvoltage
float battery_fona = 0;
float battery_solar = 0;
float v_ref = 4.996;
float conversion_factor = v_ref / 1023;



// Config Messung
const int MessInterval = 20; // Zeit in ms zwischen den einzelnen gemittelten Messwerten
const int Messwerte_Mittel = 1; // Anzahl der Messungen, die zu einem einzelnen Messwert gemittelt werden
const int MessDelay = MessInterval / Messwerte_Mittel; /* Pause zwischen Messungen, die zu einem Messwert gemittelt werden -> alle "MessDelay" ms ein Messwert,
                                                          bis Messwerte_Mittel mal gemessen wurde, dann wird ein Mittelwert gebildet und ausgegeben. */
unsigned long time;
unsigned long millis_time;
unsigned long measurement_timer = 0;
unsigned long measurement_timeout = 8000;
unsigned long gps_timer = 0;
unsigned long gps_timeout = 7000;
unsigned long data_upload = 0;

int linesinfile = 0;
int max_linesinfile = 200;

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
char MEASUREMENT_NAME[34] = "fona3_sdlong2";  //(+ Sensornummer)



String DisplayState = "";
String Uploadstring = "";
String Geohash_fine = "", Geohash_normal= "", Geohash_coarse ="", last_geohash ="", entry_geohash ="";
// PROTOTYPES

// Fona3G Functions
#include <FONA_FUNCTIONS.h>

void PumpOn(){
  digitalWrite(PUMP,HIGH);
  }
  void PumpOff(){
    digitalWrite(PUMP,LOW);
    }



float getUmrechnungsfaktor(){
        float Faktor;
        if(ads_A.getGain()==GAIN_TWOTHIRDS) {
                Faktor = 0.1875;
        }
        if(ads_A.getGain()==GAIN_ONE) {
                Faktor = 0.125;
        }
        if(ads_A.getGain()==GAIN_TWO) {
                Faktor = 0.0625;
        }
        if(ads_A.getGain()==GAIN_FOUR) {
                Faktor = 0.03125;
        }
        return Faktor;
}

void writeLineToFile(String line){
myFile = SD.open("DATA.txt", FILE_WRITE);

// if the file opened okay, write to it:
if (myFile) {
Serial.print("Writing to data.txt...");
myFile.println(line);
// close the file:
myFile.close();
Serial.println("done.");
} else {
// if the file didn't open, print an error:
Serial.println("error opening data.txt");
}
}

void generateUploadString(){
        // Messwerte in String zusammenbauen
        Uploadstring =
                String(MEASUREMENT_NAME) + "," +

                //Tags
                "host=alphasense_mobil_1" + ","  +
                "hour=" + String(GPS.hour) + "," +
                "minute=" + String(GPS.minute) + "," +
                "geohash="        + Geohash_normal + "," +
                "geohash_fine="   + Geohash_fine   + "," +
                "geohash_normal=" + Geohash_normal + "," +
                "geohash_coarse=" + Geohash_coarse +
                " " +                                     // Leerzeichen trennt Tags und Fields!

                //Messwerte
                SN1 +    "=" + String(SN1_value, 4) + "," +
                SN2 +    "=" + String(SN2_value, 4) + "," +
                SN3 +    "=" + String(SN3_value,4) + "," +
                TEMP +   "=" + String(Temp_value, 4) + "," +
                SN1_AE + "=" + String(SN1_AE_value, 4) + "," +
                SN2_AE + "=" + String(SN2_AE_value, 4) + "," +
                SN3_AE + "=" + String(SN3_AE_value, 4) + "," +

                //Batteryvoltage & Data
                "bat_solar=" + String(battery_solar) + "," +
                "bat_mod="   + String(battery_fona) + "," +
                "data_upload=" + String(data_upload) + "," +

                //Position & Speed
                "lng=" + String(GPS.longitudeDegrees, 4) + "," +
                "lat=" + String(GPS.latitudeDegrees, 4) + "," +
                "speed=" + (GPS.speed * 1.852) +  // Knots to km/h

                " " +                                    // Leerzeichen trennt Fields und Timestamp

                String(now());

}


void STATE_INIT(){

        Serial.println("Init_State:");
        Serial.println("Nothing to do so far...starting Measurement!");

        state = MEASURING;
}

void STATE_WHAIT_GPS(){
  PumpOff();
        Serial.println("Waiting for GPS-fix...");
        if (GPS.newNMEAreceived()) {
                GPS.parse(GPS.lastNMEA());

        }
        if (GPS.fix) {
                Serial.println("Got GPS-fix! Switching to MEASURING state!");
                state = MEASURING;
        }



}

void STATE_MEASURING(){

  PumpOn();

        float SN1_Integral = 0;
        float SN2_Integral = 0;
        float SN3_Integral = 0;
        float TEMP_Integral = 0;

        float SN1_AE_Integral = 0;
        float SN2_AE_Integral = 0;
        float SN3_AE_Integral = 0;

        int measurement_counter = 0;

        //Check for GPS-fix
        if (!GPS.fix) {
                state = WHAIT_GPS;
                Serial.println("No GPS-fix! Switching to WAIT_GPS state!");
                return;
        }

        Serial.println("Start Measurement...");

        measurement_timer = millis();
        entry_geohash = hasher_normal.encode(GPS.latitudeDegrees, GPS.longitudeDegrees);
        last_geohash = entry_geohash;

        while (last_geohash == entry_geohash && millis() - measurement_timer < measurement_timeout){

        gps_timer = millis();
        while (!GPS.parse(GPS.lastNMEA())) {

          if (millis() - gps_timer > gps_timeout) {
                  Serial.println("GPS-Timeout!");
                  last_geohash = "timeout";
                  break;
          }

                // Abrufen der Analogeinganswerte am ADS1115
                // Abrufen der an den Pins anliegenden Werte
                measurement_counter += 1;
                //ADC_A
                SN1_AE_Integral+=ads_A.readADC_SingleEnded(1); // NO2   Aux 1
                SN2_AE_Integral+=ads_A.readADC_SingleEnded(2); // O3/NO Aux 2
                SN3_AE_Integral+=ads_A.readADC_SingleEnded(3); // NO    Aux 3
                TEMP_Integral  +=ads_A.readADC_SingleEnded(0);// PT+

                //ADC_B
                SN1_Integral+=ads_B.readADC_SingleEnded(1); // NO2   Work 1
                SN2_Integral+=ads_B.readADC_SingleEnded(2); // O3/NO Work 2
                SN3_Integral+=ads_B.readADC_SingleEnded(3); // NO    Work 3

                //Messung();
                //Serial.println(String(SN1_value));
        }

        last_geohash = hasher_normal.encode(GPS.latitudeDegrees, GPS.longitudeDegrees);
        Serial.println("Entry-Geohash: " + entry_geohash);
        Serial.println("Last-Geohash:  " + last_geohash);
        Serial.println("Meas.No.:      " + String(measurement_counter));


      }

        if (measurement_counter < 1) {
                Serial.println("0 Measurements. ABORTING !");
                return;
        }

        SN1_Integral = SN1_Integral / measurement_counter; // Bildung des arithmetischen Mittels
        SN2_Integral = SN2_Integral / measurement_counter;
        SN3_Integral = SN3_Integral / measurement_counter;
        TEMP_Integral = TEMP_Integral / measurement_counter;
        SN1_AE_Integral = SN1_AE_Integral / measurement_counter; // Bildung des arithmetischen Mittels
        SN2_AE_Integral = SN2_AE_Integral / measurement_counter;
        SN3_AE_Integral = SN3_AE_Integral / measurement_counter;

        Umrechnungsfaktor = getUmrechnungsfaktor();

        SN1_value = Umrechnungsfaktor * SN1_Integral;
        SN2_value = Umrechnungsfaktor * SN2_Integral;
        SN3_value = Umrechnungsfaktor * SN3_Integral;
        Temp_value = Umrechnungsfaktor * TEMP_Integral;

        SN1_AE_value = Umrechnungsfaktor * SN1_AE_Integral;
        SN2_AE_value = Umrechnungsfaktor * SN2_AE_Integral;
        SN3_AE_value = Umrechnungsfaktor * SN3_AE_Integral;

        Geohash_fine   = hasher_fine.encode(GPS.latitudeDegrees, GPS.longitudeDegrees);
        Geohash_normal = hasher_normal.encode(GPS.latitudeDegrees, GPS.longitudeDegrees);
        Geohash_coarse = hasher_coarse.encode(GPS.latitudeDegrees, GPS.longitudeDegrees);

        battery_solar = (analogRead(4) * conversion_factor * 1.5);
        battery_fona =  (analogRead(6) * conversion_factor * 1.5);



        setTime(GPS.hour,GPS.minute,GPS.seconds,GPS.day,GPS.month,GPS.year);

        //Serial.print("NOW:"); Serial.println (now());

        generateUploadString();
        data_upload += Uploadstring.length();

        Serial.println("DONE");
        linesinfile += 1;

        Serial.println(String(SN1_value));
        Serial.println(String(SN1_AE_value));
        Serial.println("Measurements taken: " + String(measurement_counter));
        Serial.println("Lines in file: " + String(linesinfile));

        Serial.println(Uploadstring);

        writeLineToFile(Uploadstring);




        if (linesinfile >= max_linesinfile) {
          Serial.println("Uploading!");
          state = SEND_DATA;
          linesinfile = 0;
          data_upload = 0;
          return;
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

void STATE_SEND_DATA(){

  PumpOff();

        Serial.println("SENDING DATA...");

        ModemTurnOn();

   delay(15000);

   QuickConnectToNetwork();
   QuickConnectToNetwork();

   delay(15000);

   Parser("AT+CHTTPSSTART", 1000);
    Parser("AT+CHTTPSOPSE=\"130.149.67.141\",8086,1", 1000);  // InfluxDB
  // Parser("AT+CHTTPSOPSE=\"130.149.67.168\",3000,1", 1000);     // Test TCP-Server

//SendFromSD();
SendLongSD();

ATCOM("AT+CHTTPSSEND", 1500);
ATCOM("AT+CHTTPSRECV=88", 1000);
ATCOM("AT+CHTTPSCLSE", 500);
ATCOM("AT+CHTTPSSTOP", 500);

delay(10000);

ModemTurnOff();

delay(10000);

        state = MEASURING;
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

        case WHAIT_GPS:
                STATE_WHAIT_GPS();

                break;

        case SEND_DATA:
                STATE_SEND_DATA();

                break;
        }
}

void UpdateDisplay(){

        display.clearDisplay();
        display.setCursor(0,0);

        display.print("State: ");
        display.println(String(state));

        display.print("V_Bat: ");
        display.println(String(battery_solar));

        display.print("V_FG3: ");
        display.println(String(battery_fona));

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
        Fona3G.begin(115200);

// SETUP SD-Card-Reader

        Serial.print("Initializing SD card..."); //SD Setup
        if (!SD.begin(53)) {
                Serial.println("initialization failed!");
                //while (1);
        }
        Serial.println("initialization done.");


        Wire.begin();

if(!bme.begin())
{
  Serial.println("Could not find BME280 sensor!");
  delay(1000);
}

// SETUP I/O-PINS
        pinMode(RST_FONA,OUTPUT); // Fona3G ResetPin
        pinMode(KEY_FONA,OUTPUT); // Fona3G KeyPin
        pinMode(PS_FONA,INPUT); // Fona3G PowerStatePin

        pinMode(GPS_ENABLE,OUTPUT); // Fona3G KeyPin
        pinMode(GPS_FIX,INPUT); // Fona3G PowerStatePin

        digitalWrite(RST_FONA,HIGH);
        digitalWrite(KEY_FONA,HIGH);
        digitalWrite(GPS_ENABLE,HIGH);

        pinMode(PUMP,OUTPUT); // Fona3G KeyPin
        digitalWrite(PUMP,LOW);

// SETUP ADC's
        ads_A.setGain(GAIN_TWO);  // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
        ads_B.setGain(GAIN_TWO);

        ads_A.begin();
        ads_B.begin();

// SETUP DISPLAY

        display.begin(SSD1306_SWITCHCAPVCC,0x3C);
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.setCursor(0,0);
        display.clearDisplay();

        display.println("Boot...");
        display.display();

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
        GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); // 1 Hz update rate
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
        UpdateDisplay();

}
