// 17.07.2020 17:21 v.0.9.1.5 

#include <Arduino.h>
#include <avr/wdt.h>
#include <ArduinoJson.h>

#define Fona3G Serial1
#define UltGps Serial2

#define RST_FONA 5
#define KEY_FONA 7
#define PS_FONA  3

#define GPS_ENABLE 4
#define GPS_FIX 2

#define MODE_SWITCH  32
#define BTN_T1  10
#define BTN_T2  11
#define BTN_SEND  47

#define PUMP 46

#define POM

#define ACC_MOVEMENT_PIN 30
#define ACC_SHUTDOWN_PIN 31
#define BOD_PIN 28
#define ADC_BAT_SOLAR 4
#define ADC_BAT_FONA 6

//I2C libs
#include <Wire.h>

//Low-Power
#include "LowPower.h"

// SD-Card-Reader
#include <SPI.h>
#include <SD.h>
File myFile;

// VON MAX
File sendFile;
File restFile;
File dataFile;

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

//GeoHash libs
#include <arduino-geohash.h>
GeoHash hasher_coarse(7);
GeoHash hasher_normal(8);
GeoHash hasher_fine(9);

Adafruit_ADS1115 ads_A(0x48);
Adafruit_ADS1115 ads_B(0x49);

//BME280 Sensor
#include <BME280I2C.h>

BME280I2C bme;    // Default : forced mode, standby time = 1000 ms
                  // Oversampling = pressure ×1, temperature ×1, humidity ×1, filter off,


float Umrechnungsfaktor;

float SN1_value, SN2_value, SN3_value, Temp_value;      // globale ADC Variablen
float SN1_AE_value,SN2_AE_value,SN3_AE_value;           // fuer Ausgabe am Display

// Batteryvoltage
float battery_fona = 0;
float battery_solar = 0;
float v_ref = 3.3;
float conversion_factor = v_ref / 1023.0;
float adc_bat_fona_coeff = 0.97;
float adc_bat_solar_coeff = 1.00;
float battery_threshold = 3.2;

// Accelerometer

float acc_vektor=0;

int acc_X =0;
int acc_Y =0;
int acc_Z =0;

int acc_X_last =0;
int acc_Y_last =0;
int acc_Z_last =0;

int acc_X_abs =0;
int acc_Y_abs =0;
int acc_Z_abs =0;

int acc_offset = 345;

bool acc_flag = false;
bool acc_timer_flag = false;
bool station_mode = false;

unsigned long acc_timer= 0;
unsigned long acc_sleep_timeout=120000;

// Config Messung
const int MessInterval = 20; // Zeit in ms zwischen den einzelnen gemittelten Messwerten
const int Messwerte_Mittel = 1; // Anzahl der Messungen, die zu einem einzelnen Messwert gemittelt werden
const int MessDelay = MessInterval / Messwerte_Mittel; /* Pause zwischen Messungen, die zu einem Messwert gemittelt werden -> alle "MessDelay" ms ein Messwert,
                                                          bis Messwerte_Mittel mal gemessen wurde, dann wird ein Mittelwert gebildet und ausgegeben. */
unsigned long time;
unsigned long millis_time;
unsigned long measurement_timer = 0;
unsigned long measurement_timeout = 10000;

unsigned long station_measurement_time = 30000; //30000; // DEFAULT 30000 / Time to aquire a single datapoint in station mode 
unsigned long station_measurement_timer = 0; // init 0
unsigned long station_max_loops = 120; // normal 120 :> 120 mal 0,5 Minuten = 1 h

unsigned long gps_timer = 0;
unsigned long gps_timeout = 7000;
unsigned long data_upload = 0;

//GPS Time correction

time_t old_gps_time;
time_t new_gps_time;

String old_time_str = "";
String new_time_str = "";


unsigned long last_corr_time = 0;


int linesinfile = 0;
int max_linesinfile = 25;

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
char MEASUREMENT_NAME[34] = "fona3_sdlong3";  //(+ Sensornummer)

enum State_enum {INIT, WHAIT_GPS, MEASURING, SEND_DATA, CHARGE, SLEEP, TRANS_SLEEP, TELEMETRY};
enum State_station_enum {STATION_INIT, STATION_WHAIT_GPS, STATION_MEASURING, STATION_SEND_DATA, STATION_EXTRACT};
uint8_t state = INIT;
uint8_t state_station = STATION_WHAIT_GPS;

