
from . import candevice

class SensorEmulator(candevice.CanDevice):
    def __init__(self, name:str, id_:int, bus:str , attr: str, unit:str, initState:int, update_state):
        """
        """

        super().__init__(name, bus)
        self.id = id_
        self.state = initState
        self.update_state = update_state

        #register attributes with corresponding units
        self.registerAttr(attr, self.attr_getter, 6, units = unit)

    def update(self):
        pass
    
    def attr_getter(self):
        return str(f"{self.state} ")
    
    def spin(self):
        #spin bus
        super().spin()

        #update state 
        self.state = self.update_state(self.state)

        #send updated state
        data = self.pack_data(self.state, 2)
        self.sendFrame(self.id, data)

    
    def pack_data(self, data:int, byteLength:int)-> list[int]:
        return list(data.to_bytes(byteLength,'big', signed = True))

class SensorTracer(candevice.CanDevice):

    
        


    

