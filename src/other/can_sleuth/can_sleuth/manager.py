'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Can Sleuth / Simulator

Manager class for managing devices and outputs
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
EDITED BY: Orlando Chamberlain
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
import time

from can_sleuth.outputs import tui
from can_sleuth.devices import meta

class Manager:
    def __init__(self, devices, outputs=None, updatePeriod=0.2, spinPeriod=0.05, includeMeta=True):
        """Manager for our devices and outputs.

        :param devices: List of device.Device
        :param outputs: List of output.Output
        :param updatePeriod: how often the outputs should output state
        :param spinPeriod: how often devices can update their internal state
        :param includeMeta: do we add the meta device (tracks uptime, if the manager is meeting time deadlines)
        """

        self._devices = devices

        if (includeMeta):
            self._devices.append(meta.Meta(self))

        if outputs is None:
            outputs = [tui.TUI()]

        self._outputs = outputs

        self._updatePeriod = updatePeriod
        self._spinPeriod = spinPeriod

        self._nextSpin = 0
        self._nextUpdate = 0
        self._sleptTime = 0
        self.load = ""

    def spin(self):
        """Update the devices and outputs ad infinitum
        """
        try:
            if (time.time() >= self._nextSpin):
                self._spinDevices()
            if (time.time() >= self._nextUpdate):
                self.load = f"{100-100*self._sleptTime/self._updatePeriod:.2f}"
                self._sleptTime=0
                self._update()

            #if (time.time() >= self._nextSpin) and (time.time() >= self._nextUpdate):
            #    panic # running overtime :(

            sleepTime = max(0,min(self._nextSpin, self._nextUpdate)-time.time())
            self._sleptTime += sleepTime
            time.sleep(sleepTime)

        except Exception as e:
            # this is important for the tui as when it is garbage collected it clears the screen, hiding the error.
            # instead, do this before we print the error so it does not get cleared.
            for output in self._outputs:
                if hasattr(output, "__del__"):
                    output.__del__()
            raise e

    def _update(self):
        """Tell the devices to update their state, then the outputs
        """
        self._nextUpdate = max(time.time(), self._nextUpdate + self._updatePeriod)
        for dev in self._devices:
            dev.update()
        for output in self._outputs:
            output.update(self._devices)

    def _spinDevices(self):
        """Tell the devices to update their internal state and do anything they need to do
        frequently
        """
        self._nextSpin = max(time.time(), self._nextSpin + self._spinPeriod)
        for dev in self._devices:
            dev.spin()
