# VL53L0X Component CMakeLists.txt

set(ST_API_DIR "${COMPONENT_DIR}/st_api")

# ST Core source files
set(ST_CORE_SRCS
    "${ST_API_DIR}/core/src/vl53l0x_api.c"
    "${ST_API_DIR}/core/src/vl53l0x_api_calibration.c"
    "${ST_API_DIR}/core/src/vl53l0x_api_core.c"
    "${ST_API_DIR}/core/src/vl53l0x_api_ranging.c"
    "${ST_API_DIR}/core/src/vl53l0x_api_strings.c"
)

# Component sources
set(COMPONENT_SRCS
    "src/vl53l0x_driver.c"
    "src/vl53l0x_platform_esp32.c"
    ${ST_CORE_SRCS}
)

# Register component
idf_component_register(
    SRCS ${COMPONENT_SRCS}
    INCLUDE_DIRS 
        "include"
        "${ST_API_DIR}/core/inc"
        "${ST_API_DIR}/platform/inc"
    PRIV_INCLUDE_DIRS
        "src"
    REQUIRES
        driver
)

# Disable warnings for ST library files
set_source_files_properties(
    ${ST_CORE_SRCS}
    PROPERTIES COMPILE_FLAGS 
    "-Wno-error=maybe-uninitialized -Wno-error=absolute-value -Wno-maybe-uninitialized -Wno-absolute-value"
)
