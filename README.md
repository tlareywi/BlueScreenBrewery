# Blue Screen Brewery
A sophisticated system for brewery automation built around [Node-Red](https://nodered.org/) and [MQTT](https://mqtt.org/). Node-Red is a widely adopted low-code programming environment used in production automation systems and well suited for brewery automation. MQTT is an IoT (Internet of Things) communication protocol built on TCP. Blue Screen Brewery (BSB) leverages both these technologies to integrate sensors and devices found in the brewery along with external software services in a single integrated user interface. BSB is inspired by other brewery automation systems such as [BruControl](https://brucontrol.com/) and [CraftBeerPi](http://web.craftbeerpi.com/) but aims for maximum flexibility and extensibility in a low-code environment. BSB is entirely open source under the Apache 2 license. Some examples of elements that are possible to integrate with the 'out-of-the-box' firmware are;  

* DC Pumps (more generally, PWM devices)
* SSR controlled AC heating elements
* Digital flow meters
* DC heating elements
* Tilt sensors
* Onewire temperature sensors
* Atlas Scientific EZO sensors (pH and DO2)
* BrewFather via 3rd party contributor to Node-Red   

## Architecture
The center of the system is the Node-Red instance running on any supported platform; Windows, MacOS, Rasp-Pi, Azure cloud, etc. The MQTT broker is generally, but not necessarily, installed on the same machine. Node-Red communicates with any number of Arduino boards (ESP32) via MQTT to controll attached devices, read from sensors, etc. The firmware is common between all Arduino devices and configured dynamically via messages from Node-Red. More on this in the details below.

![BSB Arch](screen_captures/BlueScreenBrewery.png)

## User Interface
A browser based UI is generated via use of Node-Red Deshboard nodes within the flow. Create a layout and UI elements specific to your brewery needs. An example of a three vessel 'hot side' configuration is shown below.

![BSB UI](screen_captures/BSBHotSide.png)

## Development Status
Still in the early stages of development. Supported devices, interfaces and various other aspects of the code are open to change as new requirements are discovered, feedback from other users is considered, etc. Key components levergaged, such as Node-Red and Mosquitto, are already stable but the firmware was a ground up effort and almost certainly contains bugs and/or unexpected behavior. However, I'm using it in my own brewery which contains flow meters, SSRs (AC burners), Onewire sensors, Tilt devices, several DC pumps, Altas sensors and Brewfather integration so most devices have some degree of testing. I'm continuing to make fixes and tweaks as I use the software and receive user feedback. For hazardous devices such as burners, I strongly recommend hardware kill switches are installed to override unexpected behavior. 

# Installation
The recommmended option for installation is to run a pre-configured Docker image. Watch the [quick-start video](https://www.youtube.com/watch?v=PL1thxe_Kj0) for a walk-through of the entire setup process including builing the firmware targetting ESP32 board(s). The Docker image can run on any platform supporting Docker; pretty much anything down to a Rasp Pi. If for some reason you wish to run Node-Red and MQTT natively, there are [additional instructions](README_Native.md). During the quick-start, you'll need to pass some custom shell commands when initializing the Docker container. Here for cut/paste convinience ```docker run -p 1880:1880 -p 8883:8883 -ti tlareywi/blue-screen-brewery sh -c 'systemctl start nodered && systemctl start mosquitto && su bsb && cd && bash'```.

### Installation Issues (not covered in quick-start)
* Configure your network to assign the same IP (static IP or DHCP reservation) to the machine Docker, or the native MQTT broker, is running on. This IP is 'burned' into the firmware during set-up and the devices will be unable to connect if the IP changes.
* The default password is ```password``` during the quick-start process. Use this whenever prompted.
* For the first run of Node-Red the username is ```bsb``` and the password is ```password```.
* Certificate authentication is used for devices connecting to the MQTT broker. These will expire in late 2032 and you'll need to re-create the certs and update the firmware to match. Since there will likely be many other reasons to update the firmware prior to this, it does not seem like a practical limitation.

## Configuration
In Node-Red, navigate to the Interfaces tab and double click either the ```Hot Side Configuration``` or ```Cold Side Configuration``` nodes. Example JSON is provided in each which defines the mapping of MQTT messages to/from pins and the pin function. Each device type and its arguments is documented below. See the quick-start video for further detail. 

## Device Types
These are the valid devices to be used in the configuration 'Type' field(s). Note, the 'Topic' field is always required and denotes the MQTT messsage that will be subscribed to or published on depending on the type. For example, type Digital-In will publish to the Topic while type Digital-Out will subscribe to the Topic. 

* **Digital-In**  - Reads a binary value, 0 or 1, from the pin given in the GPIO field. The message is broadcast whenever the value changes.
    - Topic - Required. Payload will be set to 0 or 1 reflecting pin state.
    - GPIO  - Required. An integer representing the assigned GPIO pin.


* **Digital-Out** - Writes a binary value, 0 or 1, to the pin given in the GPIO field. Used to control on/off devices such as some types of pumps and heaters. Node-Red flows can modulate this state over time to implement a 'Duty Cycle' to control devices such as AC heaters attached to solid state relays.
    - Topic      - Required. Set payload to 0 or 1.
    - GPIO       - Required.
    - Active-Low - Optional. Set to true for devices where 0 is interpreted as 'on', as is sometimes the case for relay modules. Default value is false.


* **PWM**         - Pulse width modulation. A digital signal pulsed at a high frequency. Can be used to vary power directly to low voltage devices; e.g . dim an LED, or vary a higher voltage analog signal with some additional hardware. A common application is to vary the flow rate of DC pumps. Advanced users can change the frequency and resolution in Config.h as desired.
    - Topic - Required. Set payload to an integrer in the range [0, 255] unless custom resolution is configured.
    - GPIO  - Required.


* **Analog-In**   - Performs an analog value read from the associated GPIO pin. The message is broadcast whenever the value changes.
    - Topic - Required. Payload is set to an integer value.
    - GPIO  - Required.


* **Counter**     - Publishes 'pulses per second' from the associated GPIO pin at microsecond resolution. A common application is integration of digital flow meters. Counters are limited to 12 per device.
    - Topic - Required. Payload is set to the integer number of pulses received in the last second.
    - GPIO  - Required.   


* **Onewire**     - DS18B20 (Dallas Semiconductor) temperature sensor. The message is broadcast upon a value change.
    - Topic - Required. Payload is set to the floating point value returned by the sensor in F units.
    - GPIO  - Required.
    - Index - Required. Dnotes the index of the sensor on the Onewire bus in the range of number of sensors on the bus; e.g For three sensors valid values are 0, 1 and 2.


* **Atlas**       - Integrates an Atlas Scientific sensor via their EZO embedded board such as pH and dissolved O2 meters.
   - Topic   - Required. Payload is set to raw data from the sensor. This can be either a status string (e.g. *OK) or a reading such as 4.25. 
   - Rx      - Required. The integer software serial GPIO pin used for rx (receive).
   - Tx      - Required. The integer software serial GPIO pin used for tx (send).
   - Command - Optional. The MQTT message the sensor will listen on for incoming commands. This can be used to issue calibration or temperature compensation commands from Node-Red flows. 


* **Tilt**        - Integrates temperature and Sp.Gr. readings from a Tilt sensor.
    - Topic - Required. Payload is set to JSON containing the reading data in the format {"Temp": x, "Grav": y}.
    - GPIO  - Required.
    - Index - Required. Index in this context maps to the Tilt sensor color. The mappings are given below. For example, to integrate a Blue Tilt set Index to 5.   

| Index                                     | Color                                      |
| ------------------------------------------| ------------------------------------------:|
| 0                                         | Red                                        |
| 1                                         | Greed                                      |
| 2                                         | Black                                      |
| 3                                         | Purple                                     |
| 4                                         | Orange                                     |
| 5                                         | Blue                                       |
| 6                                         | Yellow                                     |
| 7                                         | Pink                                       |

# Troubleshooting

## Arduino Serial Console
If the firmware fails to register with Node-Red and the Interface flow is 'Connected' then the device may either be failing to connect to the MQTT broker or is not connected to the WiFi network. The [Arduino IDE](https://docs.arduino.cc/software/ide-v2) contains a serial console that will print early debug messages from the device. Ensure the device is connected via USB and the console is on the correct port. Upon boot, the firmware will print its IP address if it's able to connect to WiFi. If this doesn't happen then review your SSID and password settings in Config.h. Next, if there is any problem connecting to MQTT it will print a detailed error message. Usually, this is either the wrong IP address specified or an authentication issue. Again, review the settings in Config.h and the MQTT configuration steps earlier in this document.

## Node-Red Debug Sidebar
If Node-Red is receiving the register message from the device then any further diagnostic messages from the firmware are broadcast via the ```BSB/Console``` message. Assuming you've imported the ```Interface``` flow, these messages are already routing to the [Node-Red debug sidebar](https://nodered.org/docs/user-guide/editor/sidebar/debug). This is a good place to check for errors reported from bad device configuration JSON or failures to initialize sub-systems like OneWire or Bluetooth. You can also add Node-Red debug nodes to your flows to print the incomming MQTT messages from the firmware to see exactly what is being sent and how often.  
