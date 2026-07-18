# Science

All ROS2 nodes and interfaces associated with our teams science functionality.

See the science launch files for the latest configuration used in competition and testing:
- [URC launch file](./science_bringup/launch/urc.launch.py)
- [ARC launch file](./science_bringup/launch/arc.launch.py)

## Working with JCAN 

### Key `jcan` Commands

Get the CAN bus from the control contexts (when working with PC2):

```python
self.bus = contexts[jcan.Bus]
```

Create a CAN frame from a CAN ID and payload bytes:

```python
frame = jcan.Frame(0x001, [0x12, 0x34]) # = 001#1234
```

Send a frame on the bus:

```python
self.bus.send(frame)
```

Or inline:

```python
self.bus.send(jcan.Frame(0x001, [0x12, 0x34]))
```

### Packing an Integer Into CAN Bytes

A CAN payload is just a sequence of bytes. One byte is one two-digit hex group.

Use `to_bytes()` to pack an integer into exactly `length` bytes:

```python
payload = list(value.to_bytes(length, byteorder="big", signed=False))
```

Examples:

```python
list(0x7F.to_bytes(1, "big", signed=False))     # [0x7F]
list(0x1234.to_bytes(2, "big", signed=False))   # [0x12, 0x34]
list(0x1234.to_bytes(4, "big", signed=False))   # [0x00, 0x00, 0x12, 0x34]
```

### Unpacking CAN Bytes Back Into an Integer

Use `int.from_bytes()` to turn the CAN bytes back into an integer:

```python
value = int.from_bytes(bytes(payload), byteorder="big", signed=False)
```

Examples:

```python
int.from_bytes(bytes([0x7F]), "big", signed=False)         # 127
int.from_bytes(bytes([0x12, 0x34]), "big", signed=False)   # 4660
```
