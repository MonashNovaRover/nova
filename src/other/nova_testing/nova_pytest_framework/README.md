# nova_pytest_framework

A pytest plugin providing shared fixtures for Nova ROS 2 integration tests.

It is installed as a [pytest plugin](https://docs.pytest.org/en/stable/how-to/writing_plugins.html)
via the `pytest11` entry point, so fixtures are available automatically once the
package is installed — no manual `conftest.py` import required.

## Installation

The package is included in the Nix build via `default.nix`. 

To run a pytest on nix build, add the following to your default.nix:

```nix
  propagatedBuildInputs = [... pythonPackages.nova-pytest-framework];

  doCheck = true; # ensure tests get run

  checkPhase = ''
    runHook preCheck
    ${pythonPackages.pytest}/bin/pytest ../science/tests
    runHook postCheck
  '';
```

> Note: You will also need to add mock_jcan if using jcan fixtures, see mock_jcan on how to modify your default.nix to enable mock_jcan in checkPhase

You can run the tests as normal in a nix-shell e.g:

```sh
nova-shell -A pkgs.ros.nova-science
pytest src/ros/rover/science/science/pytest   # run science pytests
```


For local development outside of Nix:

```sh
pip install -e .
```


## Fixtures

The plugin exposes these pytest fixtures:

- `logger`: session-scoped logger for test diagnostics.
- `setup_can`: factory that opens a `jcan.Bus` for a named virtual CAN network and closes all opened buses at teardown.
- `reset_can`: autouse cleanup that clears mock JCAN state between tests.
- `initalise_ros2`: session-scoped `rclpy.init()` / `rclpy.shutdown()` wrapper used by the ROS 2 fixtures.
- `setup_ros2_tester`: factory that creates lightweight `TesterNode` instances, tracks them for cleanup, and waits up to 5 seconds for service clients to appear.
- `ros2_sut_executor`: background `SingleThreadedExecutor` used to spin the system under test while the test thread keeps running.
- `setup_ros2_sut`: factory that creates a node under test, wires it into the background executor, and tears it down after the test.

Typical usage looks like this:

```python
from nova_pytest_framework.ros2_helpers import ServiceInteraction, TopicInteraction


def test_example(setup_ros2_tester):
  tester = setup_ros2_tester(
    "my_test_node",
    services=[
      {
        "service": "/some_service",
        "srv_type": SomeSrv,
        "interaction": ServiceInteraction.CLIENT,
      }
    ],
    topics=[
      {
        "topic": "/some_topic",
        "msg_type": SomeMsg,
        "interaction": TopicInteraction.SUBSCRIBER,
        "callback": cb,
      }
    ],
  )

  future = tester.send_request("/some_service", {"field": value})
```

## TesterNode

A lightweight ROS 2 node that can:

- Create service clients and wait for them to become available (5 s timeout).
- Subscribe to topics with user-provided callbacks.
- Send async service requests via `send_request(service, payload)`.
