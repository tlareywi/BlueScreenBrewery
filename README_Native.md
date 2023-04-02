# Native Raspberry Pi Installation
This document covers configuring Node-Red and Mosquitto (MQTT) to run natively on a Rasp Pi machine. If you're not confortable in a Linux environment, please follow the Docker instructions and quick start video in the main README file instead. After completing the steps here, return to the main README for info on configuring the ESP32 device(s) as those steps are common in both apprroaches.

## Raspberry Pi System with Node-Red and Mosquitto (MQTT)

* Follow the instuctions for installing [Ubuntu-Server](https://ubuntu.com/tutorials/how-to-install-ubuntu-on-your-raspberry-pi#1-overview) using the RaspPi Imager.
* Next, perform the steps here for installing [Node-Red](https://nodered.org/docs/getting-started/raspberrypi)
* Run ```sudo apt install mosquitto mosquitto-clients``` to install the Mosquitto MQTT broker.
* Run ```sudo systemctl enable mosquitto``` to automatically run Mosquitto on system restart.

## Configure and Secure Mosquitto (recommended)
Many tutorials on running Mosquitto on a local network use username/password authentication 'in the open'. While this may be low'ish risk for a private network, most would prefer not to have authentication params flying around wirelessly in plain text. The steps below configure MQTT to use certificate authentication and TLS encryption. If this is configured, then you must also copy the certs into the firmware configuration file when building (more on this later). Node-Red also needs to have the cert files but this is relaively trivial to configure. 

* On the Node-Red/MQTT machine, download this script [generate-CA.sh](https://github.com/owntracks/tools/tree/master/TLS).
* generate-CA.sh without any arguments will generate the server certificates. Place them in /etc/mosquitto/certs.
* Run ```chmown -R mosquitto /etc/mosquitto/certs``` to ensure Mosquito can read the files.
* Edit /etc/mosquitto/conf.d/default.conf to point at the cert files and enable required certificates. An example is provided in this repo under Mosquitto.
* Restart Mosquitto using ```sudo service mosquitto restart```
* Generate the client certs by running ```generate-CA.sh client [name]```. Name can be anything but it must not be empty.
* ```ca.crt```, ```[name].key``` and ```[name].crt``` are needed for both Node-Red's MQTT configuration and the BSB firmware. More on this later.
    - Never share the crt or key files.

## Configuring Node-Red Certificates

* Open a browser and navigate to the IP address of your Node-Red instance on port 1880; i.e. http://192.168.0.10:1880.
* Import the Node-Red flow called Interfaces.json under NodeRed in this repository (ctrl-i). It should appear as below.

![BSB Config](screen_captures/configuration.png)

This flow handles three MQTT messages published by the BSB firmware; Console, Register and Online. The Console message will contain error and diagnostic information printed to Node-Red's debug console. Register is sent by the firmware at boot and Online is sent every 3 seconds to indecate the interface is live. This flow handles 4 interfaces to demonstrate how to configure multiple ESP32 boards. For now, lets configure just one. 

* Double click the yellow 'Hot Side' sswitch node and rename it to match your DEVICE_NAME set in Config.h.
* Click the ellipse ```...``` under Rules and change the label ```Hot Side``` to match DEVICE_NAME as well.
    - For now, leave the JSON as is. This is discussed later.
* Commit those changes and double click the ```Which Device?``` node, replacing ```Hot Side``` with DEVICE_NAME.
* Do the same step for the ```Online``` node.
* Finally, change the name of the orange timer node ```Hot Side``` to match DEVICE_NAME for cleanliness.

If using secure MQTTT, follow the additional steps below.

* Double click one of the MQTT message nodes in the flow.
* Click the pencil icon next to the Server field.
* Click the pencil icon netx to TLS Configuration.
* Enter the paths of your ```client``` ca.cert, key and crt files.

Now click the red ```deploy``` button to publish the changes. Assuming MQTT is configured correctly, the purple MQTT nodes should show a connected state. If not, double click one of them and review the settings. Apply power to your ESP32 interface that's running the BSB firmware. Within a few seconds, the orange timer node you configured should show a green state and begin counting down from 5 while resetting every 3 seconds. You can further customize this flow to match the number of devices you'll actually be using. 

Lastly, navigate to your Node-Red instance's Dashbaord by appendding ```/ui``` to the address; e.g. ```http://192.168.0.10:1880/ui```. You should see an Interfaces tab reflecting the online state of your ESP32 baord. Congrats, you have a working Blue Screen Brewery platform! Next, we'll cover how to configure the interface using the JSON in the yellow switch node we edited above.

## Building the BSB Firmware
The firmware has only been tested on an ESP32 board and the 'helper' script referenced below assumes such a device.

* Download arduino-cli from https://arduino.github.io/arduino-cli/0.20/installation/
* On the command line, navigate to Firmware_ESP32 and copy arduino-cli here
* Edit the following values in Firmware_ESP32/Config.h.
    - DEVICE_NAME to uniquely identify the ESP32 board.
    - WiFi name and password.
    - If secure MQTT is used, the content of the cert files created above can simply be pasted into the matching char arrays.
* Run `arduino-cli board list` with the board plugged in and unplugged to determine the port.
* On Windows, edit build.bat to set the port and then run built.bat from within Firmware_ESP32
* For other operating systems, use build.bat as a reference for what commands to run.
