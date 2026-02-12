'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Can Sleuth / Simulator

Outputs

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
EDITED BY: Orlando Chamberlain
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''

from . import tui
from . import gui
from . import terminal_out

def default_output():
    try:
        return gui.GUI()
    except RuntimeError:
        print("Graphical output not availible, defaulting to terminal output.")
        return tui.TUI()

# List of everything for help message:
allOutputs = {
        "tui": tui.TUI,
        "gui": gui.GUI,
        "txt": terminal_out.TerminalOut,
        }

