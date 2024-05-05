import abc
from logging import Logger

from python_control.controls.Direction import Direction


class Control(abc.ABC):
    """Class to control a single axis motor"""

    def __init__(self, logger: Logger):
        self.logger = logger # type: Logger

    @abc.abstractmethod
    def stop(self):
        pass
