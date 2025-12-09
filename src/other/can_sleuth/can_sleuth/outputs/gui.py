'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Can Sleuth / Simulator

Qt Graphical Interface Output

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
EDITED BY: Orlando Chamberlain
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''

from PySide6.QtWidgets import QApplication, QWidget, QLabel, QVBoxLayout
from PySide6.QtCore import QTimer

#from threading import Thread
from multiprocessing import Process, Manager
import time
import signal

from . import output

class GUI(output.Output):
    """A Graphical User Interface for rendering the state of the system.
    """

    def __init__(self):

        self.manager = Manager()
        sharedDict = self.manager.dict()
        self.qtProcess = Process(target = self.qtInit, args=(sharedDict,))
        self.sharedDict = sharedDict

        self.sharedDict["pendingUpdate"] = False
        self.sharedDict["data"] = ""

        self.qtProcess.start()
        time.sleep(0.1)
        if (not self.qtProcess.is_alive()):
            raise RuntimeError("gui could not start")


    def qtInit(self, sharedDict):
        """Initialise the qt window in a different process
        """
        self.sharedDict = sharedDict

        # This will just kill the process if it fails. 
        # What happened to "Hi, How are you? Here is an exception you can catch!"
        self.app = QApplication([])

        self.window = QWidget()
        self.window.setWindowTitle("Can Sleuth")
        self.window.show()

        self.layout = QVBoxLayout()

        self.label = QLabel("", self.window)

        self.layout.addWidget(self.label)
        self.window.setLayout(self.layout)


        self.timer = QTimer(self.window)
        self.timer.setInterval(20)
        self.timer.timeout.connect(self.qtUpdate)
        self.timer.start()

        self.app.exec()

    def cleanup(self):
        """tell the process running qt to die"""
        self.qtProcess.terminate()

    def qtUpdate(self):
        """check if there's any new data from the main process and put it on the window if needed."""

        # update on the qt thread
        if (self.sharedDict["pendingUpdate"]):
            self.label.setText(self.sharedDict["data"])
            self.sharedDict["pendingUpdate"] = False
            self.window.update()

    def update(self, devices):
        """Update the window with the state of the devices
        :param devices: the devices in the system
        """
        if (not self.qtProcess.is_alive()):
            print("GUI Closed. Quitting...")
            raise KeyboardInterrupt()

        # TODO: don't just do one big string, split it into boxes at a graphical level
        text = []
        for dev in devices:
            text.append(f"<{dev.getName()}>")
            for attr in dev.attrs:
                text.append(f"\t{attr.name}: {attr.value()}{attr.units}")

        self.sharedDict["data"] = "\n".join(text)
        self.sharedDict["pendingUpdate"] = True
