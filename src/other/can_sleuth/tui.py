
import curses

import output


class TUI(output.Output):
    """A Terminal User Interface for rendering the state of the system.
    """
    # TODO: sometimes we crash when the terminal is resized. I think it tries to draw stuff at locations that no longer exist.
    # TODO: need to make sure we don't hide error messages by accident with this.
    def __init__(self):
        # stuff from curses.wrapper
        self._stdscr = curses.initscr()
        curses.noecho()
        curses.cbreak()
        self._stdscr.keypad(1)
        try:
            curses.start_color()
        except curses.error:
            pass

        # additional options to configure the terminal
        self._stdscr.clear()
        self._stdscr.nodelay(1)
        curses.curs_set(0)

    def __del__(self):
        # stuff from curses.wrapper
        self._stdscr.keypad(0)
        curses.echo()
        curses.nocbreak()
        curses.endwin()

    def update(self, devices):
        windows = self._positionWindows(self._stdscr, devices, startHeight=0)

        char = None
        for dev in devices:
            win = windows[dev]

            win.box()
            win.addstr(0,1,f"<{dev.getName()}>")
            height = 1
            for attr in dev.attrs:
                # TODO: proper support for multiline attrs
                # we limit the string to the width of the box minus the border
                win.addnstr(height, 1, f"{attr.name}: {attr.value()}{attr.units}"+" "*attr.width, win.getmaxyx()[1]-2)
                height += attr.height
            win.refresh()

        self._stdscr.refresh()
        try:
            char = self._stdscr.getkey()
        except curses.error:
            pass # no key was pressed
        if char == "q":
            raise KeyboardInterrupt


    def _windowSize(self, device):
        height = 0
        width = len(device.getName())+2
        for attr in device.attrs:
            height += attr.height
            width = max(width, attr.width+len(attr.name)+2)
        return (width+2, height+2) # include border


    def _positionWindows(self, screen, devices, startHeight=0, startWidth=0):
        """Given the devices and how much room their attrs will take up,
        decide where the box on the terminal for each device should be.
        """
        x, y = startWidth, startHeight
        windows = {}
        nextX = startWidth
        for dev in devices:
            width, height = self._windowSize(dev)
            if not screen.enclose(height+y, width+x):
                x = nextX
                y = startHeight

            sub = screen.subpad(height,width, y, x)
            windows[dev] = sub
            y += height
            nextX = max(nextX, width+x)
        return windows



