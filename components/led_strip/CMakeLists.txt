idf_component_register(SRCS "src/led_strip_pwe.c" "src/led_strip.c"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES "driver" "pulse-width-encoding"
                      )
