set(srcs "src/pwe.c"
    "src/pwe_io_spi.c"
    "src/pwe_io_rmt.c"
    )
set(include "include")

idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS ${include}
                       REQUIRES "driver"
                      )
