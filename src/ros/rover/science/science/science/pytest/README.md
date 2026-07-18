# Science integration tests

This directory contains the pytest-based integration tests for the `science`
package.

## How the tests are run

The package's `default.nix` enables checks and runs pytest during the Nix build
phase.

To run the tests manually, enter a nix-shell for this package and run
```
pytest ../science/tests
```

In practice, that means the pytest suite is exercised as part of the package
build, not only when run manually.

## Test architecture

The test harness is assembled from the shared fixtures exposed by
`nova-pytest-framework`.
Local shared fixtures and functions are implemented in `conftest.py`
Test specific fixtures are implemented in their respective tests.
