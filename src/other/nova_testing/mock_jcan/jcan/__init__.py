"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Pure-python mock of the JCAN library (jcan_python)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Provides a drop-in replacement for `import jcan` so that code depending on the
real (compiled) JCAN python bindings - e.g. python_control2 hardware
interfaces - can be exercised in tests without real/virtual CAN hardware.

Mirrors the public API described in JCAN/jcan_python/jcan/jcan_python.pyi:
`Frame` and `Bus`.

Every `Bus` opened with the same `interface` name shares an in-process
"virtual CAN network": frames sent from one Bus are delivered to every other
Bus open on that interface, similarly to how multiple sockets on the same
real (or virtual `vcanX`) CAN interface would all see each other's traffic.
See README.md for usage examples.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
from __future__ import annotations

import queue
import threading
from typing import Callable, Dict, List, Optional, Set, Tuple, Union

__all__ = ["Frame", "Bus"]


class Frame:
    """ A single CAN frame: an 11-bit ID plus up to 8 data bytes. """

    def __init__(self, id: int, data: List[Union[int, float]]) -> None:
        """
        :param id: The 11-bit CAN ID of the frame.
        :param data: The data bytes of the frame. Cast to uint8's, matching the real
        JCAN library - so be careful to double check the values!
        """
        self._id = int(id)
        self._data = [int(round(b)) & 0xFF for b in data]

    def __str__(self) -> str:
        data_hex = " ".join(f"{b:02X}" for b in self._data)
        return f"Frame(id=0x{self._id:X}, data=[{data_hex}])"

    def __repr__(self) -> str:
        return str(self)

    def __eq__(self, other: object) -> bool:
        if not isinstance(other, Frame):
            return NotImplemented
        return self._id == other._id and self._data == other._data

    @property
    def id(self) -> int:
        """ :return: The 11-bit CAN ID of the frame. """
        return self._id

    @property
    def data(self) -> List[int]:
        """ :return: The data bytes of the frame. """
        return self._data


class _VirtualNetwork:
    """ Shared in-process state for every Bus opened with a given CAN interface name. """

    def __init__(self, name: str):
        self.name = name
        self._lock = threading.RLock()
        self._buses: List["Bus"] = []
        self.sent_frames: List[Frame] = []

    def register(self, bus: "Bus") -> None:
        with self._lock:
            self._buses.append(bus)

    def unregister(self, bus: "Bus") -> None:
        with self._lock:
            if bus in self._buses:
                self._buses.remove(bus)

    def broadcast(self, frame: Frame, sender: "Bus") -> None:
        with self._lock:
            self.sent_frames.append(frame)
            buses = list(self._buses)
        for bus in buses:
            if bus is sender:
                continue
            bus._deliver(frame)


_networks: Dict[str, _VirtualNetwork] = {}
_networks_lock = threading.RLock()


def _get_network(interface: str) -> _VirtualNetwork:
    with _networks_lock:
        network = _networks.get(interface)
        if network is None:
            network = _VirtualNetwork(interface)
            _networks[interface] = network
        return network


