from . import candevice
import jcan

class LEDStrip(candevice.CanDevice):
    commands = {
        #Brightness
        0x091: {
            0x8000: 1, #100%
            0x6000: 0.75, #75%
            0x4000: 0.5 #50%
        },
        #colour
        0x095: {
            0x0100: "red"
        }
    }
    
    def __init__(self, name:str, interface:str):
        super.__init__(name, interface)
        self.brightness = 0
        self.colour=None
    
    def set_brightness_cb(self, frame:jcan.Frame)
        
        pass

    def set_colour_cb(self, frame:jcan.Frame):
        pass
    
    def spin():
        pass