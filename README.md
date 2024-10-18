# 2024-2025 Dashboard

A dashboard for the vehicle including data from:
* CVT
* Fuel Level Sensor
* BajaDAS
* Wheel Speed Sensors

## 2024-2025 Design Goals

There are many aspects to improve on from last year's dashboard, as well as several other new features we would like to add. Here are some topics to look into:

* Sunshade
  * Last year, the LCDs were too dim in direct sunlight. Although there may not be brighter LCDs available for purchase, a sunshade may fix this issue
  * The 7-segments were also dim, however the resistor on the LED drivers used to set the supply current meant that they were also not running at peak brightness.
 * Improved front cover
     * The acrylic is subject to cracking under pressure from the mounting screws
     * Either rubber washers to reduce stress (also improves waterproofing)
     * Or stronger material 
* Integration of wheel speed sensor data (if useful)
* Better graphic design
* Other limitations/setbacks are discussed in the [2023-2024 Archive README](https://github.com/NJIT-Highlander-Racing-Electrical/Dashboard/tree/main/2023-2024%20Archive)


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
* At the competition, it was apparent that the whole dash was fairly dim in the bright sun
  * Goals for next year should be to incorporate an (ideally detachable) sunshade, brighter LEDs, brighter LCD displays or a suitable alternative
  * Other nice-to-haves would be a pot for brightness control and/or some other kind of driver input to change menus or settings
  * G-Force on driver indicator with magnitude and direction would be neat as well
