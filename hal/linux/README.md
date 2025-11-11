# U2U Linux HAL (Hardware Abstraction Layer)

## Overview

This Linux Hardware Abstraction Layer (HAL) provides the implementation for network communication using both serial (UART) and socket interfaces, enabling seamless integration of Linux devices into the U2U messaging network.

## Key Features

- **Dual Communication Interfaces**
  - Serial port (UART) communication
  - Socket-based network communication
- **Multithreaded Design**
  - Separate threads for serial and socket input
  - Concurrent message processing
- **Flexible Port Configuration**
  - Enable/disable ports dynamically
  - Support for inbound and outbound communication

## Architecture

### Communication Interfaces

| Interface | Type | Description |
|-----------|------|-------------|
| UART0 | Serial | Local serial port communication |
| UART1 | Socket | Network-based communication |

### Threading Model

- `serial_in_task()`: Handles serial port input
- `socket_in_task()`: Manages network socket connections
- Mutex-protected state management
- Fork-based connection handling for sockets

## Configuration

### Port Configuration

Ports can be configured using `set_port()`:
```c
int set_port(int port, bool dir, bool state);
// port: 0 (UART0) or 1 (UART1)
// dir: 0 (inbound), 1 (outbound)
// state: true (enable), false (disable)
```
### Peer Management

-Supports multiple peer IP addresses
-Configurable peer list
-IP address comparison utilities

## Communication Functions

Function	Description
-write_from_uart0()	Write to serial port
-write_from_uart1()	Broadcast to socket peers
-write_from_uart()	Write to both interfaces

## Error Handling

-Mutex-based thread synchronization
-Error logging
-Graceful thread termination

## Logging

## Supports communication logging with detailed flags:

-Port identification
-Communication direction
-Message origin
-Message type

## Limitations

-Fixed number of peers
-No dynamic peer discovery
-Requires manual IP configuration

## Build Requirements

-Linux environment
-POSIX threads (pthread)
-Standard system libraries
-GCC compiler


## Roadmap

-Dynamic peer discovery
-Enhanced error handling
-More robust logging mechanism
-IPv6 support

## Contributing

Contributions are welcome! Please submit pull requests or open issues.
