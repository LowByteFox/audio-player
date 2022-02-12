cmake_minimum_required(VERSION 3.0.0)
project(audio-player VERSION 0.1.0)

include_directories(${/usr/include/})
include_directories(${/usr/local/include/})
link_directories(${/usr/include/})
link_directories(${/usr/local/include})

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

include_directories(${GTK3_INCLUDE_DIRS})
link_directories(${GTK3_LIBRARY_DIRS})

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lpthread")

add_definitions(${GTK3_CFLAGS_OTHER})

include(CTest)
enable_testing()

add_executable(audio-player main.c)
target_link_libraries(audio-player ${GTK3_LIBRARIES})
target_link_libraries(audio-player raylib)
TARGET_LINK_LIBRARIES(audio-player libpthread.so.0)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

set(EXECUTABLE_OUTPUT_PATH "../")
