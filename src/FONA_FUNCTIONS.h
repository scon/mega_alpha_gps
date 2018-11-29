void TogglePS() {
  digitalWrite(KEY_FONA, LOW);
  delay(5000);
  digitalWrite(KEY_FONA, HIGH);
}

bool GetPS() {

  if (digitalRead(PS_FONA) == HIGH) {
    Serial.println("Module is ON!");
    return true;
  }
  else {
    Serial.println("Module is OFF!");
    return false;
  }

}

void ModemTurnOff() {
  if (GetPS() == true) {
    TogglePS();
  }
}

void ModemTurnOn() {
  if (GetPS() == false) {
    TogglePS();
  }
}


void ATCOM(String command, unsigned long ser_timeout) {

  while (Fona3G.available() > 0) {
    Serial.write(Fona3G.read()); // Serial Buffer leeren.
  }
  Fona3G.print(command + "\r\n");

  while (!Fona3G.available()) {} // Auf Antwort warten.

  unsigned long entry = millis();

  while ((millis() - entry) < ser_timeout || Fona3G.available() > 0) {
    if (Fona3G.available() > 0) {
      Serial.write(Fona3G.read());
    }
  } // Antwort auslesen mit Timeout
}

void WaitSerialData(unsigned long SerialWait) {
  unsigned long wait_entry = millis();
  while (millis() - wait_entry < SerialWait || Fona3G.available() > 0) {
    if (Fona3G.available() > 0) {
      Serial.write(Fona3G.read());
    }
  }
}

void Parser(String command, unsigned long ser_timeout) {

  String Response = "";

  while (Fona3G.available() > 0) {
    Serial.write(Fona3G.read()); // Serial Buffer leeren.
  }

  Fona3G.print(command + "\r\n"); //Kommando senden

  while (!Fona3G.available()) {} // Auf Antwort warten.

  unsigned long entry = millis();

  while ((millis() - entry) < ser_timeout || Fona3G.available() > 0) {
    if (Fona3G.available() > 0) {
      char c = Fona3G.read();

      if (c == '\n') {
        if (Response != "") {
          Response.trim();
          Serial.println("Found: " + Response + " L:" + String(Response.length()));
          //Serial.print(Response);

          if (Response == "OK") {
            Serial.println("Antwort gefunden!");
            return;
          }
        }
        Response = "";
      }
      else {
        Response += c;
      }

      // Serial.write(Fona3G.read());}
    } // Antwort auslesen mit Timeout
  }
}

void TestParser() {
  Parser("AT", 3000);
}

void ResetBoard() {
  digitalWrite(RST_FONA, LOW);
  delay(200);
  digitalWrite(RST_FONA, HIGH);
  Serial.println("Resetting Fona3G...");
}

void EchoOff() {
  ATCOM("ATE0", 500);
}
void EchoOn() {
  ATCOM("ATE1", 500);
}

void QuickConnectToNetwork() {
  Serial.println("Connecting...");
  //Serial.println("Sending...AT");
  Parser("AT", 500);

  Parser("AT+CPIN=\"4804\"", 500);

  //Serial.println("Sending...AT+CREG?");
  Parser("AT+CREG?", 500);
  //Serial.println("Sending...AT+CREG=1");
  Parser("AT+CREG=1", 1000);
  //Serial.println("Sending...AT+CGATT=1");
  Parser("AT+CGATT=1", 1000);
  //Serial.println("Sending...AT+CGSOCKCONT=1...");
  Parser("AT+CGSOCKCONT=1,\"IP\",\"internet.telekom\"", 2000);
  Parser("AT+CSOCKAUTH=1,1,\"congstar\",\"cs\"",500);

  //Serial.println("Sending...AT+CSOCKSETPN");
  Parser("AT+CSOCKSETPN=1", 3000);
  //Serial.println("Sending...AT+NETOPEN");
  Parser("AT+NETOPEN", 4000);
  Serial.println("Connecting DONE");
}


