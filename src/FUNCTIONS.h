bool CheckBattery(){

        if (battery_solar < battery_threshold) {
                return false;
        }
        else{
                return true;
        }

}

bool CheckAccelerometerTimer(){

}

void UpdateBatteryVoltageRaeadings(){
  float avg_solar, avg_fona =0;


        //battery_solar = (analogRead(6));
        //battery_fona  =  (analogRead(4));

        avg_solar = ((analogRead(ADC_BAT_SOLAR) * conversion_factor *2) );
        avg_fona =  ((analogRead(ADC_BAT_FONA) * conversion_factor *2) * adc_bat_fona_coeff );
        battery_solar = (battery_solar + avg_solar)/2;
        battery_fona = (battery_fona + avg_fona)/2;

}

bool UpdateAccelerometerReadings(int acc_threshold){
        int acc_X_diff,acc_Y_diff,acc_Z_diff;

        acc_X_last = acc_X;
        acc_Y_last = acc_Y;
        acc_Z_last = acc_Z;

        acc_X = (analogRead(12));
        acc_Y = (analogRead(10));
        acc_Z = (analogRead(8));

        acc_X_diff = acc_X - acc_X_last;
        acc_Y_diff = acc_Y - acc_Y_last;
        acc_Z_diff = acc_Z - acc_Z_last;

        acc_X_abs = abs(acc_X_diff);
        acc_Y_abs = abs(acc_Y_diff);
        acc_Z_abs = abs(acc_Z_diff);
        acc_vektor = 0;

        acc_vektor = sqrt(sq(acc_X_abs)+sq(acc_Y_abs)+sq(acc_Y_abs));

        if (acc_vektor > acc_threshold)  {
                acc_timer = millis();
                return true;
        }
        else{
                return false;
        }

        }

void PumpOn(){
        digitalWrite(PUMP,HIGH);
}
void PumpOff(){
        digitalWrite(PUMP,LOW);
}

void GpsOn(){
        digitalWrite(GPS_ENABLE,HIGH);
}
void GpsOff(){
        digitalWrite(GPS_ENABLE,LOW);
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

void writeLineToFile(String line, String WLF_filename){
        myFile = SD.open(WLF_filename, FILE_WRITE);

// if the file opened okay, write to it:
        if (myFile) {
                Serial.print("Writing to data.txt...");
                myFile.println(line);
// close the file:
                myFile.close();
                Serial.println("done.");
        } else {
// if the file didn't open, print an error:
                Serial.println("error opening" + WLF_filename);
        }
}
//fg
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

                //BME280
                "BME_h=" + String(bme.hum()) + "," +
                "BME_T=" + String(bme.temp()) + "," +
                "BME_P=" + String(bme.pres()) + "," +



                //Position & Speed
                "lng=" + String(GPS.longitudeDegrees, 4) + "," +
                "lat=" + String(GPS.latitudeDegrees, 4) + "," +
                "acc_v=" + String(acc_vektor, 4) + "," +
                "speed=" + (GPS.speed * 1.852) +  // Knots to km/h

                " " +                                    // Leerzeichen trennt Fields und Timestamp

                String(now());

}

float round_to_dp( float in_value, int decimal_place )
{
float multiplier = powf( 10.0f, decimal_place );
in_value = roundf( in_value * multiplier ) / multiplier;
return in_value;
}

String generateJSONString(){
  String OutputString = "";

DynamicJsonDocument doc(1024);

JsonObject fields=doc.createNestedObject();

fields["type"] = "data";
fields[SN1]=round_to_dp(SN1_value, 2);
fields[SN2]=round_to_dp(SN2_value, 2);
fields[SN3]=round_to_dp(SN3_value, 2);
fields[SN1_AE]=round_to_dp(SN1_AE_value, 2);
fields[SN2_AE]=round_to_dp(SN2_AE_value, 2);
fields[SN3_AE]=round_to_dp(SN3_AE_value, 2);
fields[TEMP]=round_to_dp(Temp_value, 2);

fields["bat_solar"]=round_to_dp(battery_solar,2);
fields["bat_fona"]=round_to_dp(battery_fona,2);
fields["data_upload"]=data_upload;
fields["bme_h"]=round_to_dp(bme.hum(),2);
fields["bme_t"]=round_to_dp(bme.temp(),2);
fields["bme_P"]=round_to_dp(bme.pres(),2);
fields["lng"]=String(GPS.longitudeDegrees, 4);
fields["lat"]=String(GPS.latitudeDegrees, 4);
fields["acc_v"]=round_to_dp(acc_vektor, 2);
fields["speed"]=round_to_dp((GPS.speed * 1.852),2); // Knots to km/h
fields["time"]= String(now());
fields["BOD"]= digitalRead(BOD_PIN);
JsonObject tags=doc.createNestedObject();

tags["hour"] = GPS.hour;
tags["minute"]= GPS.minute;
tags["geohash"]= Geohash_fine;
tags["bme_t"]=round_to_dp(bme.temp(),2);
tags["BOD"]= digitalRead(BOD_PIN);
tags["TripID"] = TripID;
tags["STN_ID"] = String(cfg.stn_id);  //STN_ID;

serializeJson(doc, OutputString);
Serial.println(OutputString);
return OutputString;

}

