from typing import TypeVar, Generic

T = TypeVar('T')

class Interface(Generic[T]):
    """Simple class that keeps a reference to a value."""
    def __init__(self, value: T):
        self.value = value

class InterfaceCollection:
    """Wrapper for a dictionary of Interfaces. Represents Command and State Interfaces"""

    def __init__(self):
        self._values: dict[str, Interface] = {}

    def __getitem__(self, item) -> Interface:
        """Gets the Interface value at the provided key. Creates one if it doesn't exist."""
        if item not in self._values:
            self._values[item] = Interface(0)
        return self._values[item]

    def __setitem__(self, key, value):
        """Sets the Interface value at the provided key to the given value."""
        self[key].value = value
