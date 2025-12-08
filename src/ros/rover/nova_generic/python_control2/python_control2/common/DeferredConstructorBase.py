from abc import abstractmethod
from typing import TypeVar, Optional, Type, Generic, Dict, Any, final, ParamSpec, List, TypeVarTuple
import re
from rclpy.node import Node
from ..controller_manager.Contexts import Contexts

T = TypeVar("T")
P = ParamSpec("P")
PConstructWithoutInit = ParamSpec("PConstructWithoutInit")
TExtraPrefixArgs = TypeVarTuple("TExtraPrefixArgs")

def camel_to_snake(name: str) -> str:
    """ Converts upper camel case to snake case. """
    s1 = re.sub(r'(.)([A-Z][a-z]+)', r'\1 \2', name)
    s2 = re.sub(r'([a-z0-9])([A-Z])', r'\1 \2', s1)
    return s2.lower()

class DeferredConstructorBase(Generic[T, P, TExtraPrefixArgs, PConstructWithoutInit]):
    def __init__(self, cls: Type[T], *deferred_args: P.args, **deferred_kwargs: P.kwargs):
        """ Creates a deferred construct for the given class cls. """
        self.cls = cls
        self.deferred_args = deferred_args
        self.deferred_kwargs = deferred_kwargs
        self.deferred_members: Dict[str, Any] = {}
        self.name: Optional[str] = None

        self.instance: T = object.__new__(self.cls)

    def set_member(self, name: str, value: Any) -> None:
        self.deferred_members[name] = value

    def init_instance(self, *extra_prefix_args: TExtraPrefixArgs) -> None:
        type(self.instance).__init__(self.instance, *extra_prefix_args, *self.deferred_args, **self.deferred_kwargs)

    def construct_without_init(self, *args: PConstructWithoutInit.args, **kwargs: PConstructWithoutInit.kwargs) -> None:
        """ Constructs an instance of the class without calling __init__

        :param contexts: A collection that provides dependencies by base class.
        :param node: The node the constructed class should use to get parameters. Tries to use Node from contexts if not
        provided.
        :param name: The name of the constructed class instance. Uses self.name if None is given, and generates a name
        from the class name if self.name is also None
        :return: The instance of the class
        """
        # Ensure name and node have values
        # if name is None:
        #     name = self.name
        # if name is None:
        #     name = camel_to_snake(self.cls.__name__.lower())

        # Create the class instance (without calling __init__ yet)
        instance = object.__new__(self.cls)

        # Get additionally defined variables
        # You can set values of variables on the DeferredConstructor, and they will appear on the constructed class
        # instance because of this mechanism.
        # externally_set_attrs = {
        #     k: v for k, v in self.__dict__.items()
        #     if k not in DeferredConstructor.__dict__.keys()
        #        and k not in ["cls", "deferred_args", "deferred_kwargs"]
        # }
        # for key, value in externally_set_attrs.items():
        #     setattr(instance, key, value)

        for key, value in self.deferred_members.items():
            setattr(instance, key, value)

        # Set default attrs:
        # instance.name = name
        # instance.node = node
        # instance.logger = instance.node.get_logger().get_child(instance.name)
        # instance.state =

        # Finally run __init__
        return instance
