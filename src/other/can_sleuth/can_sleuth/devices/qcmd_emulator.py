from . import candevice
import jcan 

class QCMDEmulator(candevice.CanDevice):


    def __init__(self, name, bus, canIdList):
        super.__init__(name, bus)
        self.idList = canIdList
        self.motors = {}
        #add callback functions and motors to motor dictionary
        for i in range(len(self.idList)):
            self.addCallback(self.idList[i], self.on_message)
            self.motors[str(self.idList[i] & 0xf0)][1 if ]

    def on_message(self, message:jcan.Frame):
        """
            Process can frame
        """
        cmd = message.id & 0xf
        controllerId = str(message.id & 0xf0)
        self.control(controllerId,cmd, self.pack_can_data(message.data))

    def control(controller:str, motorCmd:int, data:int)
        """
            Process qcmd control
        """
        match motorCmd:
            case 1:
                self.set_speed(controller, data, 1)
            case 2:
                self.set_speed(controller, data, 2)
            case 3:
                self.set_current_limit(controller, data, 1)
            case 4:
                self.set_current_limit(controller, data,2)
            
    def set_speed(controllerId,speed, motor):
        pass

    def set_current_limit(controllerId, currentLim, motor):
        pass
    
    def pack_can_data(data:list[int]):
        return int.from_bytes(data, signed = True, byteorder='big')


    def spin(self):
        super.spin()
    

