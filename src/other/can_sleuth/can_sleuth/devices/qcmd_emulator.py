from . import candevice
import jcan 

class QCMDEmulator(candevice.CanDevice):

    def __init__(self, name, bus, canIdList):
        super.__init__(name, bus)
        self.idList = canIdList
        #add callback functions 
        for i in range(len(self.idList)):
            self.addCallback(self.idList[i], self.on_message)

    def on_message(self, message:jcan.Frame):
        """
            Process can frame
        """

        cmd = message.id & 0xf

        match cmd:
            case 0x1:
                self.control(1, message.data)
            case 0x2:
                self.control(2, message.data)
            case 0x3:
                self.set_current_limit(1, message.data):
            case 0x4:
                self.set_current_limit(2, message.data):
        
    

    def control(motor:int, data: list[int])
        """
            Process qcmd control
        """
        pass

    def set_current_limit(motor, data: list[int])

    def spin(self):
        super.spin()
    

