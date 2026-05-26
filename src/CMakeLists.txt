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
