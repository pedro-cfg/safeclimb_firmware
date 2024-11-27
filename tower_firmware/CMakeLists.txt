# Necessário para configurar a versão mínima do CMake
cmake_minimum_required(VERSION 3.16)

# Inclui o projeto do ESP-IDF
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Define o projeto
project(safeclimb)

# Especifica o padrão C para ser usado
set(CMAKE_C_STANDARD 99)