int station_measurement_counter = 0;

String DisplayState = "";
String Uploadstring = "";
String Geohash_fine = "", Geohash_normal= "", Geohash_coarse ="", last_geohash ="", entry_geohash ="";
String TripID = "notset";

String STN_ID = "STN_3";

// PROTOTYPES

// Fona3G Functions
#include <FONA_FUNCTIONS.h>

// Fona3G Functions
#include <NEW_FONA_FUNCTIONS.h>

// other Functions
#include <FUNCTIONS.h>

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

void STATE_INIT(){

        // Debug SD Functions
        //divideFile(20);
        //restToNewDataFile();
        //divideFile(20);


        Serial.println("Init_State:");
        Serial.println("Nothing to do so far...");
        //Serial.println(String(divideFile(20)));



        for (size_t i = 0; i < 10; i++) {
          UpdateBatteryVoltageRaeadings();
        }


        if (digitalRead(ACC_MOVEMENT_PIN)==HIGH) {
          GpsOn();
          PumpOn();
          state = WHAIT_GPS;
        } else {
          state = TELEMETRY;
        }



 delay(100);
 //digitalWrite(ACC_SHUTDOWN_PIN,LOW);
}

void STATE_CHARGE(){


}

void STATE_WHAIT_GPS(){

        //PumpOff();
        Serial.println("Waiting for GPS-fix...");
        if (GPS.newNMEAreceived()) {
                GPS.parse(GPS.lastNMEA());

        }
        if (GPS.fix) {
                Serial.println("Got GPS-fix! Switching to MEASURING state!");
                state = MEASURING;
                if (TripID == "notset"){
                  TripID = String(GPS.year) + "-" +String(GPS.month) + "-" + String(GPS.day) +"/" + String(GPS.hour)+":" + String(GPS.minute);
                  }
        }

        if (digitalRead(ACC_MOVEMENT_PIN) == LOW) {
                state = SEND_DATA;
        }

        if (CheckBattery() == false) {
                //state =  CHARGE;
        }



}

