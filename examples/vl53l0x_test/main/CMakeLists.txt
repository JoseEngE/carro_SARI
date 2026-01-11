# VL53L0X Test Example CMakeLists.txt

idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES vl53l0x
)
