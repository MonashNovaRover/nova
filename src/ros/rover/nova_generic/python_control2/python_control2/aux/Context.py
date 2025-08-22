from typing import Type, TypeVar, Dict

T = TypeVar("T")

class Contexts:
    def __init__(self) -> None:
        self._instances: Dict[Type, object] = {}

    def __getitem__(self, cls: Type[T]) -> T:
        """Retrieve an instance for the given class."""
        # Returns none when no instance of the class exists
        return self._instances.get(cls)

    def __setitem__(self, cls: Type[T], instance: T) -> None:
        """Store an instance under its class type."""
        if not isinstance(instance, cls):
            raise TypeError(f"Expected instance of {cls.__name__}, got {type(instance).__name__}")
        self._instances[cls] = instance

    def construct(self, cls: Type[T], *args, **kwargs) -> T:
        """Construct, store, and return an instance of the given class."""
        instance = cls(*args, **kwargs)
        self._instances[cls] = instance
        return instance