# U2U core functions

## Core files
```
src
├── c_logger.c          Non essential
├── c_logger.h          Non essential
├── hardware            Non functional
│   ├── gpio.c          Non functional
│   ├── gpio.h          Non functional
│   ├── irq.h           Non functional
│   ├── uart.c          Non functional
│   └── uart.h          Non functional
├── main.cpp            Critical
├── pico
│   └── stdlib.h        Non functional
├── u2u.c               Critical
├── u2uclientdef.c      Critical
├── u2uclientdef.h      Critical
├── u2u.h               Critical
├── u2u_HAL_lx.c        Non functional
├── u2u_HAL_lx.h        Non functional
├── u2u_HAL_pico.c      Non functional
└── u2u_HAL_pico.h      Non functional
```
Files flagged as 'Non functional' are for compatiblity reasons. The compilation process should run the same as when those actually are functional.
Files flagged as 'Non essential' are helper files (logging) and are not part of the core functionality of the ultimate compilation.

## Running core file
Main.cpp function compiles to u2u_tester and takes arguments as indicated below:
Usage: ./u2u_tester <mode> <port> [additional arguments]

Modes:
  0 - Send Test
       Usage: ./u2u_tester 0 <port> <arg1> <arg2> <arg3> <arg4> <arg5>
       Example: ./u2u_tester 0 1 RECEIVER RQS TOPIC CHAPTER PAYLOAD

  1 - Receive Test
       Usage: ./u2u_tester 1 <port> <arg1>
       Example: ./u2u_tester 1 0 MESSAGE (Formatted as per U2U standard.)

  2 - Self Test
       Usage: ./u2u_tester 2 <port>
       Example: ./u2u_tester 2 1

Port values:
  0 - Port 0
  1 - Port 1



## Testing
### General
Test script outputs u2u_core_tester.log file in same directory as script. It takes '-v' or '--verbose' option flag(not by default). 

### Dependencies
To have a central file containing all files and their paths path_file.py is used. All its components are below:

### Lists
sender_list: containing all possible senders to test. Minimum 2
receiver_list containing all possible receivers to test. Minimum 2
topic_list: containing all default topics to test. These are restricted to eight character strings. 
rqs_list: Full list of RQ flags.
rqs_list_norm: Subset of RQ flags, excluding flags to indicate abnormal communication.
rqs_list_tx: Subset of RQ flags used by senders.
port_list: U2U Version 3 uses two ports; these are indicated here with 0 and 1.

### Paths
executable_path: path to compiled core file.
message_path: with in the 'logs' directory where the compiled core file runs. 
log_path: Expects a list of two paths; pointing to uartN_tester_log.txt, where N is port number.
random_payload_path: File containing lines for some random payloads to test.
crc_executable_path: compiled crc testing program.
test_log_path: subdirectory within the compiled file directories 'logs'
test_messages: Tester script will drop test messages here for the core file to parse and process.


### Checking context
Of interest is the class method context_tester() which takes number of trials as argument and belogs to Trial class. For each trial a message is placed in 'test_messages' (see Dependencies - Paths). 
When running the executable core file (see Dependencies - Paths for 'executable_path') the message is being read and parsed, this is done opting for the self_test chosen by the corresponding set of cli arguments (see Running core file). 
The executable will output  relevant information including the parsed and processed message as intended to be sent from respective ports and exits with a code that the test script evaluates. 
Said processed informations are found in 'log_path', (see Dependencies - Paths). 
The script will fetch the response information and performs the following checks on it:

- Consistancy of each present message:
    - No less than 11 segments are present
    - Each message segment has a lenght confined between minimum and maximum allowable lenght
    - Segment *LENGTH* contains value that corresponds to actual lenght of the *PAYLOAD*
    - CRC value contained in *CRC* is correct given the calculated CRC value on the message

- Routing:
    - What messages are found (0, 1 or 2)
    - In case of 0 messages, inbound message is flagged with 'RI', in which case no response is expected
    - There is be *no* response to message with RECEIVER 'GEN' and flag 'RI'
    - There is forwarding to message with RECEIVER 'GEN' and flag 'RI'

- Forwarding:
    - Same *SENDER* in both inbound and outbound message
    - Same *RECEIVER* in both inbound and outbound message
    - Same *RQS* in both inbound and outbound message
    - Same *TOPIC* in both inbound and outbound message
    - Same *CHAPTER* in both inbound and outbound message
    - Same *LENGTH* in both inbound and outbound message
    - Same *PAYLOAD* in both inbound and outbound message
    - Segment *HOP* has incremented (+1) on forwarding message
    - CRC has been checked already, see Consistancy of each present message

- Response
    - Inbound message *SENDER* is same as outbound message *RECEIVER*
    - Inbound message *RECEIVER* is either device test name or 'GEN'
    - Outbound message *RQ-flag* is 'RS
    - Same *TOPIC* in both inbound and outbound message
    - Same *CHAPTER* in both inbound and outbound message
    - Segment *HOP* has incremented (+1) on forwarding message
    - CRC has been checked already, see Consistancy of each present message


## Running the test script
Python script in core/u2u_v3/testing
python3 u2u_core_tester.py
-or:
./compile_and_test.sh [--upc N]
where N: {1|2|3}
Note: UPC 3 is only supported in Version 4.

## Version 4
### Port extension
To accomodate ESP32 with two UART ports and WIFI connection, three channels are accounted for with option UPC 3 in u2uclientdef.h.






