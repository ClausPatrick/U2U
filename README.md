# U2U
UART to UART messaging

A library to manage messages on a simple network containing various embedded systems containing RP2040 (PICO) and Raspberry Pi's.
Messages contain various fields with the intention to utilise features present on other boards.
They can be unicast (using a unique NAME) or broadcast and are filtered on TOPIC and provided specific hardware is in use on the network, topic specific responses are generated.
Messages are being forwarded if broadcast or if NAME does not correspond to the receiving device. Since the RP2040 has two UART, the network is basically a daisy chain of uart0 to uart1 connections.
Hardware wise I decided to use straight through cabling and crossover the connections on uart0 on the PCB itself.
The main.cxx file should contain the initalisation of the messaging protocol and for now at least the forever while loop should check the message counter and invoke the message_handler() in case the count went up.
Pin definitions pertaining to the UARTs as well as supported functions and their response strings are defined in u2uclientdef.c and obviously vary across the different boards.

Message fields are seperated by a ':' and consists of the following:
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
