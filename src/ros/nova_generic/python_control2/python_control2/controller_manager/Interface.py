from typing import TypeVar, Generic
from teleop_python_utils import Event

T = TypeVar('T')

class Interface(Generic[T]):
    """Simple class that keeps a reference to a value."""
    def __init__(self, value: T):
        self.value = value

        # True if anything provides the value for this Interface
        self.populated = False

    def __bool__(self) -> bool:
        """ Boolean converion checks if anything actually provides a value for the interface. """
        return self.populated

class InterfaceCollection:
    """ Wrapper for a dictionary of Interfaces. Represents Command and State Interfaces. """

    def __init__(self, default_value: T=0):
        self._values: dict[str, Interface] = {}
        self.default_value = default_value

        # Event called whenever we retrieve an interface. For internal use only.
        self.on_get_item: Event[[Interface]] = Event()

    def __getitem__(self, item) -> Interface:
        """ Gets the Interface value at the provided key. Creates one if it doesn't exist. """
        if item not in self._values:
            self._values[item] = Interface(self.default_value)

        interface = self._values[item]
        self.on_get_item.invoke(interface)
        return interface

    def __setitem__(self, key, value):
        """ Sets the Interface value at the provided key to the given value. """
        self[key].value = value

    def __contains__(self, key: str) -> bool:
        """ Gets if the collection contains an interface with a given name. Doesn't matter if the element is provided.
        """
        return key in self._values
