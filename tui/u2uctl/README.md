# U2U Control Centre (u2uctl.py)

Interactive REPL for managing embedded nodes, composing and sending messages, querying message history, and orchestrating local/remote target services.

This control centre is an operator-facing CLI for a distributed U2U messaging system. It integrates three main components (imported modules):

- `u2u_messaging.Message` — message data structure  
- `data_processing.Data_Processor` — SQLite-backed message ingestion and queries  
- `target_service.Target` — interface to the hardware target (C HAL, systemd-managed)

---

## Overview

`u2uctl.py` provides an interactive prompt (built with `prompt_toolkit`) that lets an operator:

- Enable/disable local and remote target services
- Compose, store, load, and send structured messages
- Use pre-built templates to quickly send common messages
- Query and inspect an SQLite database of recorded messages
- Monitor responses to previously sent messages
- Persist message drafts to disk
- Inspect U2U network topology

The REPL reflects the target state in its prompt (active/inactive) and shows a concise HUD for status and context-sensitive prompts.

---

## Quick Start

1. Ensure supporting packages and modules are available:
   - Python 3.8+ and installed packages (see pip_requirements.txt)
   - Your local modules: `u2u_messaging`, `data_processing`, `target_service`
   - u2u/ directory with relevant compiled programs for Linux.

2. Ensure `host_data.json` exists and contains remote host definitions.

3. Run:

    python3 u2uctl.py

---

## High-Level Architecture


```
u2uctl.py (REPL)
├─ Message (u2u_messaging)
├─ Data_Processor (data_processing)
│ ├─ process_messages() (ingest new DB entries)
│ └─ query(sql) (ad-hoc DB queries)
└─ Target (target_service)
├─ enable_local()/disable_local()
├─ enable_remote()/disable_remote()
├─ send_message(message, port)
└─ host lists (remote_hostsnames, remote_hosts)

```

`u2uctl` maintains:
- in-memory message store (`stored_messages`) persisted to `message_store.bin`  
- sent message history (`sent_messages`) with timestamps  
- lists of known senders and receivers pulled from the DB  
- `modes` accessible from the `m` key (mode select)

---

## Main Commands (REPL)

When you are at the main prompt `U2U <state> cc#`:

- `list` — enter listing submenu (hosts / senders / receivers)
- `enable` — enable a local or remote target
- `enableall` — enable both local and remote
- `disable` — disable a local or remote target
- `disableall` — disable both local and remote
- `messages` — open message management submenu
- `options` — miscellaneous utilities (cleanup, etc.)
- `help` — prints help
- `exit` — quit the REPL

---

## Messages Submenu

Type `messages` at the main prompt to manage messages. Subcommands:

- `compose` — interactive message builder (receiver, r_flag, topic, chapter, payload, port)
- `load` — load a stored draft from `message_store.bin`
- `store` — store current composed message to disk
- `responses` — check for responses (RS r_flag) from nodes since last_check
- `templates` — quick-send templates (hail, sensor, timing)
- `query` — run ad-hoc SQL queries against the DB
- `exit` — return to main menu

---

## Message Composition Flow

`compose` collects segments in a guided flow. Example segments:

- `receiver` — select from known nodes or type a custom one  
- `r_flag` — request/response flag (e.g., `RQ`, `RS`)  
- `topic` — choose from a predefined topic list in `Message`  
- `chapter` — numeric chapter identifier  
- `payload` — free-form text (max length controlled by `MAX_MESSAGE_PAYLOAD_SIZE`)  
- `port` — `0`, `1`, or both (`0 & 1`)

Before sending, `_check_message_complete()` verifies all required fields and returns error codes, printing warnings for missing parts.

---

## Templates

`templates` provides quick, pre-defined message payloads. The message itself is formatted directly at the target (hal/linux/)

- `hail` — Sending 'hail' to all nodes (receiver: `GEN`)
- `sensor` — Requesting sensor data from all nodes.
- `timing` — timing/sync message to all nodes without response (r-flag: `RI`)

