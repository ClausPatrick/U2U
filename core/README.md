# UART-to-UART Messaging Protocol (U2U) - Version 5 
## Breaking compatibility
The message structure is fundamentally changed by using a preamble instead of seperating segments with a colon. The segments and their functions remain the same.
The preamble allows the message to be processed in any order of the segments. Their length is still constrained by the same constant definitions.
The node specific data is now set in `u2u_client_profile.*`

## Routing protocol
The behaviour based on Receiver and R-Flag remains the same.

## Topic responses
The behaviour is based now on the character array set in `u2u_client_profile.c` but the functionality remains the same.# UART-to-UART Messaging Protocol (U2U)

## Function structure changes
Before I could get away with parsing/routing being done in the interrupt routines (on the microcontrollers) but this is no longer feasible and predictably will cause issues with 
heavy network traffic. Now, the received bytes land in a queue and the function `get_message()` is doing the heavy lifting in triggering parsing/routing sequences in defined chunks. 
The downside is that now a node's function in the network in regards to responses and forwarding depends on `get_message()`  being called in loop/thread.
