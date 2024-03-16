# 2023-Dashboard

Dashboard for the 2023 vehicle including data from:
* CVT
* Semi-Active Suspension
* Fuel Level Sensor
* BajaDAS
* 2WD/4WD Engagement Switch

## Hardware

* ESP32 Microcontroller
* TJA1051T CAN Transceiver
* Seven Segment Displays
* 1.28" Round LCDs
* 5mm LEDs
* TLC5940 16-Channel LED Drivers

## Key Features

1. Real-Time Vehicle Speed Display
2. Analog Dial and Digital Output of CVT RPM
3. Visual Indicator for 2WD/4WD Engagement
4. Digital CVT Ratio
5. Current Time (HH:MM:SS)
6. LED Indicators for Fuel Level
7. Status LEDs for CVT Temperature, Data Acquisition, Battery Level, and Power On/off
8. CAN-BUS Communication with Rest of Vehicle
9. Boot screen including Highlander Racing logo and Baja vehicle

## Visuals

* Include a "Highlander Racing" boot animation with Baja vehicle similar to this video at 1:20 (https://www.youtube.com/watch?v=Y0BGnHFuYBU&ab_channel=VoltLog)
* Potential Speedometer/RPM display at 1:57 (https://www.youtube.com/watch?v=Y0BGnHFuYBU&ab_channel=VoltLog)

## Useful Sites
* TLC5917 Datasheet: https://www.ti.com/lit/ds/symlink/tlc5917.pdf?ts=1710497247983&ref_url=https%253A%252F%252Fgoogle.com
* 7-Segment Datasheet: http://www.us.kingbright.com/images/catalog/spec/SA10-21SRWA.pdf
* TLC5917 Library: https://github.com/Andy4495/TLC591x/tree/main
* Adafruit CAN Pal Guide: https://learn.adafruit.com/adafruit-can-pal/pinouts
* ESP32 Pinout: https://lastminuteengineers.com/wp-content/uploads/iot/ESP32-Pinout.png

