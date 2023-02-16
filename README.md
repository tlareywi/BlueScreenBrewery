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
common between all Arduino devices and configured dynamically via messages from Node-Red. More on this in the details below.

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

### Firmware Configuration
When the BSB firmware boots, it sends the MQTT message 'BSB/Register'. Node-Red listens for this event and responds with 'BSB/Configure' which contains JSON describing the
device types and mappings between MQTT messages and Arduino pin state. For example, suppose Config.h is edited so the DEVICE_NAME is 'Cold Side'. When this device boots, it
will send the 'BSB/Register' message with the payload 'Cold Side'. Node-Red receives this message and chooses the JSON to send based on the device name payload. Node-Red sends 
the 'BSB/Configure' message with the following JSON payload, which defines two Onewire sensors, two DC heating pads, two DC pumps and two Tilt sensors. The Topic field defines
the MQTT message name to bind the device. The meaning of Index depends on the device type, both of which are documented below. It's important the label, in this case
'Cold Side', matches the DEVICE_NAME in Config.h or the configuration will be ignored. This facilitates many devices running in parallel without stepping on each other. 

```javascript
{
    "Cold Side": [
        {
            "Topic": "BSB/Unitank-1-Temp",
            "Index": 1,
            "Type": "Onewire"
        },
        {
            "Topic": "BSB/Unitank-2-Temp",
            "Index": 0,
            "Type": "Onewire"
        },
        {
            "Topic": "BSB/Unitank-1-Heat",
            "GPIO": 27,
            "Type": "Digital-Out"
        },
        {
            "Topic": "BSB/Unitank-2-Heat",
            "GPIO": 15,
            "Type": "Digital-Out"
        },
        {
            "Topic": "BSB/Unitank-1-Cool",
            "GPIO": 25,
            "Type": "Digital-Out"
        },
        {
            "Topic": "BSB/Unitank-2-Cool",
            "GPIO": 14,
            "Type": "Digital-Out"
        },
        {
            "Topic": "BSB/Blue-Tilt",
            "Index": 5,
            "Type": "Tilt"
        },
        {
            "Topic": "BSB/Purple-Tilt",
            "Index": 3,
            "Type": "Tilt"
        }
    ]
}
```

The image below is an example in Node-Red of supporting four Ardunino devices all with a different configuration. The incoming Register message on the left flows to a Switch node
that routes the message based on the DEVICE_NAME in the payload to the matching Change node. The Change node sets the msg.payload to the approprate JSON and broadcasts in via the
Configure message. The target device reads the configuration and begins listening/publishing as instructed.

![BSB Config](screen_captures/configuration.png)

### Configuration JSON details
TODO