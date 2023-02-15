# Introduction
A sophisticated system for brewery automation built around Node-Red and MQTT. Node-Red is a widely adopted low-code programming environment used
in production automation systems and well suited for brewery automation. MQTT is an IoT (Internet of Things) communication protocol built on 
TCP. Blue Screen Brewery (BSB) leverages both these technologies to integrate sensors and devices found in the brewery along with external
software services in a single integrated user interface. Some examples of elements that are possible to integrate with
the 'out-of-the-box' firmware are;  

* DC Pumps (more generally, PWM devices)
* SSR controlled AC heating elements
* Digital flow meters
* DC heating elements
* Tilt sensors
* Onewire temperature sensors
* Atlas Scientific EZO sensors (pH and DO2)
* BrewFather via 3rd party contributor to Node-Red

### Architecture
The center of the system is the Node-Red instance running on any supported platform; Windows, MacOS, Rasp-Pi, Azure cloud, etc. The MQTT broker is generally, but
not necessarily, installed on the same machine. For convinience, a pre-configured Rasp-Pi image is available for download (TODO) which provides a working Node-Red
and MQTT installtion. Node-Red communicates with any number of Arduino boards (ESP32) via MQTT to controll attached devices, read from sensors, etc. The firmware is
common between all Arduino devices and configured dynamically via messages from Node-Red. More on this in the details below. (TODO)

![BSB Arch](screen_captures/BlueScreenBrewery.png)

### User Interface
A browser based UI is generated via use of Node-Red Deshboard nodes within the flow. Create a layout and UI elements specific to your brewery needs. An example of
a three vessel 'hot side' configuration is shown below.

![BSB UI](screen_captures/BSBHotSide.png)

### Building the BSB Firmware
The firmware has only been tested on an ESP32 board and the 'helper' script referenced below assumes such a device.

* Download arduino-cli from https://arduino.github.io/arduino-cli/0.20/installation/
* On the command line, navigate to Firmware_ESP32 and copy arduino-cli here
* Edit Firmware_ESP32/Config.h to configure your device's name, WiFi parameters and SSL/TLS certs if applicable.
* Run `arduino-cli board list` with the board plugged in and unplugged to determine the port.
* On Windows, edit build.bat to set the port and then run built.bat from within Firmware_ESP32
* For other operating systems, use build.bat as a reference for what commands to run.  



