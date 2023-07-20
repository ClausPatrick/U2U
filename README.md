# U2U
UART to UART messaging

A library to manage messages on a simple network containing various embedded systems containing RP2040 (PICO) and Raspberry Pi's.
Messages contain various fields with the intention to utilise features present on other boards.
They can be unicast (using a unique NAME) or broadcast and are filtered on TOPIC and provided specific hardware is in use on the network, topic specific responses are generated.
Messages are being forwarded if broadcast or if NAME does not correspond to the receiving device. Since the RP2040 has two UART, the network is basically a daisy chain of uart0 to uart1 connections.
Hardware wise I decided to use straight through cabling and crossover the connections on uart0 on the PCB itself.
The main.cxx file should contain the initalisation of the messaging protocol and for now at least the forever while loop should check the message counter and invoke the message_handler() in case the count went up.
Pin definitions pertaining to the UARTs as well as supported functions and their response strings are defined in u2uclientdef.c and obviously vary across the different boards.

Running topic oriented messaging service is not trivial and the purpose is to implement this without too much overhead in the users main program. Certain hardware implementations allow for data transfers (reading out sensor
information, reading in display information) but in a way that doesn't require the main program to handle the transmission services.
Each unit can have 1 or 2 UART interfaces so boards are interconnected either via their single interface or daisy chained through via both interfaces. In the case of two interfaces, one will be designated IN and the other OUT. This allow for a network topology that resembles a linked list data structure
(graph topology with one vertrex per node) whereas the single interface boards terminate the network.

Each message contains several segments, bound by the desginator ':'. As the payload-length is variable one segment is dedicated to indicate the length of the payload and has a maximum length enforced by const int MAX_PAYLOAD_LENGTH.
Here is a message outlined:
::SENDER:RECEIVER:RQFLAG:TOPIC:CHAPTER:LENGTH:PAYLOAD:HOPCOUNT:CRC:
A message start is indicated by a double colon '::'. This has two functions: it allows syncing of start messages and recognise if a faulty message is received. In many cases if the sender messed up several segments are rendered empty
and the receiver will wait for a correct start.
Values for SENDER and RECEIVER are designated strings the user sets for each board. In order to send messages to all units receiver name GEN (for general) can be used.
RQ flags determine response behaviour:
RQ: request - sender requests response.
RS: response - sender is responding to a request.
RI: ignore - no response is required but message will be forwarded if the receiver is not named in RECEIVER.
NA: not acknowledged - receiver calculated a different CRC value than was indicated in CRC segment but receiver will respond to message even if it is not addressed in RECEIVER. This ensures the sender is made aware of the error
without network overhead.
TOPIC fields are preset and the specific board can implement its custom response.
CHAPTER is analogue to sequences in TCP and indicates order. This will be ascii values {0-9}.
LENGTH is indicating the character count of the PAYLOAD. This will be ascii values {0-9}.
PAYLOAD is the data to be sent. Beside the MAX_PAYLOAD_LENGTH there is no limitation on the length or the character it contains, even (double)colons.
HOPCOUNT keeps track of how many units have forwarded the message. Each forwarding will increment this value. This will be ascii values {0-9}.
CRC for crc value. This will be ascii values {0-9}.

u2u.h / u2u.c
There are two main tasks that are executed to handle inbound and outbound messages.
Inbound: Messages are being parsed on character basis and written into the correct field. Once completed and verified it will then be routed. Depending on the message contant it can be forwarded or responded to.
Outbound: struct Message can be filled in with values in order to send this message over the lines.

u2uclientdef.h
User specific profile such as SENDER name and strings for responses as well as pin settings for UART.

u2u_HAL**.c u2u_HAL**.h
Hardware implementation. If interrupt routines are used, they are defined and implemented here.



File structure

-main.c/pp
    Includes:   u2u.h
    Declares:   struct Message
    Invokes:    message_setup()
                send_message(&Message)
                get_message()

-u2u.h
    Includes:   u2u_HAL_~.h
                u2uclientdef.h

    Exposes:    uartN_character_processor()
                send_message(struct message)

    Invokes:    write_from_uartN(str)
                u2u_uart_setup()
-u2u.c
    Defines:    struct Message

-u2u_HAL_~.h




As mentioned before, message fields are seperated by a ':' and consists of the following:
:sender:        - Denoting senders unique name as 16 or less ASCII characters.
:receiver:      - Containing up to 16 ASCII characters denoting a specific device name OR 'GEN' for general call which requests a response from all connected devices.
:RQS:       - Flags {RQ, RS, RI, NA}, denoting the nature of response, where
        RQ: request (expecting response),
        RS: response (not expecting response),
        RI: inform (not expecting response),
        NA: not acknowledged.

:topic:     - Specifies the functionality or feature this message is trying to access. Some topics implemented so far are:
        GET_SENSOR: combined with the receivers name it expects sensor readings from a specific device. Combined with receiver :GEN: it exects to gather sensor data from all connected devices.
        SET_LED: combined with the receivers name it expects to change LEDs. The message payload further specifies what behaviour is expected.
        SET_LCD: combined with the receivers name it expects to print to LCD. The message payload provides the string expected to be printed.

:chapter:   - ASCII form of an integer that is incremenated on each specific receiver/topic combination. Purpose is to sequence the messages in the right order.

:payload length:
            - ASCII form of an integer specifying the payload proper.

:payload:   - String of length :payload length: of arbritary characters, including non-ASCII characters.

:hopcount:  - Each node in the network receiving and forwarding the message will increase this ASCII form of intiger.

:CRC:       - Cyclic Redundancy Check.

The purpose of this explicit specifying various elements such as payload length is to be able transfer a payload that has no limitation in terms of characters.
It should be able to handle CR and LF within the payload without changning the message state.

![20230130_110412](https://user-images.githubusercontent.com/44665589/215461139-caf5ce9b-936c-46aa-9414-bc58e963a222.jpg)
