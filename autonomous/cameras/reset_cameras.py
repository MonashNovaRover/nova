import pyrealsense2 as rs


def ctx():
    """
    Factory method for two cameras (D415 and T265) running in separate threads
    Manages QOS for both and re-starts as necessary
    """
    # reset usbs and get info
    ctx = rs.context()
    devices = ctx.query_devices()
    for dev in devices:
        dev.hardware_reset()
        print(dev.get_info)