Each template is sent through `self.target.send_from_template(idx)` and can be adjusted in the `Target` module.

---

## Stored Messages

Composed messages can be saved to `message_store.bin` using pickle. Use `store` to save and `load` to retrieve. `do_load()` shows the indexed list and lets you pick a stored message to restore into the composer.

---

## Sending Messages & Responses

To send:

- Compose the message
- Call `do_send()` which:
  - Validates the message
  - Prompts for port (if not set)
  - Calls `target.send_message(message, port)` if the target is active
  - Logs the message into `sent_messages` with a timestamp

`do_check_responses()` queries the DB for rows meeting:
- `date > last_response_time`
- `receiver = local_hostname_upper`
- `r_flag = 'RS'`

It updates `last_response_time` if responses found and prints the returned rows.

---

## Node & Host Listing

`do_listhosts()` prints active/inactive markers for local and remote hosts:

- `o` (green) — active
- `x` (red) — inactive

The output lists local machine (marked `*self`) and remote hosts with their IPs.

---

## Helper & Utility Commands

- `enable_local()` / `disable_local()` — operate local HAL via `Target`  
- `enable_remote()` / `disable_remote()` — operate remote hosts via SSH through `Target`  
- `do_options()` → `cleanup` — invokes `target.pipe_cleaner()` for maintenance  
- `_validate_input()` — numeric range checker used by `chapter` input and random-entry flows

---

## Persistence & Files

- `host_data.json` — list of remote hosts / addresses loaded by `Target`  
- `message_store.bin` — pickled list of stored message drafts  
- SQLite DB managed by `Data_Processor` (schema is in that module)

---

## Example Session

```U2U inactive cc# enable
local
U2U active cc# messages
U2U active messages# compose
U2U format segment:# receiver
NODE_A
U2U format segment:# payload
Hello Node A
U2U format segment:# send
Sending message from port 0: <Message ...>, result: 0.
U2U active cc# messages
U2U active messages# responses
t: 2023-10-20 12:14:10, response: [{...}]

```


---

## Dependencies

- Python 3.8+
- `prompt_toolkit` (for REPL & autocompletion)
- `sqlite3` (standard library)
- `u2u_messaging` (local module)
- `data_processing` (local module; uses `sqlite3`)
- `target_service` (local module; interfaces with systemd C target)

System-level:
- systemd-managed C HAL (for local target)
- SSH access to remote systems (if using remote target management)

---

## Development Notes & Suggestions

- **Validation**: `_check_message_complete()` prints helpful error codes — consider raising exceptions or returning structured error objects in future refactors for testability.
- **Unit testing**: Add mocks for `Target` and `Data_Processor` to write unit tests for REPL logic and composition flows.
- **Persistence format**: Using `pickle` for `message_store.bin` is convenient but not portable — consider JSON for better cross-version compatibility.
- **Security**: Be cautious with `do_query()` — it executes raw SQL strings. Limit to read-only or sandboxed contexts, or provide parameterized interfaces. Only `SELECT` statements are supported.
- **Concurrency**: If `Data_Processor.process_messages()` can be long-running, consider running it in a worker thread or background job and notifying the UI when new messages arrive.
- **Logging**: Centralize logging configuration (file + stdout) with rotating logs for production.

---

## Future Work & Enhancements

- Add a `B/S` style rule parser for any automaton (if you integrate visualization).  
- Provide a web dashboard front-end for remote operators.  
- Add structured export (CSV) of DB query results.  
- Implement message templates editor inside the REPL.  
- Add stricter type annotations and mypy/codestyle checks.

---

## License

Choose your license (MIT is suggested for open source demos).

---

## Contact / Author Notes

This control centre grew from a systems integration exercise: combining a small C HAL with Python control tooling and a simple SQLite message store. It’s meant to be practical, debuggable, and extensible for low-footprint embedded networks.


