# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/caio/esp/esp-idf-v5.3.1/components/bootloader/subproject"
  "/home/caio/workspace/safeclimb_firmware/build/bootloader"
  "/home/caio/workspace/safeclimb_firmware/build/bootloader-prefix"
  "/home/caio/workspace/safeclimb_firmware/build/bootloader-prefix/tmp"
  "/home/caio/workspace/safeclimb_firmware/build/bootloader-prefix/src/bootloader-stamp"
  "/home/caio/workspace/safeclimb_firmware/build/bootloader-prefix/src"
  "/home/caio/workspace/safeclimb_firmware/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/caio/workspace/safeclimb_firmware/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/caio/workspace/safeclimb_firmware/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
