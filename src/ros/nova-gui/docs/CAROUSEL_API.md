# Carousel API Reference

This document describes the ROS2 topics and services for the URC carousel system.

## Nodes

Two carousel nodes are launched:
- `carousel_inner` - Controls the inner ring
- `carousel_outer` - Controls the outer ring

---

## Topics

### Feedback Topic

**Topic:** `/science/<node_name>/feedback`

| Node | Topic |
|------|-------|
| carousel_inner | `/science/carousel_inner/feedback` |
| carousel_outer | `/science/carousel_outer/feedback` |

**Type:** `science_interfaces/msg/CarouselFeedback`

**Description:** Publishes carousel state feedback. Only publishes when values change. Uses transient local QoS (keeps last message for late subscribers).

**Fields:**

| Field | Type | Description |
|-------|------|-------------|
| `position` | float32 | Current position in degrees (0-360) |
| `current` | float32 | Motor current feedback |
| `load` | float32 | Load feedback |
| `zeroing` | bool | True while hardware zeroing is in progress |

**Example subscription (Python):**
```python
from science_interfaces.msg import CarouselFeedback

def callback(msg: CarouselFeedback):
    print(f"Position: {msg.position}°, Zeroing: {msg.zeroing}")

node.create_subscription(CarouselFeedback, '/science/carousel_inner/feedback', callback, 10)
```

---

## Services

### Set Position

**Service:** `/science/<node_name>/set_position`

| Node | Service |
|------|---------|
| carousel_inner | `/science/carousel_inner/set_position` |
| carousel_outer | `/science/carousel_outer/set_position` |

**Type:** `science_interfaces/srv/SetPosition`

**Description:** Sets the target position of the carousel ring.

**Request:**

| Field | Type | Description |
|-------|------|-------------|
| `position` | float64 | Target position in degrees (0-360) |

**Response:**

| Field | Type | Description |
|-------|------|-------------|
| `success` | bool | True if the command was accepted |

**Example call (Python):**
```python
from science_interfaces.srv import SetPosition

client = node.create_client(SetPosition, '/science/carousel_inner/set_position')
request = SetPosition.Request()
request.position = 45.0
future = client.call_async(request)
```

**Example call (CLI):**
```bash
ros2 service call /science/carousel_inner/set_position science_interfaces/srv/SetPosition "{position: 45.0}"
```

---

### Trigger Zero

**Service:** `/science/carousel/trigger_zero`

**Type:** `std_srvs/srv/Trigger`

**Description:** Initiates hardware zeroing by sending a CAN command to the carousel. The `zeroing` field in the feedback topic will be `true` until zeroing completes.

**Request:** (empty)

**Response:**

| Field | Type | Description |
|-------|------|-------------|
| `success` | bool | True if zero command was sent successfully |
| `message` | string | Status message or error description |

**Example call (Python):**
```python
from std_srvs.srv import Trigger

client = node.create_client(Trigger, '/science/carousel/trigger_zero')
future = client.call_async(Trigger.Request())
```

**Example call (CLI):**
```bash
ros2 service call /science/carousel/trigger_zero std_srvs/srv/Trigger
```

---

### Increment Zero Offset

**Service:** `/science/carousel/increment_zero`

**Type:** `science_interfaces/srv/IncrementZero`

**Description:** Adjusts the software zero offset. This allows fine-tuning the zero position without re-running hardware zeroing.

**Request:**

| Field | Type | Description |
|-------|------|-------------|
| `reset_zero` | bool | If true, resets the zero offset to 0.0 (ignores `increment_zero`) |
| `increment_zero` | float32 | Amount to add to the current zero offset in degrees |

**Response:**

| Field | Type | Description |
|-------|------|-------------|
| `success` | bool | True if the offset was updated successfully |

**Example: Increment offset by 5 degrees (Python):**
```python
from science_interfaces.srv import IncrementZero

client = node.create_client(IncrementZero, '/science/carousel/increment_zero')
request = IncrementZero.Request()
request.reset_zero = False
request.increment_zero = 5.0
future = client.call_async(request)
```

**Example: Reset offset to zero (CLI):**
```bash
ros2 service call /science/carousel/increment_zero science_interfaces/srv/IncrementZero "{reset_zero: true, increment_zero: 0.0}"
```

---

## Message/Service Definitions

### CarouselFeedback.msg
```
float32 position
float32 current
float32 load
bool zeroing
```

### SetPosition.srv
```
float64 position
---
bool success
```

### IncrementZero.srv
```
bool reset_zero
float32 increment_zero
---
bool success
```

### Trigger.srv (std_srvs)
```
---
bool success
string message
```
