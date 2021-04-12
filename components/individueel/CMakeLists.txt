set(COMPONENT_ADD_INCLUDEDIRS include)
set(COMPONENT_SRCS "individueel.c")
set(COMPONENT_PRIV_REQUIRES esp32-i2c-lcd1602 
                            esp32-smbus 
                            esp32-i2c-rotaryEncoder
                            )
register_component()