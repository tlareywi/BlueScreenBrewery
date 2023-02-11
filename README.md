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
* BrewFather via 3rd party contributor to Node-Red

### Architecture
The center of the system is the Node-Red instance running on any supported platform; Windows, MacOS, Rasp-Pi, Azure cloud, etc. The MQTT broker is generally, but
not necessarily, installed on the same machine. For convinience, a pre-configured Rasp-Pi image is available for download (TODO) which provides a working Node-Red
and MQTT installtion. Node-Red communicates with any number of Arduino boards (ESP32) via MQTT to controll attached devices, read from sensors, etc. The firmware is
common between all Arduino devices and configured dynamically via messages from Node-Red. More on this in the details below. (TODO)

![BSB Arch](screen_captures/arch.png)


