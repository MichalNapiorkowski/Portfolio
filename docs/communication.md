# Communication Model

## Modbus TCP layout

The prototype uses wired Ethernet and Modbus TCP. The PLC acts as a Modbus client. The embedded modules act as Modbus servers.

Two communication styles are used:

- cyclic read of status data, such as sensor states, motion state and homing state,
- task-based commands, where the PLC sends a task number and confirmation number, then waits for a result and returned confirmation number.

## Task exchange

1. The PLC writes a task number and a new confirmation number.
2. The module detects the new confirmation number.
3. The module performs the requested operation.
4. The module writes the result and the returned confirmation number.
5. The PLC checks whether the response belongs to the current task.

This mechanism prevents an old result from being treated as a response to a new command.

## Register concept

| Direction | Typical fields |
| --- | --- |
| Read by PLC | task result, returned confirmation number, module status, diagnostic bits |
| Written by PLC | task number, confirmation number, command parameters, reset or priority request |

## Timing

The prototype uses a conservative 100 ms communication cycle. During testing the selected timeout threshold was set with a margin large enough to avoid false faults, while still allowing fast detection of a missing response.
