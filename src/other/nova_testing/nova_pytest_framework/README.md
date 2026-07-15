# nova_pytest_framework

A pytest plugin providing shared fixtures for Nova ROS 2 integration tests.

It is installed as a [pytest plugin](https://docs.pytest.org/en/stable/how-to/writing_plugins.html)
via the `pytest11` entry point, so fixtures are available automatically once the
package is installed — no manual `conftest.py` import required.

## Installation

The package is included in the Nix build via `default.nix`. 

To run a pytest on nix build, add the following to your default.nix:

```nix
  doCheck = true;

  preCheck = ''
    export ROS_LOG_DIR="$TMPDIR/.ros/log"
  '';

  checkPhase = ''
    runHook preCheck
    ${pythonPackages.pytest}/bin/pytest ../science/pytest
    runHook postCheck
  '';
```

> Note: You will also need to add mock_jcan if using jcan fixtures, see mock_jcan on how to modify your default.nix to enable mock_jcan in checkPhase

To run pytest in a nix-shell run `checkPhase` as a command once in the shell and then run pytest as normal e.g:

```sh
nova-shell -A pkgs.ros.nova-science
checkPhase            # will enable any mock packages and fixup ros logging
pytest src/ros/rover/science/science/pytest   # run science pytests
```


For local development outside of Nix:

```sh
pip install -e .
```

## IDE
To get type hints working in vscode do the following:
Add assuming your workspace is ~/nova, add the following to your .vscode/settings.json file:
```json
    "python.analysis.extraPaths": [
        "src/other/nova_testing/nova_pytest_framework"
    ]
```
Then make sure the conftest.py in the same folder as your tests reimports the module:
```py
from nova_pytest_framework.plugin import (
    logger,
    reset_mock_jcan,
    setup_sut,
    setup_tester,
    sut_executor,
)
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
