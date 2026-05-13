# UART-to-UART Messaging Protocol (U2U) - Version 5

A lightweight messaging protocol designed for embedded systems communicating over UART and extended to wireless-capable devices. The protocol supports structured messages, deterministic routing, and platform-independent parsing.

---

## Supported Platforms

- Raspberry Pi (Linux-based SBC)
- Raspberry Pi RP2040 (Pico)
- Espressif ESP32

---

## Overview

Originally designed for **UART daisy-chained networks**, the protocol has been extended to support **wireless communication** (ESP32 / Raspberry Pi).

Each node:
- Is connected to 2 (or 3 for ESP32) peers
- Parses incoming messages
- Decides to **respond**, **forward**, or **drop** messages based on routing rules

Messages carry metadata describing:
- Sender
- Receiver
- Routing intent (R-Flag)

---

## Physical Layer

- UART-based daisy-chain topology
- Optional wireless extension
- Each node acts as both:
  - Receiver
  - Router
  - (Optional) responder

---

## Data Layer

### Message Structure

A message is enclosed by colons:
```:0071S11R13F2T8C2P6H1Y3MNODE_SENDERNODE_RECEIVERRSGET_TEMP12Zone 40089:```
### Format Breakdown

| Component | Description |
|----------|-------------|
| `:` | Start delimiter |
| `0071` | Total message length |
| `S11` | Sender (length 11) |
| `R13` | Receiver (length 13) |
| `F2` | R-Flag |
| `T8` | Topic |
| `C2` | Chapter |
| `P6` | Payload |
| `H1` | Hops |
| `Y3` | CRC |
| `M` | Start of message body |

---

## Message Segments

| Flag | Segment   | Description                      | Optional |
|------|----------|----------------------------------|----------|
| S    | Sender   | Origin node                      | No       |
| R    | Receiver | Target node or `GEN` (broadcast) | No       |
| F    | R-Flag   | Routing behaviour                | No       |
| T    | Topic    | Command / request identifier     | Yes      |
| C    | Chapter  | Sequence grouping                | Yes      |
| P    | Payload  | Message data                     | No       |
| H    | Hops     | Forward counter                  | Yes      |
| Y    | CRC      | Integrity check                  | No       |

---

## Segment Details

### Sender
- Unique node identifier  
- Max length: `16`

### Receiver
- Specific node name or `GEN` (broadcast)

### R-Flag

| Value | Meaning                  |
|------|--------------------------|
| RQ   | Response requested       |
| RS   | Response                 |
| RI   | No response required     |
| RN   | Fault / invalid message  |

---

### Topic
- Fixed-length (8 chars)
- Used for routing logic via hashing
- Defines request/response semantics

---

### Chapter
- Up to 3 characters
- Used for sequencing multi-part messages

---

### Payload
- Variable size
- Contains application data

---

### Hops
- Incremented on each forward/response
- Used for topology inference and loop detection

---

### CRC
- 3-digit zero-padded value
- Calculated over message (excluding crc and final colon)
- Ensures message integrity

---

## Protocol Properties

- Self-describing message structure
- Flexible segment ordering (based on preamble)
- Optional segments supported
- Custom segments can be added if uniquely flagged
- No dynamic memory allocation required

---

## Routing Behaviour

Each node evaluates incoming messages and chooses an action. In the case of GEN calls (broadcasts), Respond and Forward are issued:

### Respond
Conditions:
- Receiver is `SELF` or `GEN`
- R-Flag = `RQ`

Result:
- Swap sender/receiver
- Set R-Flag = `RS`

---

### Forward
Conditions:
- Receiver ≠ SELF
- R-Flag ∈ {RQ, RI, RS, RN}

Special case:
- `GEN` messages are always forwarded

---

### Drop
Conditions include:
- Receiver = SELF and R-Flag = `RI`
- Invalid routing combinations