String generateJSONStringSTN(){
  String OutputString = "";

DynamicJsonDocument doc(1024);

JsonObject fields=doc.createNestedObject();

fields["type"] = "stn";
fields[SN1]=round_to_dp(SN1_value, 2);
fields[SN2]=round_to_dp(SN2_value, 2);
fields[SN3]=round_to_dp(SN3_value, 2);
fields[SN1_AE]=round_to_dp(SN1_AE_value, 2);
fields[SN2_AE]=round_to_dp(SN2_AE_value, 2);
fields[SN3_AE]=round_to_dp(SN3_AE_value, 2);
fields[TEMP]=round_to_dp(Temp_value, 2);

fields["bat_solar"]=round_to_dp(battery_solar,2);
fields["bat_fona"]=round_to_dp(battery_fona,2);
fields["bme_h"]=round_to_dp(bme.hum(),2);
fields["bme_t"]=round_to_dp(bme.temp(),2);
fields["bme_P"]=round_to_dp(bme.pres(),2);
fields["acc_v"]=round_to_dp(acc_vektor, 2);
fields["speed"]=round_to_dp((GPS.speed * 1.852),2); // Knots to km/h
fields["time"]= String(now());
fields["BOD"]= digitalRead(BOD_PIN);
JsonObject tags=doc.createNestedObject();

tags["hour"] = GPS.hour;
tags["minute"]= GPS.minute;
tags["bme_t"]=round_to_dp(bme.temp(),2);
tags["BOD"]= digitalRead(BOD_PIN);
tags["TripID"] = TripID;
tags["STN_ID"] = String(cfg.stn_id); //STN_ID;


serializeJson(doc, OutputString);
Serial.println(OutputString);
return OutputString;

}


void UpdateDisplay(){

        display.clearDisplay();
        display.setCursor(0,0);

        display.print("State: ");
        display.println(String(state));

        display.print("V_Bat: ");
        display.println(String(battery_solar));

        display.print("V_Fona: ");
        display.println(String(battery_fona));

        display.print("Temp: ");
        display.println(String(bme.temp()));

        display.print("LiF: ");
        display.println(String(linesinfile) + "/" + String(max_linesinfile));

        display.print("ACC_T: ");
        display.println(String(millis() -acc_timer) + "/" + String(acc_sleep_timeout));

        //display.print("X:"); display.print(String(acc_X_abs));
        //display.print("Y:"); display.print(String(acc_Y_abs));
        //display.print("Z:"); display.println(String(acc_Z_abs));
        //display.print("V:"); display.println(String(acc_vektor));
        display.print("M:"); display.println(digitalRead(ACC_MOVEMENT_PIN));
        display.print("Trip:"); display.println(TripID);



        display.display();

}

void UpdateDisplaySTN(){

        display.clearDisplay();
        display.setCursor(0,0);

        display.print("STN-State: ");
        display.println(String(state_station));

        display.print("V_Bat: ");
        display.println(String(battery_solar));

        display.print("V_Fona: ");
        display.println(String(battery_fona));

        display.print("Temp: ");
        display.println(String(bme.temp()));

        display.print("NO2: ");
        display.println(String(ads_A.readADC_SingleEnded(1) * getUmrechnungsfaktor()) + "/" + String(SN1_value));

        display.print("Gain: ");
        display.println(String(ads_A.getGain()));

        display.print("STN-Loop:");
        display.println(String(station_measurement_counter) + "/" + String(station_max_loops));
        
        display.print("Trip:"); display.println(TripID);

        display.display();

}

String TimeString(){
String out_str = String(GPS.hour) + String(GPS.minute) +String(GPS.seconds) +String(GPS.day) +String(GPS.month) +String(GPS.year);
return out_str;
}

void UpdateClock(){

if (GPS.fix) {

  old_time_str = new_time_str;
  new_time_str = TimeString();

  Serial.println("OLD_STR: " + old_time_str);
  Serial.println("NEW_STR: " + new_time_str);

  if(new_time_str != old_time_str) {
        //last_corr_time = millis();
        setTime(GPS.hour,GPS.minute,GPS.seconds,GPS.day,GPS.month,GPS.year);
        Serial.println("*** Clock updated ***");
        }
  else{
        Serial.println("*!* Clock update failed *!*");
}
}
}