void STATE_MEASURING(){

        PumpOn();
        GpsOn();

        float SN1_Integral = 0;
        float SN2_Integral = 0;
        float SN3_Integral = 0;
        float TEMP_Integral = 0;

        float SN1_AE_Integral = 0;
        float SN2_AE_Integral = 0;
        float SN3_AE_Integral = 0;

        float battery_solar_Integral = 0;
        float battery_fona_Integral = 0;

        int measurement_counter = 0;

        //Check for GPS-fix
        if (!GPS.fix) {
                state = WHAIT_GPS;
                Serial.println("No GPS-fix! Switching to WAIT_GPS state!");
                return;
        }

        Serial.println("Start Measurement...");

        measurement_timer = millis();
        entry_geohash = hasher_fine.encode(GPS.latitudeDegrees,GPS.longitudeDegrees);
        last_geohash = entry_geohash;

        //ADC Warmup
        for (size_t w = 0; w < 5; w++) {
        ads_A.readADC_SingleEnded(1);
        ads_A.readADC_SingleEnded(2);
        ads_A.readADC_SingleEnded(3);
        ads_A.readADC_SingleEnded(4);
        ads_B.readADC_SingleEnded(1);
        ads_B.readADC_SingleEnded(2);
        ads_B.readADC_SingleEnded(3);
        ads_B.readADC_SingleEnded(4);
        }

        while (last_geohash == entry_geohash && millis() - measurement_timer < measurement_timeout) {

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
                        SN1_Integral+=ads_A.readADC_SingleEnded(1); // NO2   Work 1
                        SN2_Integral+=ads_A.readADC_SingleEnded(2); // O3/NO Work 2
                        SN3_Integral+=ads_A.readADC_SingleEnded(3); // NO    Work 3
                        TEMP_Integral  +=ads_A.readADC_SingleEnded(0);// PT+

                        //ADC_B
                        SN1_AE_Integral+=ads_B.readADC_SingleEnded(1); // NO2   Aux 1
                        SN2_AE_Integral+=ads_B.readADC_SingleEnded(2); // O3/NO Aux 2
                        SN3_AE_Integral+=ads_B.readADC_SingleEnded(3); // NO    Aux 3

                        //Messung();
                        //Serial.println(String(SN1_value));
                        battery_solar_Integral += (analogRead(ADC_BAT_SOLAR) * conversion_factor *2) ;
                        battery_fona_Integral +=  (analogRead(ADC_BAT_FONA) * conversion_factor * 2);
                        UpdateDisplay();
                        UpdateAccelerometerReadings(7);
                }

                last_geohash = hasher_fine.encode(GPS.latitudeDegrees,GPS.longitudeDegrees);
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

        Geohash_fine   = hasher_fine.encode(GPS.latitudeDegrees,GPS.longitudeDegrees);
        Geohash_normal = hasher_normal.encode(GPS.latitudeDegrees,GPS.longitudeDegrees);
        Geohash_coarse = hasher_coarse.encode(GPS.latitudeDegrees,GPS.longitudeDegrees);

        battery_solar = battery_solar_Integral / measurement_counter;
        battery_fona =  battery_fona_Integral / measurement_counter;

        setTime(GPS.hour,GPS.minute,GPS.seconds,GPS.day,GPS.month,GPS.year);

        //Serial.print("NOW:"); Serial.println (now());

        //generateUploadString();
        //data_upload += Uploadstring.length();

        Serial.println("DONE");
        linesinfile += 1;

        Serial.println(String(SN1_value));
        Serial.println(String(SN1_AE_value));
        Serial.println("Measurements taken: " + String(measurement_counter));
        Serial.println("Lines in file: " + String(linesinfile));

        //Serial.println(Uploadstring);

        //writeLineToFile(Uploadstring,"DATA.TXT");


        writeLineToFile(generateJSONString(),"LOGFILE.TXT");
        writeLineToFile(generateJSONString(),"DATA.TXT");


        Serial.print("\nTime: ");
        Serial.print(GPS.hour,DEC); Serial.print(':');
        Serial.print(GPS.minute,DEC); Serial.print(':');
        Serial.print(GPS.seconds,DEC); Serial.print('.');
        Serial.println(GPS.milliseconds);
        Serial.print("Date: ");
        Serial.print(GPS.day,DEC); Serial.print('/');
        Serial.print(GPS.month,DEC); Serial.print("/20");
        Serial.println(GPS.year,DEC);
        Serial.print("Fix: "); Serial.print((int)GPS.fix);
        Serial.print(" quality: "); Serial.println((int)GPS.fixquality);

        if (GPS.fix) {
                Serial.print("Location: ");
                Serial.print(GPS.latitude,4); Serial.print(GPS.lat);
                Serial.print(", ");
                Serial.print(GPS.longitude,4); Serial.println(GPS.lon);
                Serial.print("Location (in degrees, works with Google Maps): ");
                Serial.print(GPS.latitudeDegrees,4);
                Serial.print(", ");
                Serial.println(GPS.longitudeDegrees,4);

                Serial.print("Speed (knots): "); Serial.println(GPS.speed);
                Serial.print("Angle: "); Serial.println(GPS.angle);
                Serial.print("Altitude: "); Serial.println(GPS.altitude);
                Serial.print("Satellites: "); Serial.println((int)GPS.satellites);

                setTime(GPS.hour,GPS.minute,GPS.seconds,GPS.day,GPS.month,GPS.year);

                Serial.print("NOW:"); Serial.println (now());
        }

        if (digitalRead(ACC_MOVEMENT_PIN) == LOW) {
                state = SEND_DATA;
        }

        if (CheckBattery() == false) {
                state = SEND_DATA;
        }

}

void STATE_SEND_DATA(){
  display.clearDisplay();
  display.setCursor(0,0);
  Serial.println("Sending sensordata...");
  display.println("SENSORDATA");
  display.display();

  ModemTurnOn();
  delay(300);

  if (EstablishConnection("130.149.67.198","4000")==0) { //"heimdall.dedyn.io","1900" "130.149.67.141","4001"
          state = TRANS_SLEEP;
          Serial.println("going to sleep");
          ModemTurnOff();
          return;
  }


  String SendBuffer = "";
  // open the file for reading:
  myFile = SD.open("DATA.TXT");

  Serial.println("Reading File: Data.");
  if (myFile) {
    // read from the file until there's nothing in it:
    while (myFile.available()) {
      char c = myFile.read();
      SendBuffer += (c);

        if (c == '\n') {

        AdvancedParser("AT+CHTTPSSEND="+ String(SendBuffer.length()),"OK","","", 10);
        Fona3G.print(SendBuffer);
        Serial.print(SendBuffer);
        //Parser("AT+CHTTPSSEND", 500);
        AdvancedParser("AT+CHTTPSSEND", "OK", "", "", 500);
        SendBuffer = "";

      }

    }
  }
    // close the file:
    myFile.close();
    SD.remove("DATA.txt");

  delay(300);
  CloseSession();
  state = SLEEP;
}


