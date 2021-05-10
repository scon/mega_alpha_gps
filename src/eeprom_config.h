
void saveConfig() {
  // Save configuration from RAM into EEPROM
  
  EEPROM.put( cfgStart, cfg );
  delay(200);
  //EEPROM.commit();                      // Only needed for ESP8266 to get data written
                    // Free RAM copy of structure
}


void eraseConfig() {

strncpy(cfg.stn_id, "STN_0" , sizeof(cfg.stn_id) );
strncpy(cfg.sns_id, "SNS_0" , sizeof(cfg.sns_id) );
strncpy(cfg.sim_pin, "4804" , sizeof(cfg.sim_pin) );
strncpy(cfg.server_ip, "130.149.67.198" , sizeof(cfg.server_ip) );
strncpy(cfg.server_port, "4000" , sizeof(cfg.server_port) );

 /*cfg.stn_id = "STN_0";
 cfg.sns_id = "SNS_0";
 cfg.sim_pin = "4804";
 cfg.server_ip = "130.149.67.198";
 cfg.server_port = "4000";
 */

 saveConfig();
}

void loadConfig() {
  // Loads configuration from EEPROM into RAM
  
  EEPROM.get( cfgStart, cfg );
  
}


String read_command() {
  String command = "";
  bool msg_end = false;

  while (msg_end == false)
  {
    yield();
    if (Serial.available() > 0) {
      //char recieved = Serial.read();
      command = Serial.readStringUntil('\n');
      msg_end = true;
    }
  }

  Serial.print("Arduino Received: ");
  Serial.println(command);
  command.trim();
  return command;
}


void print_cfg() {
  Serial.println();
  Serial.println("Current configuration:");
  Serial.print("(1)STN_ID: "); Serial.println(cfg.stn_id);
  Serial.print("(2)Sensor_ID: "); Serial.println(cfg.sns_id);
  Serial.print("(3)SIM pin: "); Serial.println(cfg.sim_pin);
  Serial.print("(4)Server IP: "); Serial.println(cfg.server_ip);
  Serial.print("(5)Server PORT: "); Serial.println(cfg.server_port);
  Serial.print("(6)SIM pin(0/1): "); Serial.println(cfg.sim_pin_en);
  Serial.print("(7)APN: ") ; Serial.println(String(cfg.apn));
  Serial.print("(8)APN_USR: ") ; Serial.println(String(cfg.apn_usr));
  Serial.print("(9)APN_PW: ") ; Serial.println(String(cfg.apn_pw));
  Serial.println("(r)reset: "); 

  Serial.println();
}

void start_menue() {

  while (input != "exit") {
    //loadConfig();
    print_cfg();
    Serial.println("Select Parameter to edit or enter 'exit' to leave:");
    input = read_command();

    if  (input == "1") {
      Serial.println("Enter new STN_ID:");
      strncpy( cfg.stn_id, read_command().c_str() , sizeof(cfg.stn_id) );
      saveConfig();
    }
    else if (input == "2") {
      Serial.println("Enter new SENSOR_ID:");
      strncpy(cfg.sns_id, read_command().c_str() , sizeof(cfg.sns_id) );
      saveConfig();
    }
    else if (input == "3") {
      Serial.println("Enter new SIM Pin:");
      strncpy( cfg.sim_pin, read_command().c_str() , sizeof(cfg.sim_pin) );
      saveConfig();
    }
    else if (input == "4") {
      Serial.println("Enter new server IP:");
      strncpy( cfg.server_ip, read_command().c_str() , sizeof(cfg.server_ip) );
      saveConfig();
    }

    else if (input == "5") {
    Serial.println("Enter new server PORT:");
      strncpy( cfg.server_port, read_command().c_str() , sizeof(cfg.server_port) );
      saveConfig();
    }

    else if (input == "6") {
      Serial.println("Enter SIM Pin (0/1):");
      
      cfg.sim_pin_en = (read_command().toInt());
      saveConfig();
    }

    else if (input == "7") {
      Serial.println("Set GSM APN:");
      //cfg.freq = (read_command().toInt());
      strncpy(cfg.apn, read_command().c_str() , sizeof(cfg.apn) );
      saveConfig();
    }

    else if (input == "8") {
      Serial.println("Set APN Username:");
      strncpy(cfg.apn_usr, read_command().c_str() , sizeof(cfg.apn_usr) );
      saveConfig();
    }

    else if (input == "9") {

         Serial.println("Set APN Password:");
      strncpy(cfg.apn_pw, read_command().c_str() , sizeof(cfg.apn_pw) );
            saveConfig();
    }
    else if (input == "r") {

      Serial.println("resetting all values");

      eraseConfig();
      
    }

  }
}

void manageEEPROMcfg(){
                //EEPROMCFG
        

        input_timer = millis();

        print_cfg();

        Serial.println("Hit 'Return' to start configuration menue!");
        Serial.println("Measurement starts in: " + String(10000) + " ms.");

        while (millis() - input_timer < setup_delay)
        {
                if (Serial.available() > 0)
                {
                        char recieved = Serial.read();
                        if (recieved == '\n')
                        {
                                Serial.println("Start_Menue");
                                start_menue();
                        }
                }
        }

        loadConfig();
}
