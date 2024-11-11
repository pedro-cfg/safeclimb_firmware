# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/pedro/Documentos/ESP_IDE/esp-idf-v5.3.1/components/bootloader/subproject"
  "/home/pedro/ESP_IDE/safeclimb/build/bootloader"
  "/home/pedro/ESP_IDE/safeclimb/build/bootloader-prefix"
  "/home/pedro/ESP_IDE/safeclimb/build/bootloader-prefix/tmp"
  "/home/pedro/ESP_IDE/safeclimb/build/bootloader-prefix/src/bootloader-stamp"
  "/home/pedro/ESP_IDE/safeclimb/build/bootloader-prefix/src"
  "/home/pedro/ESP_IDE/safeclimb/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/pedro/ESP_IDE/safeclimb/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/pedro/ESP_IDE/safeclimb/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
