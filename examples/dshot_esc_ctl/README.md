# RMT Transmit Example -- Dshot ESC control

## How to Use Example

### Configure the Project

Open the project configuration menu (`idf.py menuconfig`). 

In the `Example Configuration` menu:

* Set the GPIO number used for transmitting the IR signal under `RMT TX GPIO` option.
* Set the number of LEDs in a strip under `Number of LEDS in a strip` option.

### Build and Flash

Run `idf.py -p PORT flash monitor` to build, flash and monitor the project.

(To exit the serial monitor, type ``Ctrl-]``.)