void STATE_SEND_DATA_old(){

        int divide_flag= 0;

        PumpOff();

        Serial.println("SENDING DATA...");

        ModemTurnOn();

        if (EstablishConnection("87.123.157.161","1900")==0) {
                state = TRANS_SLEEP;
                Serial.println("going to sleep");

                return;
        }
        Serial.println("Connection Established");
        do {
                Serial.println("Dividing");
                divide_flag=divideFile(20);

                if(SendSequence("SEND.TXT")!=0) {
                        SD.remove("DATA.txt");
                        restToNewDataFile();
                }
                else{
                        break;
                }

                /* code */
        } while(divide_flag != 0);

        CloseSession();
        ModemTurnOff();

        state = TRANS_SLEEP;


        //if (CheckBattery() == false) {
        //        state =  CHARGE;
        //}
}

void STATE_TELEMETRY(){

display.clearDisplay();
display.setCursor(0,0);
display.println("TELEMETRY");
display.display();

Serial.println("SENDING TELEMETRY...");

ModemTurnOn();

delay(300);

if (EstablishConnection("130.149.67.198","4000")==0) { //"heimdall.dedyn.io","1900" "130.149.67.141","4001"
        state = TRANS_SLEEP;
        Serial.println("going to sleep");

        return;
}

  //TELEMETRY CODE GOES HERE
  // alt String TelemetryString = "{\"type\": \"telemetry\", \"Solar\" :" + String(battery_solar)+ ", \"Fona\" : " + String(battery_fona)+ ", \"Temp\" : " + String(bme.temp()) + " }\n";
  String TelemetryString = "";

  DynamicJsonDocument tel(512);

  JsonObject fields=tel.createNestedObject();

  fields["type"] = "telemetry";
  fields["bat_solar"]= round_to_dp(battery_solar,2);
  fields["bat_fona"]= round_to_dp(battery_fona,2);
  fields["bme_t"]= round_to_dp(bme.temp(),2);
  fields["BOD"]= digitalRead(BOD_PIN);

  JsonObject tags=tel.createNestedObject();

  tags["BOD"]= digitalRead(BOD_PIN);

  serializeJson(tel, TelemetryString);
  TelemetryString += '\n';

  Parser("AT+CHTTPSSEND="+ String(TelemetryString.length()), 500);
  Fona3G.print(TelemetryString);
  Parser("AT+CHTTPSSEND", 500);
  delay(300);
  CloseSession();
  state = SLEEP;
}


void STATE_TRANSIT_SLEEP(){

        GpsOff();
        PumpOff();
        ModemTurnOff();

        state = SLEEP;

}


void STATE_SLEEP(){

        GpsOff();
        PumpOff();
        ModemTurnOff();

        digitalWrite(ACC_SHUTDOWN_PIN, HIGH);

}