Illegal cases:
- Sender == Receiver (possible loop)
- `GEN` + `RS` (responses must not broadcast)
- `GEN` + `RN` (fault messages must not broadcast)

---

## Routing Summary

| Action | Receiver | R-Flag |
|--------|----------|--------|
| RESP   | GEN      | RQ     |
| RESP   | SELF     | RQ     |
| FORW   | GEN      | RQ     |
| FORW   | GEN      | RI     |
| FORW   | OTH      | RQ     |
| FORW   | OTH      | RI     |
| FORW   | OTH      | RS     |
| FORW   | OTH      | RN     |
| DROP   | SELF     | RI     |
| DROP   | GEN      | RS     |
| DROP   | SELF     | RS     |
| DROP   | GEN      | RN     |
| DROP   | SELF     | RN     |

---

## Code Structure

### Core
- `u2u.c`
- `u2u.h`

Responsibilities:
- Message parsing
- Routing logic
- Queue management

---

### Hardware Abstraction Layers (HAL)

| Platform | Module |
|----------|--------|
| Linux SBC | `u2u_HAL_lx` |
| RP2040 | `u2u_HAL_pico` |
| ESP32 | `u2u_HAL_esp` |

---

### Node profiles
- `u2u_client_profile.c`
- `u2u_client_profile.h`

Responsibilities:
Setting constant definitions that are specific to the node such as:
- `U2U_PLATFORM_CHANNELS`
Used to indicate the port capabilities (Linux: 1, PICO: 2, ESP32: 3).
- `SELF_NAME`
Sets the Node's name.
- `topic_default_responses`
Array of character arrays used to set the payload as a response to a specific topic.
- `UART_{R/T}X_{0/1/2}`
Defines the hardware pin (0, 1, or 2) to set the UART as (R)eceiver or (T)ransmit.

---

## API Overview

### Data Structures

- `struct Message`
- `struct Message_Segments`
- `struct U2U_Errors`

Design:
- Messages stored in stack-based queues
- No dynamic allocation
- Segment struct provides direct access to parsed fields

---

### Receiving Messages

- Messages stored in **per-port cyclic queues**
- Avoids race conditions

Function:
Message* get_message(Message_Segments* seg);
Returns:
- Pointer to message
- `NULL` if queue empty

---

### Sending Messages

#### Method 1: Structured API
format_message()
u2u_send_message()

Supports:
- String topics
- Integer-based topics (`topic_int`)

---

#### Method 2: Pipe Composer
pipe_message_composer(
"69 {SRFTCPH} {TEST_SNDR} {TEST_RCVR} {RI} {GET_ENCR} {0} {tzns} {309}"
);

Format:

| Segment | Meaning |
|--------|--------|
| `{SRFTCPH}` | Preamble |
| `{TEST_SNDR}` | Sender |
| `{TEST_RCVR}` | Receiver |
| `{RI}` | R-Flag |
| `{GET_ENCR}` | Topic |
| `{0}` | Chapter |
| `{tzns}` | Payload |
| `{309}` | Hops |

Notes:
- Sender and Hops can be auto-filled
- `{}` currently used as delimiters (limitation for payload)

---

## Error Handling

Errors per message are reported via an enum:

```c
CRC_HAN_ERR = 1   // CRC mismatch
LEN_HAN_ERR = 2   // Length mismatch
RFL_HAN_ERR = 4   // Invalid R-flag prefix
RFS_HAN_ERR = 8   // Invalid R-flag value
GRS_ROU_ERR = 16  // GEN + RS invalid
GSC_ROU_ERR = 32  // Self-loop detected
GRN_ROU_ERR = 64  // Invalid R-flag
```

Errors in general are tallied in `U2U_Errors` struct where each member is indexed per port. Function `get_u2u_errors()` will outline all occurred errors and 
write it in a buffer for reference. 

## Design Notes
Fully deterministic routing
No heap usage → predictable embedded behaviour
Extendable segment system
Designed for constrained environments
