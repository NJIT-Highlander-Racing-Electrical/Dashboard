// Get CANBUS Data here

/*
*     FUEL GAUGE: 0-9 VALUE
*         Note: if fuel sensor is malfunctioning or not initialized on power on, set to zero; no fuel lights will be displayed. If fuel is working but empty, make sure its minimum value is 1 so red fuel LED stays lit.
*     2WD/4WD INPUT FROM DAQ
*     LOW BAT WARNING FROM DAQ
*     CVT PRIMARY/SECONDARY RPM FROM CVT
*     CVT RATIO FROM CVT
*     CVT OVER TEMP WARNING FROM CVT
*     
*/

void updateCanbus() {

  int packetSize = CAN.parsePacket();

  if ((packetSize || CAN.packetId() != -1) && (packetSize != 0)) {
    // received a packet
    int packetId = CAN.packetId();  // Get the packet ID

    Serial.print("Received packet with id 0x");
    Serial.print(packetId, HEX);
    Serial.print(" and length ");
    Serial.println(packetSize);

    switch (packetId) {
        // Primary RPM Case
      case 0x1F:
        primaryRPM = CAN.parseInt();
        Serial.print("Received primary RPM as: ");
        Serial.println(primaryRPM);
        break;

        // Secondary Primary RPM Case
      case 0x20:
        secondaryRPM = CAN.parseInt();
        Serial.print("Received secondary RPM as: ");
        Serial.println(secondaryRPM);
        break;

      case 0x21:
        cvtTemp = CAN.parseInt();
        Serial.print("Received cvtTemp as: ");
        Serial.println(cvtTemp);
        break;


      default:
        Serial.println("Unknown packet ID");
        while (CAN.available()) {
          CAN.read();  // Discard the data
        }
        break;
    }

  }

}















/*

// try to parse packet
  int packetSize = CAN.parsePacket();

  if (packetSize || CAN.packetId() != -1) {
    // received a packet
    Serial.print("Received ");

    if (CAN.packetExtended()) {
      Serial.print("extended ");
    }

    if (CAN.packetRtr()) {
      // Remote transmission request, packet contains no data
      Serial.print("RTR ");
    }

    Serial.print("packet with id 0x");
    Serial.print(CAN.packetId(), HEX);

    if (CAN.packetRtr()) {
      Serial.print(" and requested length ");
      Serial.println(CAN.packetDlc());
    } else {
      Serial.print(" and length ");
      Serial.println(packetSize);

      // only print packet data for non-RTR packets
      while (CAN.available()) {
        Serial.print((char)CAN.read());
      }
      Serial.println();
    }

    Serial.println();
  }


  */