void setup(){

// EXTERNAL_ACC_PINS
pinMode(ACC_MOVEMENT_PIN,INPUT);
pinMode(ACC_SHUTDOWN_PIN,OUTPUT);
digitalWrite(ACC_SHUTDOWN_PIN,LOW);

// Set 3.3V as AREF
analogReference(EXTERNAL);



// SETUP SERIAL
        Serial.begin(115200);
        Serial.println("Setup...");
        Fona3G.begin(115200);
        Wire.begin();
Serial.println("Setup...2");
        // SETUP DISPLAY

        display.begin(SSD1306_SWITCHCAPVCC,0x3C);
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.setCursor(0,0);
        display.clearDisplay();

        display.println("Boot...");
        display.display();
        delay(500);

// SETUP SD-Card-Reader

        display.print("SD...");
        display.display();
        delay(500);

        Serial.print("Initializing SD card..."); //SD Setup
        if (!SD.begin(53)) {
                Serial.println("failed!");
                display.println("failed!");
                display.display();
                //while (1);
        }
        Serial.println("initialization done.");

        display.println("OK!");
        display.display();


        display.print("BME280...");
        display.display();
        delay(500);


        if(!bme.begin())
        {
                Serial.println("Could not find BME280 sensor!");
                display.println("failed!");
                display.display();
                delay(500);

        }
        
        display.println("OK!");
        display.display();

        display.println("I/O-Pins...");
        display.display();
        delay(500);

// SETUP I/O-PINS
        pinMode(RST_FONA,OUTPUT); // Fona3G ResetPin
        pinMode(KEY_FONA,OUTPUT); // Fona3G KeyPin
        pinMode(PS_FONA,INPUT); // Fona3G PowerStatePin

        pinMode(GPS_ENABLE,OUTPUT); // GPS_EnablePin
        pinMode(GPS_FIX,INPUT); // GPS_FixOutputPin
        pinMode(LED_BUILTIN,OUTPUT);  // Debug LED

        digitalWrite(RST_FONA,HIGH);
        digitalWrite(KEY_FONA,HIGH);
        digitalWrite(GPS_ENABLE,HIGH);
        
        //Buttons&Switches
        pinMode(MODE_SWITCH,INPUT_PULLUP);
        pinMode(BTN_T1,INPUT_PULLUP);
        pinMode(BTN_T2,INPUT_PULLUP);
        pinMode(BTN_SEND,INPUT_PULLUP);

        //Pump control
        pinMode(PUMP,OUTPUT);
        digitalWrite(PUMP,LOW);

        display.println("Setup ADCs...");
        display.display();
        delay(500);

// SETUP ADC's
        ads_A.setGain(GAIN_TWO);  // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
        ads_B.setGain(GAIN_TWO);

        ads_A.begin();
        ads_B.begin();

// Setup GPS

        display.println("Start GPS...");
        display.display();
        delay(500);
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

        delay(500);
// Ask for firmware version
        UltGps.println(PMTK_Q_RELEASE);

        display.println("Done...");
        display.display();
        delay(500);

        state_station = STATION_INIT;
        state = INIT;

        if (digitalRead(MODE_SWITCH)==HIGH){
          station_mode= true;

        } else{
          station_mode= false;
        }


}

void state_machine_mobile(){
        switch(state){

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

        case CHARGE:
                STATE_CHARGE();
                break;

        case SLEEP:
                STATE_SLEEP();
                break;

        case TRANS_SLEEP:
                STATE_TRANSIT_SLEEP();
                break;
        case TELEMETRY:
                STATE_TELEMETRY();
                break;
        }
}

void STATE_STATION_INIT(){
  Serial.println("Station Init..");
  delay(1000);
  Serial.println("Switching to WAIT_GPS...");
  TripID="station";

  state_station = STATION_WHAIT_GPS;
  GpsOn();
}

void STATE_STATION_WHAIT_GPS(){

        if (digitalRead(BTN_T1) == LOW) {
                  Serial.println("Switching to extract state!");
                  state_station = STATION_EXTRACT;
                  }         

  Serial.println("WHAIT_GPS...");
  Serial.println( "Now: " + String(now()));
  delay(2000);
  Serial.println( "Now: " + String(now()));
  //PumpOff();
  Serial.println("Waiting for GPS-fix...");
  if (GPS.newNMEAreceived()) {
          GPS.parse(GPS.lastNMEA());

  }
  if (GPS.fix) {
          setTime(GPS.hour,GPS.minute,GPS.seconds,GPS.day,GPS.month,GPS.year);
          new_time_str = TimeString();
          Serial.println("Got GPS-fix! Switching to MEASURING state!");
          state_station = STATION_MEASURING;
          if (TripID == "notset"){
            TripID = String(GPS.year) + "-" +String(GPS.month) + "-" + String(GPS.day) +"/" + String(GPS.hour)+":" + String(GPS.minute);
            }
  }



}

