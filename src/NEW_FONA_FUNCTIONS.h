// Function Constructors
//int sendWithCheck(){};

// END

int AdvancedParser(String command, String response_a, String response_b, String response_c, int ser_timeout) {

  String Response = "";

  while (Fona3G.available() > 0) {
    Serial.write(Fona3G.read()); // Serial Buffer leeren.
  }

  if (command != "") {
    Fona3G.print(command + "\r\n"); //Kommando senden
    Serial.println("Command: " + command );
  }

  unsigned long entry = millis(); // Timer Starten
  while (1) {
    if (Fona3G.available() > 0) {
      char c = Fona3G.read();

      if (c != '\n') {
        Response += c;
      }
      else {
        Response += c; //Newline char als letztes hinzufügen

        Serial.print("Response:" + Response);
        Response.trim(); //Steuerzeichen entfernen
        if (Response != "") {
          if (Response == response_a) {
            Serial.println("Result: 1");
            return 1;
          }
          else if (Response == response_b) {
            Serial.println("Result: 2");
            return 2;
          }
          else if (Response == response_c) {
            Serial.println("Result: 3");
            return 3;
          }
          Response = "";
        }
      }
    }

    if ((millis() - entry) > ser_timeout) {
      Serial.println("Result: 0");
      return 0;
    }
  }
}

void ResetBoard() {
  digitalWrite(RST_FONA, LOW);
  delay(200);
  digitalWrite(RST_FONA, HIGH);
  Serial.println("Resetting Fona3G...");
}

int UnlockSIM() {
  AdvancedParser("AT", "OK", "", "", 5000);
  AdvancedParser("AT+CPIN?", "OK", "", "", 5000);
  Serial.println(cfg.sim_pin);
  if (AdvancedParser("AT+CPIN=\"" + String(cfg.sim_pin) + "\"", "OK", "", "", 5000) != 1) { //if (AdvancedParser("AT+CPIN=\"4804\"", "OK", "", "", 5000) != 1) {
    Serial.println("SIM UNLOCK ERROR");
    return 0;
  }
  else {
    Serial.println("SIM UNLOCKED");
    return 1;
  }

}


void CloseSession() {
  ATCOM("AT+CHTTPSCLSE", 500);
  ATCOM("AT+CHTTPSSTOP", 500);
}

