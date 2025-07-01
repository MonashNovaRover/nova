# Generic Interfaces

This package contains a collection of lightweight, reusable message and service definitions for common ROS 2 use cases. 
These interfaces are designed to provide consistent patterns for publishing simple data types and requesting basic operations across systems.

Generic messages/service definitions only, if a message/service definition is for a specific use case please put it in [Nova Interfaces](../../nova_interfaces).

## Message Definitions

All message types follow a consistent structure:

- A `std_msgs/Header` field for timestamp and frame information.
- Typed data.

These are useful for minimal, timestamped data transport and can be easily used in many scenarios.

### Messages

| Message File | Type      | Purpose                                                              |
|------------|-----------|----------------------------------------------------------------------|
| `Float64.msg` | `float64` | Generic numeric data (e.g., measurements, thresholds, sensor values) |
| `Int32.msg` | `int32`   | Discrete values such as IDs, states, counters                        |
| `Bool.msg` | `bool`    | Binary flags like enable/disable, detected/not detected              |
| `String.msg` | `string`  | Textual labels, identifiers, status messages                         |
| `Byte.msg` | `byte`    | Compact status codes, enumerated values, or mode flags               |

Each message includes a header to enable time and frame synchronization with other components in the system.

## Service Definitions

The services are built on top of the message types above, extending them into simple request–response patterns:

- **Request**: Header + typed data  
- **Response**: `bool success` — indicating whether the request was successfully handled

### Services

| Service File | Request Type | Purpose                                                             |
|------------|--------------|---------------------------------------------------------------------|
| `Float64.srv` | `float64`    | Send a floating-point command or value, receive success confirmation |
| `Int32.srv` | `int32`      | Send a discrete integer (e.g. ID or state), confirm processing      |
| `Bool.srv` | `bool`       | Trigger binary operations (e.g. toggle, activate)                   |
| `String.srv` | `string`     | Send a label, status command, or ID                                 |
| `Byte.srv` | `byte`       | Send a compact enum/status flag and confirm it was handled          |

These are intended for generic communication patterns like setting parameters, triggering actions, or signaling state changes.
