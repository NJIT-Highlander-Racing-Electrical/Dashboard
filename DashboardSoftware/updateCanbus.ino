// Updates all data variables from CAN-Bus data here

/*
*     FUEL GAUGE: 0-9 VALUE
*         Note: if fuel sensor is malfunctioning or not initialized on power on, set to zero; no fuel lights will be displayed. If fuel is working but empty, make sure its minimum value is 1 so red fuel LED stays lit.
*     2WD/4WD INPUT FROM DAQ
*     LOW BAT WARNING FROM DAQ
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

      //CVT Temperature Case
      case 0x21:
        cvtTemp = CAN.parseInt();
        Serial.print("Received cvtTemp as: ");
        Serial.println(cvtTemp);
        break;

      case 0x51:
        daqOn = CAN.parseInt();
        Serial.print("Received daqOn as: ");
        Serial.println(daqOn);
        break;

      case 0x53:
        daqError = CAN.parseInt();
        Serial.print("Received daqError as: ");
        Serial.println(daqError);
        break;

      case 0x52:
        fourWheelsState = CAN.parseInt();
        Serial.print("Received fourWheelsEngaged as: ");
        Serial.println(fourWheelsState);
        break;

      case 0x54:
        fourWheelsUnknown = CAN.parseInt();
        Serial.print("Received fourWheelsUnknown as: ");
        Serial.println(fourWheelsUnknown);
        break;

      case 0x3D:
        batteryLow = CAN.parseInt();
        Serial.print("Received batteryLow as: ");
        Serial.println(batteryLow);
        break;

      case 0x33:
        fuelLevel = CAN.parseInt();
        Serial.print("Received fuelLevel as: ");
        Serial.println(fuelLevel);
        break;

      case 0x49:
        timeH = CAN.parseInt();
        Serial.print("Received timeH as: ");
        Serial.println(timeH);
        break;

      case 0x4A:
        timeM = CAN.parseInt();
        Serial.print("Received timeM as: ");
        Serial.println(timeM);
        break;

      case 0x4B:
        timeS = CAN.parseInt();
        Serial.print("Received timeS as: ");
        Serial.println(timeS);
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
