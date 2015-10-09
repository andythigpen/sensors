# My MySensors Sensors...
Arduino code and supporting files for my [MySensors](http://mysensors.org) sensors (say that 5x fast).

## How the sensors are used

Each sensor node connects wirelessly to a MySensors Serial Gateway attached to a local install of [Home Assistant](https://home-assistant.io) (seen below).  
![Home Assistant](images/home-assistant.png?raw=true)

The PIR motion sensor is used to determine presence in a particular room, and the photocell light sensor is used to determine if the lights need to turn off or on based upon the ambient light level in the room and the current setting for the given time of day.  The state of the rooms and household is managed using a [custom Home Assistant module](https://github.com/andythigpen/hass-config/blob/master/custom_components/myhome.py).

## A Finished Sensor Node

### Front View
![Sensor Front](images/sensor_front.jpg?raw=true)

_Picture taken with a potato._

It's difficult to tell the size, but the project box is roughly the height and width of a pack of cards.

### Side View
![Sensor Side](images/sensor_side.jpg?raw=true)

_Cutting square holes is harder than it looks._

### Guts View
![Sensor Guts](images/sensor_guts.jpg?raw=true)

_It's not pretty but it works._

The battery holder is hot-glued to the lid, and the PIR sensor and power switch are fastened to the box using plastic screws/nuts.  The Arduino and nRF24 radio are not currently permanently attached as there isn't enough room inside the case to move around anyway.

### Schematic
![Sensor Guts](schematic/schematic.png?raw=true)

## Project Layout
* images/ - pictures of a finished sensor node
* parts/ - CAD drawings of parts and layouts
* schematic/ - schematic files
* src/ - firmware source code
