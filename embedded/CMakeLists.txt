cmake_minimum_required(VERSION 3.28)

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# Imposta il percorso della sysroot
set(CMAKE_SYSROOT /opt/arm64-sysroot)

set (CMAKE_C_COMPILER aarch64-linux-gnu-gcc-12)
set (CMAKE_CXX_COMPILER aarch64-linux-gnu-g++-12)

set(CMAKE_FIND_ROOT_PATH ${CMAKE_SYSROOT})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

#set(CURL_INCLUDE_DIR "/usr/aarch64-linux-gnu/include")
set(CURL_INCLUDE_DIR "${CMAKE_SYSROOT}")
set(CURL_LIBRARY "{CMAKE_SYSROOT}")
find_package(CURL REQUIRED)

include_directories("${CMAKE_SYSROOT}/usr/local/include")

#nome del progetto e linguaggio
project(ProgTiro LANGUAGES C CXX) 

#aggiunge la libreria condivisa
add_library(MPU6050 SHARED src/MPU6050.cpp)

#aggiunge l'eseguibile
add_executable(Example src/Example.cpp src/DataSender.cpp)

target_link_libraries(Example PRIVATE MPU6050 curl i2c pthread dl)
