from typing import Callable, Optional

import pyudev


def find_cameras() -> dict[str, str]:
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
            (_identify_camera(device), device)
            for device in context.list_devices(subsystem="video4linux")
        )
        if serial is not None
    }


def watch_cameras(callback: Callable[[bool, str, str], None]) -> Callable[[], None]:
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
        serial = _identify_camera(device)
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


def _identify_camera(device: pyudev.Device) -> Optional[str]:
    # Filter out any devices lacking the "capture" capability.
    #
    # Udev properties can be found on the commandline with `udevadm`
    # (https://www.freedesktop.org/software/systemd/man/udevadm.html),
    # e.g. `udevadm info --name=/dev/video0 -q property`.
    #
    # A comprehensive list of V4L capabilities can be found in the udev source code:
    # https://cgit.freedesktop.org/systemd/systemd/tree/src/udev/v4l_id/v4l_id.c
    if "capture" not in device["ID_V4L_CAPABILITIES"].strip(":").split(":"):
        return None

    serial = device["ID_SERIAL"]
    path = device["ID_PATH"]
    try:
        return serial_overrides[serial][path]
    except KeyError:  # EAFP
        return serial


# Some USB cameras, such as the Microsoft LifeCam HD 3000, do not have a unique serial
# number. This dictionary can be used to spoof a serial number for specific cameras,
# based on their Linux device path.
serial_overrides = {
    "Microsoft_Microsoft\u00AE_LifeCam_HD-3000": {
        "platform-3530000.xhci-usb-0:1.1:1.0": "mast_forward",
        "platform-3530000.xhci-usb-0:1.3:1.0": "mast_down",
        "platform-3530000.xhci-usb-0:1.4:1.0": "mast_backward",
        "platform-3530000.xhci-usb-0:1.2:1.0": "mast_arm_stow",
        "platform-3530000.xhci-usb-0:3.1.3:1.0": "arm_end_forward",
        "platform-3530000.xhci-usb-0:3.1.4:1.0": "arm_end_top",
        "platform-3530000.xhci-usb-0:3.1.1:1.0": "arm_end_finger",
        "platform-3530000.xhci-usb-0:3.1.4:1.0": "arm_end_screw",
        "platform-3530000.xhci-usb-0:3.2:1.0": "arm_gimbal",
        "platform-3530000.xhci-usb-0:2.4.4:1.0": "science_forward",
        "platform-3530000.xhci-usb-0:2.2:1.0": "science_backward",
        "platform-3530000.xhci-usb-0:2.4.1:1.0": "science_platform",
        "platform-3530000.xhci-usb-0:2.4.3:1.0": "science_microscope",
        "platform-3530000.xhci-usb-0:2.4.4:1.0": "science_hypo",
    },
}
