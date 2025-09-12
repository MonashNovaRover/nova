import device
import time

class Meta(device.Device):
    def __init__(self, manager):
        super().__init__("Meta")
        self._manager=manager
        self._start = time.time()
        self._time = 0
        self.registerAttr("Uptime", self._getTime, 8, units="s")
        self.registerAttr("Load", lambda: self._manager.load, 6, units="%")
    def _getTime(self):
        return round(self._time,2)
    def update(self):
        self._time = time.time()-self._start