class Bus:
    """ A virtual CAN bus, connected to other `Bus` instances opened with the same interface name. """

    def __init__(self) -> None:
        self._interface: Optional[str] = None
        self._network: Optional[_VirtualNetwork] = None
        self._is_open = False
        self._callbacks_enabled = True
        self._tx_queue_len = 2
        self._rx_queue_len = 256
        self._rx_queue: "queue.Queue[Frame]" = queue.Queue()
        self._thread_buffer: List[Frame] = []
        self._thread_buffer_lock = threading.RLock()
        self._callbacks: Dict[int, List[Callable[[Frame], None]]] = {}
        self._allowed_ids: Optional[Set[int]] = None
        self._allowed_mask: Optional[Tuple[int, int]] = None

    def open(self, interface: str, tx_queue_len: int = 2, rx_queue_len: int = 256) -> None:
        """
        :param interface: The name of the CAN interface to open, e.g. "vcan0".
        :param tx_queue_len: The length of the internal transmit queue, after which send() will block.
        :param rx_queue_len: The length of the internal receive queue, after which older Frames will be dropped.
        """
        if self._is_open:
            self.close()

        self._interface = interface
        self._tx_queue_len = tx_queue_len
        self._rx_queue_len = rx_queue_len
        self._network = _get_network(interface)
        self._network.register(self)
        self._is_open = True

    def close(self) -> None:
        """ Closes the CAN interface. """
        if self._network is not None:
            self._network.unregister(self)
        self._network = None
        self._is_open = False

    def is_open(self) -> bool:
        """ :return: True if the CAN interface is open, False otherwise. """
        return self._is_open

    def callbacks_enabled(self) -> bool:
        """ :return: True if callbacks are enabled, False otherwise. """
        return self._callbacks_enabled

    def set_callbacks_enabled(self, mode: bool) -> None:
        """ :param mode: True to enable callbacks, False to disable. """
        self._callbacks_enabled = mode

    def receive(self) -> Frame:
        """
        Blocks until a frame is received, then returns it.
        :return: The received frame.
        """
        return self._rx_queue.get()

    def receive_with_timeout(self, timeout_ms: int) -> Optional[Frame]:
        """
        Blocks until a frame is received, or the timeout expires, then returns it.
        :param timeout_ms: The timeout in milliseconds.
        :return: The received frame, or None if the timeout expired.
        """
        try:
            return self._rx_queue.get(timeout=timeout_ms / 1000.0)
        except queue.Empty:
            return None

    def send(self, frame: Frame) -> None:
        """
        Sends a frame, blocking until it is queued for transmission on the TX queue.
        :param frame: The frame to send.
        """
        if not self._is_open or self._network is None:
            raise RuntimeError("Cannot send on a Bus that is not open.")
        self._network.broadcast(frame, self)

    def drop_buffered_frames(self) -> None:
        """ Drop all frames in the RX queue. """
        while True:
            try:
                self._rx_queue.get_nowait()
            except queue.Empty:
                break
        with self._thread_buffer_lock:
            self._thread_buffer.clear()

    def set_id_filter(self, allowed_ids: List[int]) -> None:
        """
        Set a filter for the CAN ID's that will be received, and for which callbacks will be called.
        :param allowed_ids: A list of allowed CAN ID's.
        """
        self._allowed_ids = set(allowed_ids)

    def set_id_filter_mask(self, allowed: int, allowed_mask: int) -> None:
        """
        Set a filter for the CAN ID's that will be received, and for which callbacks will be called.
        :param allowed: The base allowed ID value.
        :param allowed_mask: The mask for bits of the allowed ID, which are to be checked.
        """
        self._allowed_mask = (allowed, allowed_mask)

    def receive_from_thread_buffer(self) -> List[Frame]:
        """ :return: A list of frames that are currently in the internal receive buffer. """
        with self._thread_buffer_lock:
            frames = list(self._thread_buffer)
            self._thread_buffer.clear()
        return frames

    def add_callback(self, frame_id: int, callback: Callable[[Frame], None]) -> None:
        """
        Add a callback for a specific CAN ID.
        :param frame_id: The CAN ID for which the callback will be called.
        :param callback: The callback function, which will be called with the received frame as an argument.
        """
        self._callbacks.setdefault(frame_id, []).append(callback)

    def spin(self) -> None:
        """ Needs to be called periodically to process received frames and call callbacks. """
        if not self._callbacks_enabled:
            return
        for frame in self.receive_from_thread_buffer():
            for callback in self._callbacks.get(frame.id, []):
                callback(frame)

    def _passes_filter(self, frame_id: int) -> bool:
        if self._allowed_ids is not None and frame_id not in self._allowed_ids:
            return False
        if self._allowed_mask is not None:
            allowed, mask = self._allowed_mask
            if (frame_id & mask) != (allowed & mask):
                return False
        return True

    def _deliver(self, frame: Frame) -> None:
        """ Internal method. Delivers a frame sent by another Bus on the same network to this Bus. """
        if not self._passes_filter(frame.id):
            return

        # Drop the oldest buffered frame once the RX queue is full, matching the real library's behaviour.
        if self._rx_queue.qsize() >= self._rx_queue_len:
            try:
                self._rx_queue.get_nowait()
            except queue.Empty:
                pass
        self._rx_queue.put(frame)

        with self._thread_buffer_lock:
            self._thread_buffer.append(frame)
