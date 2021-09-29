# Pulse Width Encoding Component

This directory contains a virtual driver for pulse-width-encoding pattern signal which is used by WS2812, SK6812, as well as Dshot protocol

```
Pulse Width Encoding
 |    T0H  T0L   T1H  T1L          TxH  TxL     TRST    TxH
 |    +--+     +-----+  +-- ...... --+     |            +--
 |    |  |     |     |  |            |     |            |   ......
 |  --+  +-----+     +--+            +------------------+
 +------------------------------------------------------------------->
```

Low level driver supports:

- SPI, send only
- RMT, send and recv
- I2S, TODO

Dshot is only supported by RMT backend due to resolution limitation

To learn more about how to use this component, please check API Documentation from header file [led_strip.h](./include/led_strip.h).
