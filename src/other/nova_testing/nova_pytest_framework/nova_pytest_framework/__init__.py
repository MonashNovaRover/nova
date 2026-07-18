import logging
from collections.abc import Generator

import jcan
import jcan.testing
import pytest

# reexport fixtures for pytest
from .ros2_helpers import (  # noqa: F401
    initalise_ros2,
    ros2_sut_executor,
    setup_ros2_sut,
    setup_ros2_tester,
    setup_python_control2_sut,
)


@pytest.fixture(scope="session")
def logger():
    return logging.getLogger(__name__)


@pytest.fixture()
def setup_can() -> Generator[jcan.Bus, None, None]:
    buses: list[jcan.Bus] = []

    def _make_jcan_bus(bus_name: str) -> jcan.Bus:
        bus = jcan.Bus()
        bus.open(bus_name)
        buses.append(bus)
        return bus

    yield _make_jcan_bus
    for bus in buses:
        bus.close()


@pytest.fixture(autouse=True)
def reset_can():
    """
    Ensures virtual CAN networks from the mock jcan library (src/other/mock/JCAN)
    don't leak state (e.g. sent frames, open buses) from one test into the next.
    """
    yield
    jcan.testing.reset_all_networks()
