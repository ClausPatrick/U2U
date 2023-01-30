# U2U
UART to UART messaging

Customer library to manage messages on a simple network containing various embedded systems containing RP2040 (PICO) and Raspberry Pi's.
Messages contain various fields with the intention to utilise functions present on other boards. They can be unicast (using a unique NAME) or broadcast and are filtered on TOPIC and provided specific hardware is in use on the network, topic specific responses are generated. Messages are being forwarded if broadcast or if NAME does not correspond to the receiving device. Since the RP2040 has two UART, the network is basically a daisy chain of uart0 to uart1 connections. Hardware wise I decided to use straight through cabling and crossover the connections on uart0 on the PCB itself. 
The main.cxx file should contain the initalisation of the messaging protocol and for now at least the forever while loop should check the message counter and invoke the message_handler() in case the count went up.
Pin definitions pertaining to the UARTs as well as supported functions and their response strings are defined in u2uclientdef.c and obviously vary across the different boards.
Functions that are used additionally but are not UART specific are thrown into the u2u_aux.c file.
![20230130_110412](https://user-images.githubusercontent.com/44665589/215461139-caf5ce9b-936c-46aa-9414-bc58e963a222.jpg)
