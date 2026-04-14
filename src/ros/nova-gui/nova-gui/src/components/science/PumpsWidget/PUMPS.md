# URC Pumps Node

Controls two peristaltic pumps for the URC science sample processing system.

## Node

**Name:** `pumps`

## Hardware

| Pump | Hardware Interface | CAN ID |
|------|-------------------|--------|
| Cache to Shot | `cache_to_shot_pump` | `0x031` |
| Shot to Carousel | `shot_to_carousel_pump` | `0x032` |

## Services

### `/science/pumps/run`

Start a pump operation.

**Type:** `science_interfaces/srv/RunPump`

```
# Request
string pump        # Pump name: "fill_shots", "fill_cuvettes_prime", or "fill_cuvettes"
float32 duration   # Duration in seconds (0 = use default of 10s)
---
# Response
bool success       # True if pump started
string message     # Status message
```

**Example:**
```bash
ros2 service call /science/pumps/run science_interfaces/srv/RunPump "{pump: 'fill_shots', duration: 5.0}"
```

### `/science/pumps/stop`

Stop the current pump operation.

**Type:** `std_srvs/srv/Trigger`

```
# Request
(empty)
---
# Response
bool success
string message
```

**Example:**
```bash
ros2 service call /science/pumps/stop std_srvs/srv/Trigger
```

## Topics

### `/science/pumps/status`

Current pump status, published at 5Hz.

**Type:** `science_interfaces/msg/PumpStatus`

```
bool running         # True if a pump is currently running
string pump          # Current pump name (empty if not running)
float32 time_elapsed # Seconds elapsed since pump started
float32 time_target  # Target duration in seconds
```

**Example:**
```bash
ros2 topic echo /science/pumps/status
```

## Pump Operations

| Operation | Pump Used | Description |
|-----------|-----------|-------------|
| `fill_shots` | Cache to Shot | Transfer samples from cache to shot glasses |
| `fill_cuvettes_prime` | Shot to Carousel | Prime the carousel pump system |
| `fill_cuvettes` | Shot to Carousel | Fill cuvettes in the carousel |

## Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `max_effort` | `0.75` | Maximum pump effort (0.0-1.0) |
| `default_duration` | `10.0` | Default run duration in seconds |
| `publish_rate` | `5` | Status publish rate in Hz |

## Usage

```python
# Start fill_shots for 5 seconds
ros2 service call /science/pumps/run science_interfaces/srv/RunPump "{pump: 'fill_shots', duration: 5.0}"

# Monitor status
ros2 topic echo /science/pumps/status

# Stop early if needed
ros2 service call /science/pumps/stop std_srvs/srv/Trigger
```