int sendWithCheck(String Filename) {
  String header = "";
  long BytesInFile = 0;
  long BytesInHeader = 0;
  long BytesCounter = 0;
  long BytesRemain = 0;
  long BytesToTransmit = 0;

  // open the file for counting Bytes:
  myFile = SD.open(Filename);

  Serial.println("Reading File: " + Filename);
  if (myFile) {
    // read from the file until there's nothing in it:
    while (myFile.available()) {
      char c = myFile.read();
      Serial.print(c);
      if (c != '\r') {
        BytesInFile++;
      }
      else {
        Serial.print("*");
      }
    }
    // close the file:
    myFile.close();

    Serial.println("Found: " + String(BytesInFile));
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening file!");
  }
  // Compose HEADER
  header = "POST /write?db=ALPHASENSE&precision=s HTTP/1.1\r\n";
  header += "Host: Fona3G\r\n";
  header += "User-Agent: Fona3G\r\n";
  header += "Accept: */*\r\n";
  header += "Content-Type: application/x-www-form-urlencoded\r\n";
  header += "Content-Length: " + String(BytesInFile) + "\r\n";
  header += "\r\n";

  BytesInHeader = header.length();

  BytesToTransmit = BytesInHeader + BytesInFile;
  BytesRemain = BytesToTransmit;

  Serial.println("Counted Bytes:");
  Serial.println("Header: " + String(BytesInHeader));
  Serial.println("File:   " + String(BytesInFile));
  Serial.println("Total:  " + String(BytesToTransmit));

  Parser("AT+CHTTPSSEND=1024", 500);

  Fona3G.print(header);
  WaitSerialData(500);

  myFile = SD.open(Filename);


  /////////// Header und Erste Zeilen

  while (BytesCounter <= 1023 - BytesInHeader) {
    Serial.println(BytesCounter);
    char c = myFile.read();
    if (c != '\r') {
      Fona3G.write(c);
      BytesCounter++;
    }
    else {
      //Serial.println("Found R!");
    }
  }

  BytesRemain -= 1024;

  /////////// 1024-Byte Blöcke

  //WaitForString("OK", 5000);

  delay(200);

  while (BytesRemain > 1024) {
    BytesCounter = 0;

    ATCOMMAND("AT+CHTTPSSEND=1024");
    WaitForChar('>', 5000);
    //Parser("AT+CHTTPSSEND=1024", 200);
    //WaitSerialData(500);

    Serial.println("Beginne zu Senden!");

    while (BytesCounter < 1024) {
      char c = myFile.read();
      if (c != '\r') {
        Fona3G.write(c);
        BytesCounter++;
      }
      else {
        //Serial.println("Found R!");
      }
    }

    Serial.println("Senden beended!");
    WaitForString("OK", 60000);
    Serial.println("Warten beended!");

    //Parser("AT+CHTTPSSEND?",2000);
    BytesRemain -= 1024;
  }

  /////////// Ende der Datei

  delay(300);
  Parser("AT+CHTTPSSEND=" + String(BytesRemain) , 500);

  BytesCounter = 0;

  while (BytesCounter < BytesRemain) {
    char c = myFile.read();
    if (c != '\r') {
      Fona3G.write(c);
      BytesCounter++;
    }
    else {
      //Serial.println("Found R!");
    }
  }

  while (myFile.available()) {
    char c = myFile.read();
    Serial.print(c);
    if (c != '\r') {
      Fona3G.write(c);
    }
    else {
      //Serial.print("*");
    }
  }
  // close the file:
  myFile.close();

  WaitSerialData(500);
  //Parser("AT+CHTTPSSEND", 500);

  Serial.println("###############Warte auf Recv>0#################");

  unsigned long entry = millis();

  while ((AdvancedParser("AT+CHTTPSRECV?", "+CHTTPSRECV: LEN,0", "", "", 5000) == 1 && (millis() - entry) < 5000 ));


  Serial.println("#############Recv ist da############");
  if (AdvancedParser("AT+CHTTPSRECV=9000", "HTTP/1.1 204 No Content", "", "", 5000) == 1)
    return 1;
  else
    return 0;

  //ATCOM("AT+CHTTPSRECV=300", 500);
}

int QuickConnect(String server_ip, String server_port) {

  AdvancedParser("AT+CREG?", "OK", "", "", 5000);
  AdvancedParser("AT+CREG=1", "+CREG: 1", "OK", "", 10000);
  AdvancedParser("AT+CGATT=1", "OK", "ERROR", "", 5000);

  //AdvancedParser("AT+CGSOCKCONT=1,\"IP\",\"internet.telekom\"", "OK", "", "", 5000);
  //AdvancedParser("AT+CSOCKAUTH=1,1,\"congstar\",\"cs\"", "OK", "", "", 5000);

  AdvancedParser("AT+CGSOCKCONT=1,\"IP\",\"" + String(cfg.apn) +"\"", "OK", "", "", 5000);
  AdvancedParser("AT+CSOCKAUTH=1,1,\"" + String(cfg.apn_usr) + "\",\"" + String(cfg.apn_pw) + "\"", "OK", "", "", 5000);
  AdvancedParser("AT+CSOCKSETPN=1", "OK", "", "", 5000);
  AdvancedParser("AT+NETOPEN", "+NETOPEN: 0", "+IP ERROR: Network is already opened", "+NETOPEN: 1", 20000);

  AdvancedParser("AT+CHTTPSSTART", "OK", "", "", 5000);


  //if (AdvancedParser("AT+CHTTPSOPSE=\"130.149.67.168\",3000,1", "OK", "", "", 5000) == 1) {   // Test TCP-Server
  if (AdvancedParser("AT+CHTTPSOPSE=\""+ server_ip + "\"," + server_port + ",1", "OK", "", "", 5000) == 1) {   //  InfluxDB server_port
  return 1;
}
else {
  return 0;

}
//AdvancedParser("AT+CHTTPSOPSE=\"130.149.67.168\",3000,1", 1000);


}

