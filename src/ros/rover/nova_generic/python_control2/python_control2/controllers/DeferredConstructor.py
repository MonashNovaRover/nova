from typing import TypeVar, Optional, Type, Generic
from rclpy.node import Node
from ..controller_manager.Contexts import Contexts

T = TypeVar("T")

class DeferredConstructor(Generic[T]):
    def __init__(self, cls: Type[T], *deferred_args, **deferred_kwargs):
        self.cls = cls
        self.deferred_args = deferred_args
        self.deferred_kwargs = deferred_kwargs
        self.name: Optional[str] = None
        pass

    def construct(self, name: str, node: Node, contexts: Contexts) -> T:
        instance = object.__new__(self.cls)
        instance.name = name
        instance.node = node
        instance.logger = instance.node.get_logger()

        self.cls.__init__(instance, contexts, *self.deferred_args, **self.deferred_kwargs)

        return instance
