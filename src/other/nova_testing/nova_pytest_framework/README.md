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

### `setup_tester`

A fixture factory that manages `rclpy` lifecycle and creates lightweight
`TesterNode` instances for each test. Nodes are automatically destroyed and
`rclpy` is shut down at the end of the test.

```python
def test_example(setup_tester):
    tester = setup_tester(
        "my_test_node",
        services=[{"service": "/some_service", "srv_type": SomeSrv}],
        topics=[{"topic": "/some_topic", "msg_type": SomeMsg, "callback": cb}],
    )

    future = tester.send_request("/some_service", {"field": value})
```

### `logger`

A session-scoped logger instance for test diagnostics.

## TesterNode

A lightweight ROS 2 node that can:

- Create service clients and wait for them to become available (5 s timeout).
- Subscribe to topics with user-provided callbacks.
- Send async service requests via `send_request(service, payload)`.
