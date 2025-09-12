import jcan
import device

class CanDevice(device.Device):
    def __init__(self, name, interface, canIdList=None, canIdMask=None, canIdMatch=None):
        """Provide either canIdList or both canIdMask and canIdMatch"""
        super().__init__(name)
        self.bus = jcan.Bus()

        if (canIdList is not None):
            self.bus.set_id_filter(canIdList)
        if (canIdMask is not None and canIdMatch is not None):
            self.bus.set_id_filter_mask(canIdMatch, canIdMask)

        self.bus.open(interface)

    def addCallback(self, canId, callback):
        self.bus.add_callback(canId, callback)

    def sendFrame(self, canId, data):
        self.bus.send(jcan.Frame(canId, data))

    def spin(self):
        self.bus.spin()