void STATE_STATION_MEASURING(){

// Variables
float SN1_Integral = 0;
float SN2_Integral = 0;
float SN3_Integral = 0;
float TEMP_Integral = 0;

float SN1_AE_Integral = 0;
float SN2_AE_Integral = 0;
float SN3_AE_Integral = 0;

float battery_solar_Integral = 0;
float battery_fona_Integral = 0;
int measurement_counter = 0;

// CODE
if (GPS.fix) { // If GPS available start a measurement.
  delay(50);
  GPS.parse(GPS.lastNMEA());
  Serial.println("GPS-LOCK");
  station_measurement_counter += 1;
  Serial.println("Stationloop: " + String(station_measurement_counter));

  //PASTED code
  //Check for GPS-fix

  Serial.println("Start Measurement...");
  station_measurement_timer = millis();

  //ADC Warmup
  for (size_t w = 0; w < 5; w++) {
  ads_A.readADC_SingleEnded(1);
  ads_A.readADC_SingleEnded(2);
  ads_A.readADC_SingleEnded(3);
  ads_A.readADC_SingleEnded(4);
  ads_B.readADC_SingleEnded(1);
  ads_B.readADC_SingleEnded(2);
  ads_B.readADC_SingleEnded(3);
  ads_B.readADC_SingleEnded(4);
  }

  while (millis() - station_measurement_timer < station_measurement_time) {

                  // Abrufen der Analogeinganswerte am ADS1115
                  // Abrufen der an den Pins anliegenden Werte
                  measurement_counter += 1;

                  //ADC_A
                  SN1_Integral+=ads_A.readADC_SingleEnded(1); // NO2   Work 1
                  SN2_Integral+=ads_A.readADC_SingleEnded(2); // O3/NO Work 2
                  SN3_Integral+=ads_A.readADC_SingleEnded(3); // NO    Work 3
                  TEMP_Integral  +=ads_A.readADC_SingleEnded(0);// PT+

                  //ADC_B
                  SN1_AE_Integral+=ads_B.readADC_SingleEnded(1); // NO2   Aux 1
                  SN2_AE_Integral+=ads_B.readADC_SingleEnded(2); // O3/NO Aux 2
                  SN3_AE_Integral+=ads_B.readADC_SingleEnded(3); // NO    Aux 3

                  //Messung();
                  //Serial.println(String(SN1_value));
                  battery_solar_Integral += (analogRead(ADC_BAT_SOLAR) * conversion_factor *2) ;
                  battery_fona_Integral +=  (analogRead(ADC_BAT_FONA) * conversion_factor * 2);
                  UpdateDisplaySTN();
                  UpdateAccelerometerReadings(7);

                  if (digitalRead(BTN_T1) == LOW) {
                  Serial.println("Switching to extract state!");
                  state_station = STATION_EXTRACT;
                  }
                  if (digitalRead(BTN_SEND) == LOW) {
                  Serial.println("Switching to sending state!");
                  state_station = STATION_SEND_DATA;
                  station_measurement_counter = 0;
                  }             
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

  Geohash_fine   = hasher_fine.encode(GPS.latitudeDegrees,GPS.longitudeDegrees);
  Geohash_normal = hasher_normal.encode(GPS.latitudeDegrees,GPS.longitudeDegrees);
  Geohash_coarse = hasher_coarse.encode(GPS.latitudeDegrees,GPS.longitudeDegrees);

  battery_solar = battery_solar_Integral / measurement_counter;
  battery_fona =  battery_fona_Integral / measurement_counter;

  

  
  //new_gps_time = now();
  
  old_time_str = new_time_str;
  new_time_str = TimeString();

  Serial.println("OLD_STR: " + old_time_str);
  Serial.println("NEW_STR: " + new_time_str);

  if (new_time_str != old_time_str) {
        last_corr_time = millis();
        setTime(GPS.hour,GPS.minute,GPS.seconds,GPS.day,GPS.month,GPS.year);
        Serial.println("Clock updated!!!");
  }

  Serial.println( "Now: " + String(now()));
  //Serial.println( "old_gpstime: " + String(old_gps_time));
  //Serial.println( "new_gpstime: " + String(new_gps_time));
  //Serial.println( "diff: " + String(new_gps_time - old_gps_time));
  Serial.println("Last_Corr: " + String(millis()-last_corr_time));
  Serial.println( "Now: " + String(now()));
  //ffrf


  //Serial.print("NOW:"); Serial.println (now());

  //generateUploadString();
  //data_upload += Uploadstring.length();

  Serial.println("DONE");
  
  Serial.println(String(SN1_value));
  Serial.println(String(SN1_AE_value));
  Serial.println("Measurements taken: " + String(measurement_counter));


  //Serial.println(Uploadstring);

  //writeLineToFile(Uploadstring,"DATA.TXT");


  writeLineToFile(generateJSONStringSTN(),"STN_LOG.TXT");
  writeLineToFile(generateJSONStringSTN(),"STN_DATA.TXT");

          //DEBUG
/*
        Serial.println("Reading STN_DATA.TXT :");
        if(SD.exists("STN_DATA.TXT")){
        dataFile = SD.open("STN_DATA.TXT", FILE_READ);
        if(dataFile){
        while(dataFile.available()) {
        Serial.write(dataFile.read());
        }
        dataFile.close();
        }
        
        }

        //DEBUG END
   
*/

  if (station_measurement_counter % 10 == 0) { // Normal 40 
  Serial.println("5 min, writing tel..");
  writeLineToFile(generateJSONStringSTN(),"STN_TEL.TXT");
  }
  //PASTED CODE END


  Serial.println();


  setTime(GPS.hour,GPS.minute,GPS.seconds,GPS.day,GPS.month,GPS.year);

  Serial.print("\nMeasurement: ");
  Serial.print(String(station_measurement_counter));
  Serial.print("\nTime: ");
  Serial.print(GPS.hour,DEC); Serial.print(':');
  Serial.print(GPS.minute,DEC); Serial.print(':');
  Serial.print(GPS.seconds,DEC); Serial.print('.');
  Serial.println(GPS.milliseconds);
  Serial.print("Date: ");
  Serial.print(GPS.day,DEC); Serial.print('/');
  Serial.print(GPS.month,DEC); Serial.print("/20");
  Serial.println(GPS.year,DEC);
  Serial.print("Fix: "); Serial.print((int)GPS.fix);
  Serial.print(" quality: "); Serial.println((int)GPS.fixquality);


} else{
  Serial.println("Lost GPS-LOCK...");
  Serial.println("Switching to WHAIT_GPS state!");
  state_station = STATION_WHAIT_GPS;
}

if (station_measurement_counter >= station_max_loops){ // 120 pro stunde (Messung alle 30 Sekunden)

  station_measurement_counter = 0;
  state_station = STATION_SEND_DATA;
}

}

void STATE_STATION_SEND_DATA(){
  Serial.println("Sending...");
  delay(1000);


  //PASTED
  display.clearDisplay();
  display.setCursor(0,0);
  Serial.println("Sending sensordata...");
  display.println("STATIONDATA");
  display.display();

  ModemTurnOn();
  delay(300);

  if (EstablishConnection("130.149.67.198","4000")==0) { //"heimdall.dedyn.io","1900" "130.149.67.141","4001"
          state_station = STATION_WHAIT_GPS;
          ModemTurnOff();
          Serial.println("Sending failed...");
          Serial.println("Returning to Measuring State.");

          return;
  }


  String SendBuffer = "";
  // open the file for reading:
  myFile = SD.open("STN_TEL.TXT");

  Serial.println("Reading File: Data.");
  if (myFile) {
    // read from the file until there's nothing in it:
    while (myFile.available()) {
      char c = myFile.read();
      SendBuffer += (c);

        if (c == '\n') {

        AdvancedParser("AT+CHTTPSSEND="+ String(SendBuffer.length()),"OK","","", 10);
        Fona3G.print(SendBuffer);
        Serial.print(SendBuffer);
        //Parser("AT+CHTTPSSEND", 500);
        AdvancedParser("AT+CHTTPSSEND", "OK", "", "", 500);
        SendBuffer = "";

      }

    }
  }
    // close the file:
    myFile.close();
    SD.remove("STN_TEL.TXT");

  delay(300);
  CloseSession();
  ModemTurnOff();

    //PASTED
  Serial.println("Switching to WHAIT_GPS state!");
  state_station = STATION_WHAIT_GPS;
}

void STATE_STATION_EXTRACT(){

        delay(50);
        if (digitalRead(BTN_T2) == LOW) {
        Serial.println("Switching to WHAIT_GPS state!");
        state_station = STATION_WHAIT_GPS;
        }
}

void state_machine_station(){
        switch(state_station)
        {
        case STATION_INIT:
                STATE_STATION_INIT();
                break;

        case STATION_MEASURING:
                STATE_STATION_MEASURING();
                break;

        case STATION_WHAIT_GPS:
                STATE_STATION_WHAIT_GPS();
                break;

        case STATION_SEND_DATA:
                STATE_STATION_SEND_DATA();
                break;

        case STATION_EXTRACT:
                STATE_STATION_EXTRACT();
                break;
        }
}



void loop() {

        if (station_mode == false){
        state_machine_mobile();
        UpdateDisplay();

      } else {
        state_machine_station();
        UpdateDisplaySTN();
      }

        UpdateBatteryVoltageRaeadings();
        //acc_flag = UpdateAccelerometerReadings(11);
        
}
