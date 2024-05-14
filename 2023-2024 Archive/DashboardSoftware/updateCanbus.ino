// Updates all data variables from CAN-Bus data here

void updateCanbus() {

  int packetSize = CAN.parsePacket();

  if ((packetSize || CAN.packetId() != -1) && (packetSize != 0)) {
    // received a packet
    int packetId = CAN.packetId();  // Get the packet ID

    //DEBUG_SERIAL.print("Received packet with id 0x");
    //DEBUG_SERIAL.print(packetId, HEX);
    //DEBUG_SERIAL.print(" and length ");
    //DEBUG_SERIAL.println(packetSize);

    switch (packetId) {
        // Primary RPM Case
      case 0x1F:
        primaryRPM = CAN.parseInt();
        primaryRPM *= 10;
        DEBUG_SERIAL.print("Received primary RPM as: ");
        DEBUG_SERIAL.println(primaryRPM);
        break;

        // Secondary Primary RPM Case
      case 0x20:
        secondaryRPM = CAN.parseInt();
        secondaryRPM *= 10;
        DEBUG_SERIAL.print("Received secondary RPM as: ");
        DEBUG_SERIAL.println(secondaryRPM);
        break;

      //CVT Temperature Case
      case 0x21:
        cvtTemp = CAN.parseInt();
        DEBUG_SERIAL.print("Received cvtTemp as: ");
        DEBUG_SERIAL.println(cvtTemp);
        break;

      case 0x51:
        daqOn = CAN.parseInt();
        DEBUG_SERIAL.print("Received daqOn as: ");
        DEBUG_SERIAL.println(daqOn);
        break;

      case 0x53:
        daqError = CAN.parseInt();
        DEBUG_SERIAL.print("Received daqError as: ");
        DEBUG_SERIAL.println(daqError);
        break;

      case 0x52:
        fourWheelsState = CAN.parseInt();
        DEBUG_SERIAL.print("Received fourWheelsEngaged as: ");
        DEBUG_SERIAL.println(fourWheelsState);
        break;

      case 0x54:
        fourWheelsUnknown = CAN.parseInt();
        DEBUG_SERIAL.print("Received fourWheelsUnknown as: ");
        DEBUG_SERIAL.println(fourWheelsUnknown);
        break;

      case 0x3D:
        batteryLow = CAN.parseInt();
        DEBUG_SERIAL.print("Received batteryLow as: ");
        DEBUG_SERIAL.println(batteryLow);
        break;

      case 0x33:
        fuelLevel = CAN.parseInt();
        DEBUG_SERIAL.print("Received fuelLevel as: ");
        DEBUG_SERIAL.println(fuelLevel);
        break;

      case 0x49:
        timeH = CAN.parseInt();
        DEBUG_SERIAL.print("Received timeH as: ");
        DEBUG_SERIAL.println(timeH);
        break;

      case 0x4A:
        timeM = CAN.parseInt();
        DEBUG_SERIAL.print("Received timeM as: ");
        DEBUG_SERIAL.println(timeM);
        break;

      case 0x4B:
        timeS = CAN.parseInt();
        DEBUG_SERIAL.print("Received timeS as: ");
        DEBUG_SERIAL.println(timeS);
        break;


      default:
        DEBUG_SERIAL.print("Unknown packet ID: ");
        DEBUG_SERIAL.println(packetId);
        while (CAN.available()) {
          CAN.read();  // Discard the data
        }
        break;
    }
  }
}
