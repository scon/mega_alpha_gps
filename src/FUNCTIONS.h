bool CheckBattery(){

        if (battery_solar < battery_threshold) {
                return false;
        }
        else{
                return true;
        }

}

bool CheckAccelerometerTimer(){
  if (digitalRead(9)==LOW){
    acc_timer = millis();
  }

        if (millis() - acc_timer > acc_sleep_timeout) {
                return false;
        }
        else{
                return true;
        }

}

void UpdateBatteryVoltageRaeadings(){
        battery_solar = (analogRead(4) * conversion_factor * 1.5);
        battery_fona  =  (analogRead(6) * conversion_factor * 1.5);
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

void UpdateDisplay(){

        display.clearDisplay();
        display.setCursor(0,0);

        display.print("State: ");
        display.println(String(state));

        display.print("V_Bat: ");
        display.println(String(battery_solar));

        display.print("V_FG3: ");
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
        display.print("V:"); display.println(String(acc_vektor));


        display.display();

}
