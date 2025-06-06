# Fork Changes

* The coryking/FastLED library was not being loaded. When fixed, there was a type incompatibility between FastLED and working versions of Arduino.h. I had to fork coryking/FastLED to correct this. This fork now loads that fork.
* upload.py had some MacOS-specific code. I made it work on Windows.

# Synchrobike
Synchronized LED color palettes and animations.

The goal of this project is to synchronize amazing LED animations and colors across a wireless node mesh.

[Video Preview](https://github.com/jsonpoindexter/synchrobike/blob/7a36adf7c99e19d84d8cd89cc18fcecfbe31a66b/IMG_6202.mov)

## Components
* 5V Phone Battery Bank
* USB to Micro USB cable
* [50pc 12mm WS281x 5v](https://www.aliexpress.com/item/50-Pcs-string-12mm-WS2811-2811-IC-RGB-LED-Pixels-Module-String-Light-Black-Wire-cable/1854864234.html)
* [WEMOS D1 Mini](https://www.wemos.cc/en/latest/d1/d1_mini.html)
* [Male 3 Pin JST Connector](https://www.aliexpress.com/item/Free-Shipping-10pcs-3pin-JST-Connector-Male-Female-plug-and-socket-connecting-Cable-Wire-for-WS2811/32366522079.html)
* [5M Telescopic Fishing Rod](https://www.aliexpress.com/item/AZJ-Brand-Wholesale-2-1-7-2M-Stream-Fishing-Rod-Glass-Fiber-Telescopic-Fishing-Rod-Ultra/32794897069.html)

## Project Dependencies
* FastLED (ESP8266 DMA Fork): https://github.com/coryking/FastLED  -*this fixes flickering caused by interrupts on the ESP8266 and the WS281x's* 

* ESP8266TrueRandom: https://github.com/marvinroger/ESP8266TrueRandom

* painlessMesh: https://gitlab.com/painlessMesh/painlessMesh
    * Dependencies:
        * ArduinoJson: https://github.com/bblanchon/ArduinoJson
        * TaskScheduler: https://github.com/arkhipenko/TaskScheduler

## Changeable Constants
The following constants in the code can be modified to customize the behavior of the project:

- **`NUM_LEDS`**: The number of LEDs in the WS281x strip. Default is `50`.
- **`DATA_PIN`**: The GPIO pin connected to the LED strip's data line. Default is `GPIO3 (Rx)`.
- **`BRIGHTNESS`**: The brightness level of the LEDs (0-255). Default is `255`.
- **`MESH_PREFIX`**: The Wi-Fi mesh network name. Default is `"synchrobike-fw"`.
- **`MESH_PASSWORD`**: The password for the Wi-Fi mesh network. Default is `"synchrobike-fw"`.
- **`MESH_PORT`**: The port used for the mesh network. Default is `5555`.
- **`REVERSE_ANIMATIONS`**: Whether to reverse LED strip animations. Default is `0` (disabled).
- **`HOLD_PALETTES_X_TIMES_AS_LONG`**: Duration (in seconds) to hold each color palette. Default is `10`.
- **`HOLD_ANIMATION_X_TIMES_AS_LONG`**: Duration (in seconds) to hold each animation. Default is `20`.

These constants can be found and modified in the `synchrobike.ino` file under the `src/` directory.

## Wiring
| **WS281x**        |   **Wemos D1 Mini**| 
| :-------------: |:-------------: |
| Digital In (DI) | GPIO3 (Rx)
| Ground      | Ground       |
| 5v / Vcc | 5v       |

## Powering
Power the Wemos D1 Mini through the micro USB port using a 5v mobile phone battery bank. The peak power usage is around 350-500mA on full brightness (255). 

**Note: It is HIGHLY suggested to unplug the LEDs from the Wemos D1 Mini while uploading the sketch. The LEDs being on the RX pin means that they can possibly interfere with the upload. During upload, all connected LEDs will turn white at max brightness, which could possibly cause harm to the Wemos D1 Mini and/or the USB port.**

## Range 
I was able to successfully sync two nodes at a distance of 180m before running out of line of sight.

## Pinout

![Wemos D1 Mini Pinout](https://www.projetsdiy.fr/wp-content/uploads/2016/05/esp8266-wemos-d1-mini-gpio-pins.jpg)
