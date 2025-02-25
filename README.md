SAFECLIMB PROJECT

This is the firmware software for the Safeclimb project - A real time weather monitoring system

This software includes the code for the main, repeater and server towers.
You can choose the tower by changing the parameters in the "parameters.h" file.
In this file, there is three defines for the towers:

#define MAIN_TOWER  - 1 for the main tower, 0 else
#define SERVER_TOWER  - 1 for server tower, 0 else
#define TOWER_NUMBER - Defines the number of the tower in the hierarchy, from the main tower (greater number) to the server tower (zero)

For example, in the three tower scenario (main, repeater, server):

MAIN:
#define MAIN_TOWER  1
#define SERVER_TOWER 0
#define TOWER_NUMBER 2

REPEATER:
#define MAIN_TOWER  0
#define SERVER_TOWER 0
#define TOWER_NUMBER 1

SERVER:
#define MAIN_TOWER  0
#define SERVER_TOWER 1
#define TOWER_NUMBER 0

Other parameters can also be defined in this files, such as timeouts, LoRa parameters and stack size.

This project is made to be built using the Espressif IDE, availabe in:
https://github.com/espressif/idf-eclipse-plugin/blob/master/README.md