int EstablishConnection(String server_ip, String server_port){
  int FailedConnection = 0;
  ResetBoard();

  AdvancedParser("", "+CPIN: SIM PIN", "", "", 10000);

  if (cfg.sim_pin_en == 1){
  if (UnlockSIM() == 0) {
    Serial.println("SIM Failed");
    return 0;
  }
  }

  while (QuickConnect(server_ip,server_port) == 0) {
    FailedConnection++;

    if (FailedConnection > 4) {
      Serial.println("Connection Failed");
      return 0;
    }
  }
  return 1;
}

int SendSequence(String FileToSend) {

  int FailedUpload = 0;

  while (sendWithCheck(FileToSend) == 0) {
    FailedUpload++;

    if (FailedUpload > 4) {
      Serial.println("Upload Failed");
      return 0;
    }
      }
    Serial.println("Upload complete");
    return 1;
}

/*
int divideFile(int n) {
  if(SD.exists("SEND.txt")) {
    SD.remove("SEND.txt");
  }
  if(SD.exists("REST.txt")) {
    SD.remove("REST.txt");
  }

  if(SD.exists("DATA.txt")) {
   dataFile = SD.open("DATA.txt", FILE_READ);
   sendFile = SD.open("SEND.txt", FILE_WRITE);

   if(dataFile) {
     int linebreak = 0;
     char c[256] = "";
     while(dataFile.available()) {
       if(linebreak < n) {
         for (size_t i = 0; i < 256; i++) {
           c[i] = dataFile.read();
           if(c[i] == '\n') {
             linebreak++;
             if(linebreak == n) {
               c[i+1] = '\0';
               break;
             }
           }
           if(c[i] == (char)-1) {
             c[i] = '\0';
             break;
           }
         }
         for (size_t k = 0; k < 256; k++) {
           if (c[k] != '\0') {
             sendFile.write(c[k]);
           } else {
             break;
           }
         }
       } else {
         if(!restFile) {
           sendFile.close();
           restFile = SD.open("REST.txt", FILE_WRITE);
           /*for (size_t i = 0; i < 256; i++) {
             c[i] = (char)0;
           }*/
           /*
           c[0] = '\0';
         }
         for (size_t i = 0; i<256; i++) {
           c[i] = dataFile.read();
           if(c[i] == (char)-1) {
             c[i] = '\0';
             break;
           }
         }
         for (size_t j = 0; j < 256; j++) {
          if (c[j] != '\0') {
            restFile.write(c[j]);
          } else {
            break;
          }
         }
       }
     }
     dataFile.close();
     restFile.close();
     if(SD.exists("REST.txt")) {
       return 1;
     } else {
       sendFile.close();
       return 0;
     }
   } else {
     Serial.println("Couldn't open DATA.txt or SEND.txt!");
     sendFile.close();
     dataFile.close();
     return -1;
   }
 } else {
   Serial.println("No data file!");
   return -1;
 }
}
*/

/*
int restToNewDataFile() {
  char c[256] = "";
  if(SD.exists("REST.txt")) {
    restFile = SD.open("REST.txt", FILE_READ);
    if(restFile) {
      if(SD.exists("DATA.txt")) {
        SD.remove("DATA.txt");
      }
      dataFile = SD.open("DATA.txt", FILE_WRITE);
      while(restFile.available()) {
        for (size_t i = 0; i < 256; i++) {
          c[i] = restFile.read();
          if(c[i] == (char)-1) {
            c[i] = '\0';
            break;
          }
        }
        for (size_t j = 0; j < 256; j++) {
          if(c[j] != '\0') {
            dataFile.write(c[j]);
          } else {
            break;
          }
        }
      }
      dataFile.close();
      restFile.close();
      if(SD.exists("DATA.txt")) {
        SD.remove("REST.txt");
        return 1;
      }
      return 0;
    } else {
      Serial.println("Couldn't open REST.txt!");
      return -1;
    }
  } else {
    Serial.println("No REST file to update from!");
    return -1;
  }
}
*/