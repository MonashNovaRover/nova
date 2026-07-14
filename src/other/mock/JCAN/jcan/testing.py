"""
Test-only helpers for interacting with the mock JCAN virtual CAN networks.
These are NOT part of the real JCAN API - only use them from test code, to
reset state between tests and to make assertions about frames that were sent.
"""
from typing import List

from . import Frame, _get_network, _networks, _networks_lock

__all__ = ["reset_all_networks", "get_sent_frames"]


def reset_all_networks() -> None:
    """
    Clears every virtual CAN network, disconnecting any currently open `Bus` instances
    from one another. Call this between tests (e.g. in an autouse fixture) so that state
    doesn't leak from one test into the next:

        @pytest.fixture(autouse=True)
        def _reset_mock_jcan():
            yield
            jcan.testing.reset_all_networks()
    """
    with _networks_lock:
        _networks.clear()


def get_sent_frames(interface: str) -> List[Frame]:
    """
    :param interface: The CAN interface name (as passed to `Bus.open`) to inspect.
    :return: Every Frame that has been sent on the given CAN interface so far, in order.
    """
    return list(_get_network(interface).sent_frames)
