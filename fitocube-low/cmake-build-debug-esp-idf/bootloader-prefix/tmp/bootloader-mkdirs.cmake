# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/Users/montana/MyLabs/ESP-IDF/esp-idf-v5.5.1/components/bootloader/subproject"
  "/Users/montana/MyLabs/embedded-systems/fitocube-low/cmake-build-debug-esp-idf/bootloader"
  "/Users/montana/MyLabs/embedded-systems/fitocube-low/cmake-build-debug-esp-idf/bootloader-prefix"
  "/Users/montana/MyLabs/embedded-systems/fitocube-low/cmake-build-debug-esp-idf/bootloader-prefix/tmp"
  "/Users/montana/MyLabs/embedded-systems/fitocube-low/cmake-build-debug-esp-idf/bootloader-prefix/src/bootloader-stamp"
  "/Users/montana/MyLabs/embedded-systems/fitocube-low/cmake-build-debug-esp-idf/bootloader-prefix/src"
  "/Users/montana/MyLabs/embedded-systems/fitocube-low/cmake-build-debug-esp-idf/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/montana/MyLabs/embedded-systems/fitocube-low/cmake-build-debug-esp-idf/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/montana/MyLabs/embedded-systems/fitocube-low/cmake-build-debug-esp-idf/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
