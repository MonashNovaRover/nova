
from . import candevice
from enum import Enum
import jcan

class LEDStrip(candevice.CanDevice):
    class LedCommand(Enum):
        BRIGHTNESS = 0x091
        COLOUR = 0x095
        PINK = 0x096

    commandsDIct = {
        #Brightness
        LedCommand.BRIGHTNESS: {
            0x80: 1, #100%
            0x60: 0.75, #75%
            0x40: 0.5, #50%
            0x00: 0 # off
        },
        #colour
        LedCommand.COLOUR: {
            0x01: "red",
            0x02: "green",
            0x03: "blue"
        },
        LedCommand.PINK: "pink"
    }
    
    def __init__(self, name:str, interface):
        super().__init__(name, interface)
        self.brightness = 0
        self.colour= None
        #add callback functions
        self.addCallback(LEDStrip.LedCommand.brightness.value, self.set_brightness_cb) #brightness
        self.addCallback(LEDStrip.LedCommand.COLOUR.value, self.set_colour_cb) #colour
        #register attributes
        self.registerAttr("brightness", self.get_brightness, 3)
        self.registerAttr("colour", self.get_colour, 10)
    
    def update(self):
        pass

    def set_brightness_cb(self, frame:jcan.Frame):
        self.brightness = commandsDict[frame.id][frame.data[0]]

    def set_colour_cb(self, frame:jcan.Frame):
        print(frame.data)
        #pink
        if frame.dlc== 0 and frame.id == LedCommand.PINK: 
            self.colour = commandsDict[LedCommand.PINK]
        else:
            self.colour = commandsDict[frame.id][frame.data[0]]
    
    #getters for tracing
    def get_colour(self):
        return str(self.colour)

    def get_brightness(self):
        return str(self.brightness)