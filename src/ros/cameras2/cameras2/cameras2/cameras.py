import pyudev


def find_cameras() -> dict[str, str]:
    """
    Finds cameras connected to the system, and their serial numbers.

    This function queries the udev database, and will not work on systems
    without udev and Video4Linux.

    :return: A dictionary mapping serial numbers to device nodes.
    """
    # Find all Video4Linux devices with the "capture" capability, and generate a
    # dictionary mapping serial numbers to device nodes.
    #
    # Udev properties can be found on the commandline with `udevadm`
    # (https://www.freedesktop.org/software/systemd/man/udevadm.html),
    # e.g. `udevadm info --name=/dev/video0 -q property`
    context = pyudev.Context()
    return {
        _find_camera_serial(device["ID_SERIAL"], device["ID_PATH"]): device.device_node
        for device in context.list_devices(
            subsystem="video4linux", ID_V4L_CAPABILITIES=":capture:"
        )
    }


def _find_camera_serial(serial: str, path: str) -> str:
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
