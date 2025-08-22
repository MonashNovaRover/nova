from typing import TypeVar, Generic

T = TypeVar('T')

class Pointer(Generic[T]):
    def __init__(self, value: T):
        self.value = value

class Interface:

    def __init__(self):
        self.values: dict[str, Pointer] = {}

    def __getitem__(self, item) -> Pointer:
        if item not in self.values:
            self.values[item] = Pointer(0)
        return self.values[item]

    def __setitem__(self, key, value):
        self.values[key] = value
