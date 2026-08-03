
from . import candevice

class ToggleHardwareEmulator(candevice.CanDevice):
    def __init__(self, name:str, id_:int, bus:str, on_command:int = 1, off_command:int=0, on_state:str = "on", off_state:str = "off", init_state:str = "off"):
        """
        """
        super.__init__(name, bus)
        self.id = id_
        self.state = init_state
        self.on_state = on_state
        self.off_state = off_state

        #register callback function for can id
        self.add_callback(self.id, self.on_message)

        #register state attribute 
        self.registerAttr(f"{self.name} state",self.attr_getter, 6, units = "")


    def update(self):
        pass

    def spin(self):
        #spin bus
        super().spin()

        #send updated state
        data = self.pack_data(self.state, 2)
        self.sendFrame(self.id, data)
    
    def on_message(self, message):
        """
        """
        match message.data:
            case self.off_command:
                self.state = self.off_state
            case self.on_command:
                self.state = self.on_state

    def attr_getter(self):
        return f"{self.state}"
