
from candevice import candevice
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
        super.__init__(name, interface)
        self.brightness = 0
        self.colour=None
        #add callback functions
        #brightness
        self.addCallback(LedCommand.BRIGHTNESS, self.set_brightness_cb)
        self.addCallback(LedCommand.COLOUR, self.set_colour_cb)


    
    def set_brightness_cb(self, frame:jcan.Frame):
        #this is kind of hardcoding, but oh well
        self.brightness = commandsDict[frame.id][frame.data[0]]

    def set_colour_cb(self, frame:jcan.Frame):
        #TODO: Handle pink
        #TODO: Handle data being integer list of hexes
        print(frame.data)
        self.colour = commandsDict[frame.id][frame.data[0]]


# def main():
#     led = LEDStrip("led", "can0")
#     while true:
#         led.spin()

# if __name__=="__main__":
#     main()
