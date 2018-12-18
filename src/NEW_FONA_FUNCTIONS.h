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
  if (AdvancedParser("AT+CPIN=\"4804\"", "OK", "", "", 5000) != 1) {
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
  header = "POST /write?db=ALPHASENSE HTTP/1.1\r\n";
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

int QuickConnect() {

  AdvancedParser("AT+CREG?", "OK", "", "", 5000);
  AdvancedParser("AT+CREG=1", "+CREG: 1", "", "", 10000);
  AdvancedParser("AT+CGATT=1", "OK", "ERROR", "", 5000);
  AdvancedParser("AT+CGSOCKCONT=1,\"IP\",\"internet.telekom\"", "OK", "", "", 5000);
  AdvancedParser("AT+CSOCKAUTH=1,1,\"congstar\",\"cs\"", "OK", "", "", 5000);
  AdvancedParser("AT+CSOCKSETPN=1", "OK", "", "", 5000);
  AdvancedParser("AT+NETOPEN", "+NETOPEN: 0", "+IP ERROR: Network is already opened", "", 20000);

  AdvancedParser("AT+CHTTPSSTART", "OK", "", "", 5000);


  if (AdvancedParser("AT+CHTTPSOPSE=\"130.149.67.168\",3000,1", "OK", "", "", 5000) == 1) {   // Test TCP-Server
  //if (AdvancedParser("AT+CHTTPSOPSE=\"130.149.67.141\",8086,1", "OK", "", "", 5000) == 1) {   //  InfluxDB
  return 1;
}
else {
  return 0;

}
//AdvancedParser("AT+CHTTPSOPSE=\"130.149.67.168\",3000,1", 1000);


}


int SendSequence(String FileToSend) {
  int FailedConnection = 0;
  int FailedUpload = 0;

  ResetBoard();

  AdvancedParser("", "+CPIN: SIM PIN", "", "", 10000);

  if (UnlockSIM() == 0) {
    Serial.println("SIM Failed");
    return 0;
  }

  while (QuickConnect() == 0) {
    FailedConnection++;

    if (FailedConnection > 4) {
      Serial.println("Connection Failed");
      return 0;
    }
  }

  while (sendWithCheck(FileToSend) == 0) {
    FailedUpload++;

    if (FailedUpload > 4) {
      Serial.println("Upload Failed");
      return 0;
    }
      }
    Serial.println("Upload complete");
    CloseSession();
    return 1;

}
