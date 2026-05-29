add_executable(task app.cpp)
target_compile_features(task PRIVATE cxx_std_20)
target_link_libraries(task PRIVATE config tape-emulator utils)
