#!/usr/bin/env python3

import abc
from logging import Logger

class Control(abc.ABC):
    """Class to control a single axis motor"""

    def __init__(self, logger: Logger):
        self.logger = logger # type: Logger

    def get_logger(self) -> Logger:
        return self.logger

    @abc.abstractmethod
    def stop(self):
        pass
