cmake_minimum_required(VERSION 3.10)
project(qhsjf_lab C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

include_directories(include)

if(MSVC)
    add_compile_options(/W4 /O2)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -O2)
endif()

# Core simulation library
add_library(qhsjf_core STATIC
    src/sim.c
    src/workload.c
    src/tests.c
)

# Test runner executable
add_executable(test_runner
    src/main_test.c
)
target_link_libraries(test_runner PRIVATE qhsjf_core)
if(NOT WIN32)
    target_link_libraries(test_runner PRIVATE m)
endif()

# HTTP server executable
add_executable(qhsjf-server
    src/server.c
)
target_link_libraries(qhsjf-server PRIVATE qhsjf_core)
if(WIN32)
    target_link_libraries(qhsjf-server PRIVATE ws2_32)
else()
    target_link_libraries(qhsjf-server PRIVATE m)
endif()

# Enable CTest
enable_testing()
add_test(NAME PaperTestSuite COMMAND test_runner)
