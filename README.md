# U2U (UART-to-UART) Messaging Network Library

## Overview

U2U is a lightweight messaging library designed for creating a flexible, embedded system network using UART communication across various hardware platforms including RP2040 (PICO), Raspberry Pi, and ESP32 devices.

## Key Features

- **Flexible Network Topology**: Supports daisy-chained UART connections resembling a linked list
- **Topic-Oriented Messaging**: Messages filtered by topics with configurable responses
- **Multi-Platform Support**: Works across RP2040, Raspberry Pi, and ESP32
- **Robust Message Handling**:
  - Unicast and broadcast messaging
  - Message forwarding
  - CRC validation
  - Configurable response behaviors

## Message Structure

Each message follows this format:

::SENDER:RECEIVER:R-FLAG:TOPIC:CHAPTER:LENGTH:PAYLOAD:HOPCOUNT:CRC:


### Message Segments Explained

| Segment | Description |
|---------|-------------|
| SENDER | Unique identifier for the sending device |
| RECEIVER | Destination device (or 'GEN' for broadcast) |
| R-FLAG | Response behavior type |
| TOPIC | Message category/type |
| CHAPTER | Message sequence (0-9) |
| LENGTH | Payload character count |
| PAYLOAD | Actual message data |
| HOPCOUNT | Number of device hops |
| CRC | Cyclic Redundancy Check |

### R-Flag Types

- `RQ`: Request response
- `RS`: Responding to a request
- `RI`: Ignore (forward if not direct recipient)
- `RN`: Not acknowledged (CRC mismatch)

## Project Structure

- `u2u.h` / `u2u.c`: Core messaging protocol implementation
- `u2uclientdef.h`: Device-specific configuration
- `u2uclientdef.c`: Network configuration (IP addresses for Linux)
- `u2u_HAL**.c` / `u2u_HAL**.h`: Hardware abstraction layer

## Hardware Configuration

- Supports devices with 1-2 UART interfaces
- Uses straight-through cabling with crossover on uart0
- Single-interface boards can terminate or bridge the network

## Wireless Extension

- Linux driver supports WebSocket for wireless capabilities
- Changes network topology from wired to mixed

## Current Limitations

- No dynamic ARP-like IP address discovery
- Character-based string comparisons (future: hash-table)

## Roadmap

- [x] Null-character independent functions
- [x] Hashing for topic/sender comparisons
- [x] Port extensions to 3 to accomodate ESP32 (two UARTs and wifi)
- [ ] Bluetooth bridging support

## Getting Started

1. Configure `u2uclientdef.h` with your device settings
2. Initialize messaging protocol in `main.cxx`
3. Use `get_message()` to receive and process messages
4. Implement topic-specific responses as needed

## Example Usage

```c
// Basic message sending
struct Message msg;
msg.sender = "DEVICE1";
msg.receiver = "GEN";
msg.r_flag = "RQ";
msg.topic = "SENSOR";
msg.payload = "TEMPERATURE_REQUEST";
send_message(&msg);

// Message processing
struct Message* received = get_message();
if (received != NULL) {
    // Process received message
    process_message(received);
}
