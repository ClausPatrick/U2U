# UART to UART messaging

## Introduction

A library to manage messages on a simple network containing various embedded systems containing RP2040 (PICO) and Raspberry Pi's. Messages contain various fields with the intention to utilise features present on other boards. They can be unicast (using a unique NAME) or broadcast and are filtered on TOPIC, and provided specific hardware is in use on the network, topic specific responses are generated. Messages are being forwarded if broadcast or if NAME does not correspond to the receiving device. Since the RP2040 has two UART, the network is basically a daisy chain of uart0 to uart1 connections (equivalent to a double linked list).

Hardware-wise I decided to use straight through cabling and crossover the connections on uart0 on the PCB itself. The main.cxx file should contain the initalisation of the messaging protocol and should check if get_message() does not return NULL. Non-NULL returns point to struct containing elements that were received from the messages as well as calculated values.

User settings in u2uclientprofile.* set up the unit’s unique profile and topic that are supposed to be responded to. Pin definitions pertaining to the UARTs as well as supported functions and their response strings are defined in u2uclientdef.c and obviously vary across the different boards.

The Linux driver uses websockets to extend the network, adding wireless capabilities but this also changes the network’s topology. The IP address for each unit is set in u2uclientdef.c.

Message

Running topic oriented messaging service is not trivial and the purpose is to implement this without too much overhead in the users main program. Certain hardware implementations allow for data transfers (reading out sensor information, reading in display information) but in a way that doesn't require the main program to handle the transmission services. Each unit can have 1 or 2 UART interfaces so boards are interconnected either via their single interface or daisy chained through via both interfaces. In the case of two interfaces, one will be designated IN and the other OUT. This allow for a network topology that resembles a linked list data structure (graph topology with one vertrex per node) whereas the single interface boards terminate the network but act as a bridge to extend it wireless.

Each message contains several segments, bound by the designator ':'. As the payload-length is variable one segment is dedicated to indicate the length of the payload and has a maximum length enforced by const int MAX_PAYLOAD_LENGTH.

Here is a message outlined:

::SENDER:RECEIVER:R-FLAG:TOPIC:CHAPTER:LENGTH:PAYLOAD:HOPCOUNT:CRC:

A message start is indicated by a double colon '::'. This has two functions: it allows syncing of start messages and recognise if a faulty message is received. In many cases if the sender messed up several segments are rendered empty and the receiver will wait for a correct start. Values for SENDER and RECEIVER are designated strings the user sets for each board. In order to send messages to all units receiver name GEN (for general) can be used.

R-Flags

This determines the response behaviour:

RQ: request - sender requests response.

RS: response - sender is responding to a request.

RI: ignore - no response is required but message will be forwarded if the receiver is not named in RECEIVER.

RN: not acknowledged - receiver calculated a different CRC value than was indicated in CRC segment but receiver will respond to message even if it is not addressed in RECEIVER. This ensures the sender is made aware of the error without network overhead.

Other message segments

TOPIC fields are preset in u2uclientdef.h and can be overwritten via exposed function u2u_topic_exchange() to allow for a custom response.

CHAPTER is analogue to sequences in TCP and indicates order. This will be ascii values {0-9}.

(payload)LENGTH is indicating the character count of the PAYLOAD. This will be ascii values {0-9}.

PAYLOAD is the data to be sent. Beside the MAX_PAYLOAD_LENGTH there is no limitation on the length or the character it contains, even (double)colons.

HOPCOUNT keeps track of how many units have forwarded the message. Each forwarding will increment this value. This will be ascii values {0-9}. CRC for crc value. This will be ascii values {0-9}.

u2u.h / u2u.c

There are two main tasks that are executed to handle inbound and outbound messages. Inbound: Messages are being parsed on character basis and written into the correct field. Once completed and verified it will then be routed. Depending on the message content it can be forwarded or responded to. Outbound: struct Message can be filled in with values in order to send this message over the lines.

u2uclientdef.h

User specific profile such as SENDER name and strings for responses as well as pin settings for UART.

u2uclientdef.c

For Linux devices, IP address lists of the network are listed here. There is no equivalent to ARP as of now that would allow the network to learn of its peer IP addresses.

u2u_HAL**.c u2u_HAL**.h Hardware implementation. If interrupt routines are used, they are defined and implemented here. For the Linux versions however phtreads are setup to listen / monitor both the serial port as well as the socket that has been bound to port / IP address.

Features -The used topology does not allow for message collisions on the UART wiring and the sockets are run via phtreads where processes are forked for each new connection to handle multiple connections seamlessly.

-Payload can contain any characters within a predetermined length, including NULL and line returns, etc. Although the segmentation character ‘:’ is used through the message to allow for dynamic parsing the character itself is ignored within the payload.

-the message parsing will reset if messages are not started with a ‘::’ (double colon) or if the R-flag field does not have ‘R’ as first character.

-CRC values are checked but the response behaviour is determined by user in u2uclientdef.h whether the get_message() allows for failed CRC messages to be passed on.

-for now, all segments are character compared by cmp_str(), but later versions shall implement a hash-table.

To do

[DONE]-Although string determination (NULL-character) has been added thorough the entirety of a message various function shall be made NULL-character independent. Since the length of each segment is known at each stage this feasible. These functions, not depending on the NULL, will be affixed with ‘_i()’.

[DONE]-Hashing function for topic / sender comparisons.

-Bluetooth bridging.
