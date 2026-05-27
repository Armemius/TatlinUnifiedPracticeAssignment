# Tape emulation library

file(GLOB TAPE_EMULATOR_SOURCES_LIST CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/tape/*.cpp")
file(GLOB TAPE_EMULATOR_HEADER_LIST CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/include/tape/*.hpp")
add_library(tape-emulator ${TAPE_EMULATOR_SOURCES_LIST} ${TAPE_EMULATOR_HEADER_LIST})
target_include_directories(tape-emulator PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_compile_features(tape-emulator PUBLIC cxx_std_20)

source_group(
    TREE "${PROJECT_SOURCE_DIR}/include"
    PREFIX "Tape emulator Header Files"
    FILES ${TAPE_EMULATOR_HEADER_LIST}
)
source_group(
    TREE "${PROJECT_SOURCE_DIR}/src"
    PREFIX "Tape emulator Source Files"
    FILES ${TAPE_EMULATOR_SOURCES_LIST}
)

# Utilites library

file(GLOB UTILITIES_SOURCES_LIST CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/utils/*.cpp")
file(GLOB UTILITIES_HEADER_LIST CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/include/utils/*.hpp")
add_library(utils ${UTILITIES_SOURCES_LIST} ${UTILITIES_HEADER_LIST})
target_include_directories(utils PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_compile_features(utils PUBLIC cxx_std_20)

source_group(
    TREE "${PROJECT_SOURCE_DIR}/include"
    PREFIX "Utilites Header Files"
    FILES ${UTILITIES_HEADER_LIST}
)
source_group(
    TREE "${PROJECT_SOURCE_DIR}/src"
    PREFIX "Utilites Source Files"
    FILES ${UTILITIES_SOURCES_LIST}
)

# Config library

FetchContent_Declare(
    tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG        v3.4.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(tomlplusplus)

file(GLOB CONFIG_SOURCES_LIST CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/config/*.cpp")
file(GLOB CONFIGS_HEADER_LIST CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/include/config/*.hpp")
add_library(config ${CONFIG_SOURCES_LIST} ${CONFIGS_HEADER_LIST})
target_include_directories(config PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(config PUBLIC tomlplusplus::tomlplusplus)
target_compile_features(config PUBLIC cxx_std_20)

source_group(
    TREE "${PROJECT_SOURCE_DIR}/include"
    PREFIX "Configuration Header Files"
    FILES ${CONFIGS_HEADER_LIST}
)
source_group(
    TREE "${PROJECT_SOURCE_DIR}/src"
    PREFIX "Configuration Source Files"
    FILES ${CONFIG_SOURCES_LIST}
)
