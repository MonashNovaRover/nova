import abc
class Output(abc.ABC):
    @abc.abstractmethod
    def update(self, devices):
        """To be run every time we want the output to output the current state of devices
        to the terminal/a file/whatever this output is.
        """
        pass
