# 2024-2025 Dashboard

A dashboard for the vehicle including data from:
* CVT
* Fuel Level Sensor
* BajaDAS
* Wheel Speed Sensors

## 2024-2025 Design Goals

There are many aspects to improve on from last year's dashboard, as well as several other new features we would like to add. Here are some topics to look into:

* Sunshade
  * Last year, the LCDs were too dim in direct sunlight. This year, we will be implementing a sunshade to fix this
  * This sunshade can be made out of sheet metal, formed to shape with a 3D printed mold, attached via screws to the dashboard's back cover, and painted black to minimize reflections.
 * Improved front cover
     * The acrylic is subject to cracking under pressure from the mounting screws
     * This year, we will be using polycarbonate and we can use rubber washers on the mounting screws too
 * Waterproofing
     * Last year, the dashboard was not waterproof. This year, we are adding gasket material
          * For the front, we will sandwich it between the polycarbonate and 3D printed frame
          * For the rear, we can put it between the rear 3D printed cover and the main 3D printed frame
* Integration of wheel speed sensor data
     * Wheel speed data can be used to determine whether a wheel has locked up or has wheelspin.
     * This should be a bright color that takes priority over anything else on the dashboard. For example, if the front left wheel is locked, the top of the left circular display should flash red.
* Brighter Displays
     * With the dark mode background last year, the displays were not bright enough. This year we are using light backgrounds.
* Driver Input
     * This year, we are implementing two push buttons on the back of the dashboard that the driver can use for miscellaneous functions
         * Switching display data, adjusting brightness, etc
* Other limitations/setbacks are discussed in the [2023-2024 Archive README](https://github.com/NJIT-Highlander-Racing-Electrical/Dashboard/tree/main/2023-2024%20Archive)

## 2024-2025 Known Issues

### GC9A01 SCLK Pin

The SPI Clock pin on the ESP32 is 18, but the PCB has it tied to 19. On the final version, we can just snip off the two pins that go into the PCB and swap them around with two small jumpers. Software Serial is not ideal for speed and would require software modifications.

### TLC5917 Chip 3 OE and R-EXT Pins

On the TLC5917 that controls the green fuel LEDs, the R-EXT pin is tied to ground. This causes the TLC to try to push infinite current into the LEDs. We can simply not solder that pin to the pad, and instead bend it up and jump it to a GND pin using the proper resistor

The OE pin has the resistor that should have been attached to the R-EXT pin. The OE pin needs to be pulled LOW to enable the displays, so we can simply cut off the resistor there and short it to ground.

### Fuel LEDs

 The leftmost red LED does not light up in the boot animation
 


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
