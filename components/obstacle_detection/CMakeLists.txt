# Obstacle Detection Component CMakeLists.txt

idf_component_register(
    SRCS "src/obstacle_detection.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "src"
    REQUIRES vl53l0x
)
