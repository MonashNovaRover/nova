from resolver_publisher import ResolverTransceiver


def get_transceiver():
    return ResolverTransceiver(
        receive_timeout = 0.05,
        receive_fmt = '<H',
        transmit_fmt = '@B',
        logger = None,
        baudrate = 115200,
        port = '/dev/ttyUSB0',
        )
