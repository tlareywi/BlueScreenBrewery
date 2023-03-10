# Example Flows
A set of flows demonstrating how to integrate various types of brewery hardware devices and external software suites. Use them as a starting point and customize for your own specific needs. Example configuration JSON that accompanies each example flow is also documented here. The ```Interfaace``` flow is recommended for everyone since it demonstrates how to integrate Node-Red with the BSB firmware.

## Atlas-pH
This flow is split into 3 distinct parts. First, an injection node sends the current mash temp every few seconds for pH temperature compensation. Secondly, three UI buttons are defined to send calibration messages for 3-point calibration. Finally, it listens for the pH readings and prints them to a text field. 

```javascript
 {
    "Type": "Atlas",
    "Rx": 26,
    "Tx": 27,
    "Topic": "BSB/Mash-pH",
    "Command": "BSB/Command-pH"
}
```
## Interfaces
Responds to BSB firmware configure messages with JSON that tells each Arduino board how to configure its pins. See the main README for additional details and JSON documentation.

## Unitank-Temp-Control
Controls a heater and pump attached to a Unitank, such as those in an SS Brewtech FTSs bundle. It defines two UI fields for a temperature ceiling and floor and listens for current temperature messages. Pump and heater state messages are sent depending on these values and the current temp. Additional UI elements are defined to show current heater and pump state + current temperature.

```javascript
{
    {
        "Topic": "BSB/Unitank-1-Temp",
        "Index": 1,
        "Type": "Onewire"
    },
    {
        "Topic": "BSB/Unitank-1-Cool",
        "GPIO": 25,
        "Type": "Digital-Out"
    },
    {
        "Topic": "BSB/Unitank-1-Heat",
        "GPIO": 27,
        "Type": "Digital-Out"
    }
}
```