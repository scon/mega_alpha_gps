
void eraseConfig() {
  // Reset EEPROM bytes to '0' for the length of the data structure
  
  for (int i = cfgStart ; i < sizeof(cfg) ; i++) {
    EEPROM.write(i, 0);
  }
  delay(200);
  //EEPROM.commit();
  
}

void saveConfig() {
  // Save configuration from RAM into EEPROM
  
  EEPROM.put( cfgStart, cfg );
  delay(200);
  //EEPROM.commit();                      // Only needed for ESP8266 to get data written
                    // Free RAM copy of structure
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
  //Serial.print("(2)Wifi_name: "); Serial.println(cfg.wifi_name);
  //Serial.print("(3)Wifi_pw: "); Serial.println(cfg.wifi_pw);
  //Serial.print("(4)Influx_enabled: "); Serial.println(String(cfg.influx_en));
  //Serial.print("(5)Influx_port: "); Serial.println(String(cfg.influx_port));
  //Serial.print("(6)Influx_ip: "); Serial.println(cfg.influx_ip);
  //Serial.print("(7)Meas. frequency: ") ; Serial.println(String(cfg.freq));
  //Serial.print("(8)Serial_enable: ") ; Serial.println(String(cfg.ser_en));
  //Serial.print("(9)Serial format [(0)CSV/(1)JSON]: ") ; Serial.println(String(cfg.ser_f));
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
      strncpy(cfg.wifi_name, read_command().c_str() , sizeof(cfg.wifi_name) );
      saveConfig();
    }
    else if (input == "3") {
      Serial.println("Enter new wifi password:");
      strncpy( cfg.wifi_pw, read_command().c_str() , sizeof(cfg.wifi_pw) );
      saveConfig();
    }
    else if (input == "4") {
      Serial.println("Enable(1)/Disable(0) InfluxDB:");
      cfg.influx_en = (read_command().toInt());
      saveConfig();
    }

    else if (input == "5") {
      Serial.println("InfluxDB Port:");
      cfg.influx_port = (read_command().toInt());
      saveConfig();
    }

    else if (input == "6") {
      Serial.println("Enter Influx IP:");
      strncpy(cfg.influx_ip, read_command().c_str() , sizeof(cfg.wifi_name) );
      saveConfig();
    }

    else if (input == "7") {
      Serial.println("Set Measurement freq. (ms):");
      cfg.freq = (read_command().toInt());
      saveConfig();
    }

    else if (input == "8") {
      Serial.println("Enable(1)/Disable(0) serial output:");
      cfg.ser_en = (read_command().toInt());
      saveConfig();
    }

    else if (input == "9") {

      Serial.println("Select serial output format [(0)CSV/(1)JSON]:");

      cfg.ser_f = (read_command().toInt());
      saveConfig();
    }


  }
}


