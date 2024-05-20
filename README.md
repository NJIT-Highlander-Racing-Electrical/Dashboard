# 2023-2024 Dashboard

Dashboard for the 2023 vehicle including data from:
* CVT
* Semi-Active Suspension
* Fuel Level Sensor
* BajaDAS
* 2WD/4WD Engagement Switch

## Hardware

* ESP32 Microcontroller (Select NodeMCU-32S board in Arduino IDE)
* TJA1051T CAN Transceiver
* 1" Seven Segment Displays (Kingbright SA10-21SRWA)
* 1.28" Round LCDs
* 5mm LEDs
* TLC5917 8-Channel Sinking Constant Current LED Drivers

## Key Features

1. Real-Time Vehicle Speed Display
2. Analog Dial and Digital Output of CVT RPM
3. Visual Indicator for 2WD/4WD Engagement
4. Digital CVT Ratio
5. Current Time (HH:MM:SS)
6. LED Indicators for Fuel Level
7. Status LEDs for CVT Temperature, Data Acquisition, Battery Level, and Power On/off
8. CAN-BUS Communication with Rest of Vehicle
9. Boot screen including Highlander Racing logo

## ESP32 Dual-Core Utilization

* We decided to run CAN-Bus scanning solely on Core 0 (via a task) to ensure all messages are properly received
* Writing to displays, 7-segment drivers, and math functions all take time, which means messages can be missed

## Visuals

* Includes a custom boot animation featuring the Highlander Racing logo and Lightning McQueen's number: 95
* RPM Visual comes from this repository: https://github.com/VolosR/TDisplayDashboard/tree/main
 
## Useful Sites
* TLC5917 Datasheet: https://www.ti.com/lit/ds/symlink/tlc5917.pdf?ts=1710497247983&ref_url=https%253A%252F%252Fgoogle.com
* 7-Segment Datasheet: http://www.us.kingbright.com/images/catalog/spec/SA10-21SRWA.pdf
* TLC5917 Library: https://github.com/Andy4495/TLC591x/tree/main
* Adafruit CAN Pal Guide: https://learn.adafruit.com/adafruit-can-pal/pinouts
* ESP32 Pinout: https://lastminuteengineers.com/wp-content/uploads/iot/ESP32-Pinout.png

 ## Known Issues/Limitations
 * The current PCB inside the dashboard has the fuel LEDs wire as follows
   * TLC Output -> LED Anode -> LED Cathode -> Current Limiting Resistor -> Ground
   * However, the TLC5917 is a sinking driver, and thus the LEDs need a 5V source
   * This issue should be fixed in the most up-to-date KiCad schematic and PCB
 * The density of the components and traces meant that seven traces could not be made without increasing the board thickness beyond two layers
   * To accomodate this, jumper wires were installed point to point to make the missing connections
 * The through holes for the LCDs and CAN-Bus module were too small and the headers had to be adjusted
 * The enclosure for the dashboard needs some improvements:
   * Better waterproofing (obviously)
   * Improved acrylic support
     * The front clear acrylic cover cracked due to the pressure from the bolts
* The DAS/battery subsystems do not include a method to obtain a low battery warning, so the battery light on the dashboard is not active
* The fuel sensor was not installed on the 2024 vehicle due to a lack of time, so the fuel LEDs are not active
* At the competition, it was apparent that the whole dash was fairly dim in the bright sun
  * Goals for next year should be to incorporate an (ideally detachable) sunshade, brighter LEDs, brighter LCD displays or a suitable alternative  
