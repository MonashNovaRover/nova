from . import candevice
import jcan

class LEDStrip(candevice.CanDevice):
    class LedCommand(Enum):
        BRIGHTNESS = 0x091
        COLOUR = 0x095
        PINK = 0x096

    commandsDIct = {
        #Brightness
        LedCommand.BRIGHTNESS: {
            0x8000: 1, #100%
            0x6000: 0.75, #75%
            0x4000: 0.5 #50%
            0x0000: 0 # off
        },
        #colour
        LedCommand.COLOUR: {
            0x0100: "red",
            0x0200: "green",
            0x0300: "blue"
        },
        LedCommand.PINK: "pink"
    }

    
    def __init__(self, name:str, interface:str):
        super.__init__(name, interface)
        self.brightness = 0
        self.colour=None
        #add callback functions
        #brightness
        self.addCallback(LedCommand.BRIGHTNESS, self.set_brightness_cb)
        self.addCallback(LedCommand.COLOUR, self.set_colour_cb)


    
    def set_brightness_cb(self, frame:jcan.Frame)
        self.brightness = commandsDict[frame.id]

    def set_colour_cb(self, frame:jcan.Frame):
        #TODO: Handle pink
        #TODO: Handle data being integer list of hexes
        self.colour = commandsDict[frame.id][frame.data]