void SendFromSD() {
  // Count bytes to send
  unsigned long count = 0;
String header = "";
unsigned long BytesToTransmit = 0;

  // re-open the file for reading:
  myFile = SD.open("DATA.txt");
  if (myFile) {
    Serial.println("DATA.txt:");
    // read from the file until there's nothing else in it:
    while (myFile.available()) {
      char c = myFile.read();
      Serial.print(c);
      if (c != '\r') {
        count++;
      }
      else {
        Serial.print("*");
      }
    }
    // close the file:
    myFile.close();
    Serial.println("count: " + String(count));
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening test.txt");
  }


  header = "POST /write?db=ALPHASENSE HTTP/1.1\r\n";
  header += "Host: Fona3G\r\n";
  header += "User-Agent: Fona3G\r\n";
  header += "Accept: */*\r\n";
  header += "Content-Type: application/x-www-form-urlencoded\r\n";
  header += "Content-Length: " + String(count) + "\r\n";
  header += "\r\n";

  BytesToTransmit = header.length() + count;

  Serial.println("Message:");
  Serial.print(header);

  Serial.println("Length(H+P): " + String(BytesToTransmit));

  ATCOM("AT+CHTTPSSEND=" + String(BytesToTransmit), 500);

  Fona3G.print(header);
  WaitSerialData(500);

  // read File again and send

  myFile = SD.open("DATA.txt");
  if (myFile) {
    Serial.println("DATA.txt:");


    // read from the file until there's nothing else in it:
    while (myFile.available()) {
      char c = myFile.read();
      if (c != '\r') {
        Fona3G.write(c);
      }
      else {
        Serial.println("Found R!");
      }

    }
    // close the file:
    myFile.close();

    WaitSerialData(500);

    ATCOM("AT+CHTTPSSEND", 500);
    ATCOM("AT+CHTTPSRECV=300", 500);

  }

}


void CommandWithAnswer(String command, String answer,  unsigned long ser_timeout) {

  String Response = "";

  while (Fona3G.available() > 0) {
    Serial.write(Fona3G.read()); // Serial Buffer leeren.
  }

  Fona3G.print(command + "\r\n"); //Kommando senden

  while (!Fona3G.available()) {} // Auf Antwort warten.

  unsigned long entry = millis();

  while ((millis() - entry) < ser_timeout || Fona3G.available() > 0) {
    if (Fona3G.available() > 0) {
      char c = Fona3G.read();

      // if (c == '>') {
      //   Serial.println("> gefunden!");
      //   return;
      //}

      if (c == '\n') {
        if (Response != "") {
          Response.trim();
          Serial.println("Answer: " + Response + " L:" + String(Response.length()));
          //Serial.print(Response);

          if (Response == answer) {
            Serial.println("Antwort gefunden!");
            return;
          }
        }
        Response = "";
      }
      else {
        Response += c;
      }

      // Serial.write(Fona3G.read());}
    } // Antwort auslesen mit Timeout
  }
}

void WaitForString(String answer,  unsigned long ser_timeout) {

  Serial.println("Warte auf String: " + answer );

  String Response = "";

  //while (!Fona3G.available()) {} // Auf Antwort warten.

  unsigned long entry = millis();

  while ((millis() - entry) < ser_timeout || Fona3G.available() > 0) {

   // Serial.println("Warte auf String");

    if (Fona3G.available() > 0) {
      char c = Fona3G.read();
      Serial.write(c);

      if (c == '\r') {
        if (Response != "") {
          Response.trim();
          Serial.println("Response: " + Response + " L:" + String(Response.length()));
          //Serial.print(Response);
          if (Response == answer) {
            Serial.println("String gefunden!:" + answer);
            Serial.println( String(millis() - entry));
            return;
          }
        }
        Response = "";
      }
      else {
        Response += c;
      }

      // Serial.write(Fona3G.read());}
    } // Antwort auslesen mit Timeout
  }
}

void WaitForChar(char answer, unsigned long ser_timeout) {

  Serial.print("Warte auf Char:");
  Serial.write(answer);
  Serial.println("");

  unsigned long entry = millis();

  while ((millis() - entry) < ser_timeout) {

    if (Fona3G.available() > 0) {
      char c = Fona3G.read();
      Serial.write(c);

      if (c == '>') {
        Serial.println("Char gefunden!");
        return;
      }
    }
  }
  Serial.println("Char NICHT gefunden!");
}

void ATCOMMAND(String command) {
  Fona3G.print(command + "\r\n");
}

void SendLongSD() {

  String header = "";
  String Filename = "DATA.txt";
  long BytesInFile = 0;
  long BytesInHeader = 0;
  long BytesCounter = 0;
  long BytesRemain = 0;
  long BytesToTransmit = 0;

  // open the file for counting Bytes:
  myFile = SD.open(Filename);

  Serial.println("Reading File: " + Filename);
  if (myFile) {
    // read from the file until there's nothing else in it:
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

    //Serial.println("Found: " + String(BytesInFile));
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
    WaitForChar('>', 1500);
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
    WaitForString("OK", 1500);
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
  SD.remove("DATA.txt");

  WaitSerialData(500);
  Parser("AT+CHTTPSSEND", 500);
  ATCOM("AT+CHTTPSRECV=1023", 500);
}
