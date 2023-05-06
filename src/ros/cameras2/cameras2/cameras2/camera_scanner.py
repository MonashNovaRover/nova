from typing import Callable, Optional, NamedTuple

import pyudev


class CameraScanner:
    class SerialOverride(NamedTuple):
        root: str
        paths: dict[str, str]

    _serial_remaps = dict[str, str]

    # Some USB cameras, such as the Microsoft LifeCam HD 3000, do not have a unique serial
    # number. This table can be used to spoof a serial number for specific cameras, based
    # on their Linux device path.
    _serial_override_map: dict[str, str]

    def __init__(
        self,
        serial_remaps: dict[str, str] = None,
        serial_overrides: list[SerialOverride] = None,
    ):
        self._serial_remaps = serial_remaps if serial_remaps is not None else {}
        self._serial_override_map = {
            f"{override.root}.{path}": serial
            for override in (serial_overrides if serial_overrides is not None else [])
            for path, serial in override.paths.items()
        }

    def _identify_camera(self, device: pyudev.Device) -> Optional[str]:
        # Filter out any devices lacking the "capture" capability.
        #
        # Udev properties can be found on the commandline with `udevadm`
        # (https://www.freedesktop.org/software/systemd/man/udevadm.html),
        # e.g. `udevadm info --name=/dev/video0 -q property`.
        #
        # A comprehensive list of V4L capabilities can be found in the udev source code:
        # https://cgit.freedesktop.org/systemd/systemd/tree/src/udev/v4l_id/v4l_id.c
        capabilities: list[str]
        try:
            capabilities = (
                device.properties["ID_V4L_CAPABILITIES"].strip(":").split(":")
            )

        except KeyError:
            # Sometimes, V4L is still initializing and has not yet made a capabilities property available.
            # In this situation, treat the camera as if it were incapable of recording.
            # Once V4L has finished initializing, it will be reported again by the udev through the monitor.
            capabilities = []
        if "capture" not in capabilities:
            return None

        serial = device["ID_SERIAL"]
        path = device["ID_PATH"]
        overriden_serial = self._serial_override_map.get(path, serial)
        return self._serial_remaps.get(overriden_serial, overriden_serial)

    def find_cameras(self) -> dict[str, str]:
        """
        Finds cameras connected to the system, and their serial numbers.

        This function queries the udev database, and will not work on systems
        without udev and Video4Linux.

        :return: A dictionary mapping serial numbers to device nodes.
        """
        context = pyudev.Context()
        return {
            serial: device.device_node
            for serial, device in (
                (self._identify_camera(device), device)
                for device in context.list_devices(subsystem="video4linux")
            )
            if serial is not None
        }

    def watch_cameras(
        self,
        callback: Callable[[bool, str, str], None],
    ) -> Callable[[], None]:
        """
        Continuously watches for cameras to be connected to or removed from the system.

        :param callback: A callback function called whenever a camera is connected or removed.
                         The callback is passed three arguments: A boolean (True if the camera has been added or updated,
                         False if it has been removed or taken offline), the serial number of the camera, and the camera's
                         device node.
        :return: A function that can be called to stop watching for cameras.
        """
        context = pyudev.Context()
        monitor = pyudev.Monitor.from_netlink(context)
        monitor.filter_by("video4linux")

        def monitor_callback(device: pyudev.Device) -> None:
            serial = self._identify_camera(device)
            if serial is None:
                return None

            callback(
                device.action in {"add", "change", "move", "online"},
                serial,
                device.device_node,
            )

        observer = pyudev.MonitorObserver(monitor, callback=monitor_callback)
        observer.start()
        return observer.stop
