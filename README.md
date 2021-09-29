# Pulse Width Encoding Protocol

which is widely used on led strips.

Named by me and is not responsible for any thing -.-

## Backend difference

### RMT

- Very very accurate timing. 
- Currently cannot take use of DMA and is heavily depending on ISR routine, which makes it quite unstable when higher proority tasks are running (such as WiFi).
- Able to simulate Dshot timing.

### SPI

- Minimal step resolution is limited so that it is expected to have some timing difference between real output and desired output.

- Can take use of DMA and is reliable for great amount data to send.

### I2S

TODO

## Examples

There are currently two examples under the repo:

### Dshot ESC control

in `examples/dshot_esc_ctl`

Only supports RMT backend

### LED strip

in `examples/led_strip`

