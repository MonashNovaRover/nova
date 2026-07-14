# Science integration tests

This directory contains the pytest-based integration tests for the `science`
package.

## How the tests are run

The package's `default.nix` enables checks and runs pytest from the Nix build
phase. Before pytest starts, `preCheck` does two important things:

1. It sets `ROS_LOG_DIR` inside the temporary build directory so ROS logging
   stays writable during the test run.
2. It prepends the `mock-jcan` package to `PYTHONPATH`, which makes the virtual
   CAN bus implementation available to the tests.

The actual command is:

```sh
pytest ../science/pytest
```

In practice, that means the pytest suite is exercised as part of the package
build, not only when run manually.

## Test architecture

The shared test fixtures live in `conftest.py` and provide a small ROS 2 test
harness:

- `ClientNode` is a lightweight ROS node used by tests to create service clients
  and subscriptions.
- `setup_tester()` initializes `rclpy`, creates client nodes, and tears them
  down after the test finishes.
- `sut_executor()` spins a `SingleThreadedExecutor` in a background thread so
  the node under test can process callbacks while the test thread keeps running.
- `setup_sut()` creates the system-under-test node and adds it to the executor.
- `reset_mock_jcan()` clears the mock CAN network after every test so CAN state
  does not leak between tests.

# Tests
## Kiln temperature test

`test_kiln_temp.py` is an integration test for the kiln controller.

What it does:

1. Starts the kiln SUT from `science.arc.kiln.main`.
2. Creates a tester node that waits for the `/science/thermal_command` service.
3. Opens the mocked `can1` bus used by the kiln controller.
4. Sends a thermal command request with `state=True` and `target=0`.
5. Waits for the service response and checks that it reports success.
6. Verifies that the kiln controller emitted CAN traffic for either the left or
   right heater.

The test is marked `@pytest.mark.manual`, which makes it easy to separate from
lighter unit tests when you want to filter the suite.

The kiln test keeps a few values in sync with `science/arc/kiln.py`:

- `SERVICE = "/science/thermal_command"`
- `CAN_BUS = "can1"`
- `LEFT_HEATER_CAN_ID = 0x0C1`
- `RIGHT_HEATER_CAN_ID = 0x0D2`

If any of those change in the kiln implementation, update the test and this
README together.
