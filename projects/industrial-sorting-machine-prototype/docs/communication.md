# Communication Model

## Modbus TCP layout

The prototype uses wired Ethernet and Modbus TCP. The PLC acts as a Modbus client. The embedded modules act as Modbus servers.

Two data paths are used:

- cyclic reads for status data, such as sensor states, motion state and homing state,
- task commands, where the PLC sends a task number and confirmation number, then waits for the result and returned confirmation number.

## Task exchange

1. PLC writes a task number and a new confirmation number.
2. The module detects the changed confirmation number.
3. The module runs the requested operation.
4. The module writes the result and returned confirmation number.
5. PLC checks whether the response belongs to the current task.

The confirmation number prevents an old result from being accepted as a response to a new task.

## Register concept

| Direction | Typical fields |
| --- | --- |
| Read by PLC | task result, returned confirmation number, module status, diagnostic bits |
| Written by PLC | task number, confirmation number, command parameters, reset or priority request |

## Timing

The prototype uses a conservative 100 ms communication cycle. The timeout threshold was set with enough margin to avoid false faults, but still short enough to detect a missing response quickly.
