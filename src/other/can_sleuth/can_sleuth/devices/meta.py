import time

from . import device

class Meta(device.Device):
    """A fake device to expose meta stuff like uptime, processing load etc
    """
    def __init__(self, manager):
        super().__init__("Meta")
        self.connected = True
        self._manager=manager

        # Timekeeping Vars
        self._start = time.time()
        self._uptime = 0

        # Attributes
        self.registerAttr("Uptime", self._getTime, 8, units="s")
        self.registerAttr("Load", lambda: self._manager.load, 6, units="%")

    def _getTime(self):
        return round(self._uptime,2)

    def update(self):
        # Set the uptime
        self._uptime = time.time()-self._start

