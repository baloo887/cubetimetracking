# cubetimetracking
Arduino sketch for an accelerometer based switch.

The code is still in a prototype-like status. It needs to be cleaned an to be splitted in differet classes.

This switch is design to be placed inside a cube shaped box, and after the cube is rotated and placed down on a differnt face a http request is performed.

in the current configuration the switch is powered by a 500mAh battery that grants 10 hours operativity

At the moment is being used as a time tracking tool, in which each face represents an activity.

Circuit used for this prototype:
![alt text](docs/circuit.png?raw=true "circuit")

Part list
- adxl345 accellerometer
- WeMos d1 wifi prototyping board
- RGB led
- 2x 4.7Kohm resistors
- 500mAh battery 
