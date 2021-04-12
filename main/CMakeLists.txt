set(COMPONENT_SRCS "main.c")
set(COMPONENT_ADD_INCLUDEDIRS .)
set(COMPONENT_REQUIRES  esp_peripherals 
                        nvs_flash 
                        esp32-smbus 
                        esp32-i2c-lcd1602 
                        esp32-i2c-rotaryEncoder 
                        wifi-connect
                        individueel
                        individueel-menu
                        )
register_component()