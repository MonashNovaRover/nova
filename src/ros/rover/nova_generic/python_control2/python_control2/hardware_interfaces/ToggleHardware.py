import jcan

from ..controller_manager.Interface import InterfaceCollection
from .HardwareInterface import HardwareInterface
from ..controller_manager.Contexts import Contexts
from teleop_python_utils import EventCollection


class ToggleHardware(HardwareInterface):

    def __init__(self, contexts:Contexts,can_id:int, on_command:int = 1, off_command:int=0, on_state:int=1, off_state:int=0; init_state:int=0):
        super().__init__(contexts)
        self.bus = contexts[jcan.Bus]
        self.state = init_state 
        self.on_state = on_state
        self.off_state =off_state
        self.can_id =self.declare_parameter("can_id", can_id).value
        self.on_command = self.declare_parameter("on_command", on_command, "command to turn on toggleable hardware").value
        self.off_command = self.declare_parameter("off_command". off_command, "command to turn off toggleable hardware").value

        #setup a toggle event 
        if EventCollection in contexts:
            events = contexts[EventCollection]
            events[f"{self.name}/toggle"].add_callback(self.on_toggle)

    
    def on_configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection)-> Optional[bool]:
        pass

    def on_read(self, now: float, period: float):
        pass

    def on_write(self, now: float, period: float):
        pass

    def on_toggle(self):
        match self.state:
            case self.on_state:
                self.state = self.off_state
                frame = jcan.Frame(id=self.can_id, data=self.off_command)
            case self.off_state:
                self.state = self.on_state
                frame = jcan.Frame(id=self.can_id, data=self.on_command)
        self.bus.send(frame)



    