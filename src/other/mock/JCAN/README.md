# mock_jcan

A pure-Python mock of the [JCAN](https://github.com/nova-rover/JCAN) CAN bus library
(`jcan_python`), for use in unit/integration tests that exercise code depending on
`import jcan` (e.g. `python_control2` hardware interfaces) without needing:

- the compiled Rust extension (`jcan_python.cpython-*.so`), or
- a real or virtual (`vcanX`) SocketCAN interface.

It implements the same public API described in
[`JCAN/jcan_python/jcan/jcan_python.pyi`](../../../../../JCAN/jcan_python/jcan/jcan_python.pyi):
`Frame` and `Bus`, with the same methods used throughout `python_control2`
(`open`, `close`, `is_open`, `send`, `receive`, `receive_with_timeout`,
`add_callback`, `spin`, `drop_buffered_frames`, `set_id_filter`,
`set_id_filter_mask`, `receive_from_thread_buffer`, `callbacks_enabled`,
`set_callbacks_enabled`).

## How it works

There's no real CAN hardware to talk to, so each CAN interface name (e.g.
`"can1"`, as passed to `Bus.open(interface)`) is backed by an in-process
"virtual network" shared by every `Bus` that opens it. Calling `bus.send(frame)`
delivers the frame to every *other* `Bus` open on the same interface name
(mirroring how multiple sockets on the same real/virtual CAN interface would
all see each other's traffic). Delivered frames sit in a queue until:

- `receive()` / `receive_with_timeout()` is called directly, or
- `spin()` is called, which drains the frames and invokes any callbacks
  registered with `add_callback(frame_id, callback)` - just like the real
  library, callbacks only fire when `spin()` is called.

This means a test can open its own `jcan.Bus()` on the same interface name
used by the code under test (e.g. `"can1"`) to observe frames sent by it, or
to inject incoming frames for it to receive.

## Usage

Make sure this package is importable as `jcan` in place of the real one, e.g.
install it into your test virtual environment:

```sh
pip install -e src/other/mock/JCAN
```

(Do not install this alongside the real `jcan` package - they both provide the
`jcan` module, so only one should be installed at a time in a given
environment.)

### Example: asserting a CAN frame was sent

```python
import jcan
import jcan.testing

def test_heater_turns_on(setup_client_node):
    bus = jcan.Bus()
    bus.open("can1")

    # ... trigger the code under test, e.g. call a ROS service that should
    # result in the node under test sending a CAN frame on "can1" ...

    frame = bus.receive_with_timeout(1000)
    assert frame is not None
    assert frame.id == 0x0C1
    assert frame.data == [1]
```

### Example: resetting state between tests

Add an autouse fixture (e.g. in `conftest.py`) so that virtual CAN networks
from one test don't leak into the next:

```python
import pytest
import jcan.testing

@pytest.fixture(autouse=True)
def _reset_mock_jcan():
    yield
    jcan.testing.reset_all_networks()
```

## Notes on API differences from the real library

`jcan_python.pyi` documents `Frame.id` and `Frame.data` as methods
(`def id(self) -> int`, `def data(self) -> List[int]`), but the compiled
library actually exposes them as read-only properties/attributes - this is
also how `python_control2` uses them (e.g. `frame.data[0]`). This mock matches
the real, observed behaviour and exposes `id`/`data` as properties.
