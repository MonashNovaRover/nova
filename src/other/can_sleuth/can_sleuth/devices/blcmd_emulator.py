'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Can Sleuth / Simulator

BrushLess Can Motor Driver (BLCMD) Emulator

See also: https://github.com/MonashNovaRover/pics/tree/master/BLCMD.X

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
EDITED BY: Orlando Chamberlain
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''

import jcan
import time
import math

from . import candevice

class BLCMDEmulator(candevice.CanDevice):
    """BLCMD Emulator class
    """
    def __init__(self, name, id_, bus, hasResolver=True, multiturn=False):
        """Create a BLCMD emulator

        :param name: Display name for this blcmd
        :param id_: the id number for this blcmd
        :param bus: the name of the canbus to use
        :param hasResolver: is there a resolver for position feedback?
        """
        super().__init__(name, bus, canIdMask=0xff0,canIdMatch=id_<<4)
        self.id = id_

        for i in range(0xf):
            self.addCallback((self.id<<4)|i, self.on_message)

        self.command_timeout = 0.5 # seconds

        self.prev_command_time=-1
        self.lastTimestep = time.time()

        # state
        self.pos_control = False
        self.vel_control = False
        self.target_pos = 0;
        self.target_vel = 0;
        self.prev_pos = 0;
        self.pos = 0;
        self.vel = 0;

        # config
        self.hasResolver = int(bool(hasResolver)) # ensure this is 0 or 1
        self.minInterval = 122 # I don't know what this physically means

        self.multiturn = multiturn

        # in radians, corresponding to 0x8000
        self.max_pos = 4*math.pi; 
        self.max_vel = 2*math.pi;

        self.registerAttr("Pos", lambda:f"{360*self.pos/(2*math.pi) :+06.1f}", 6, units="°")
        self.registerAttr("Vel", lambda:f"{360*self.vel/(2*math.pi) :+06.1f}", 6, units="°/s")

    def telem1(self):
        """Send telemetry message 1
        """
        data = []
        vel = int(0x8000 * self.vel / self.max_vel)
        data.append((vel >> 8) & 0xff)
        data.append(vel & 0xff)
        data.append(0) # TODO: Qcurrent
        data.append(0)
        self.send_message(1, data, 4)

    def telem3(self):
        """Send telemetry message 3
        """
        data = []
        pos = int(0x8000 * self.pos / self.max_pos)
        data.append((pos >> 8) & 0xff)
        data.append(pos & 0xff)
        if (self.multiturn):
            turns = (pos << 2) % 0xffff
            data.append((turns >> 8 ) & 0xff)
            data.append(turns & 0xff)
        else:
            vel = int(0x8000 * self.vel / self.max_vel)
            data.append((vel >> 8 ) & 0xff)
            data.append(vel & 0xff)

        self.send_message(3, data, 4)

    def update(self):
        """Process anything that needs to be done before outputs update
        """
        pass # everything is in spin

    def spin(self):
        """Process any new messages and track our state.
        """
        super().spin() # process can messages

        newTime = time.time();
        delta = newTime - self.lastTimestep;
        prevCommandAge = newTime - self.prev_command_time

        if (prevCommandAge > self.command_timeout):
            delta = self.command_timeout
            timeout = True
        else:
            timeout = False

        self.prev_pos = self.pos

        if (self.pos_control):
            self.pos = self.target_pos

        if (self.vel_control):
            self.pos += self.target_vel*delta;

        self.vel = (self.pos - self.prev_pos) / delta

        self.lastTimestep = newTime;

        if (timeout):
            self.stop()

        self.telem3()
        self.telem1()

    def stop(self):
        self.pos_control = False
        self.vel_control = False

    def reset(self):
        self.stop();

    def twitch(self, is_forwards):
        if (is_forwards):
            self.vel_cmd(0x1fff)
        else:
            self.vel_cmd(-0x1fff)

    def vel_cmd(self, speed):
        # assume max vel is 2pi rads/s?
        self.target_vel = self.max_vel*speed/0x8000
        self.pos_control = False
        self.vel_control = True
        self.prev_command_time = time.time()

    def pos_cmd(self, pos):
        # -720 to + 720 deg
        # -4pi to + 4pi rads
        self.target_pos = self.max_pos *pos / 0x8000
        self.pos_control = True
        self.vel_control = False
        self.prev_command_time = time.time()


    def on_message(self, message):
        """Process a can message
        """
        msgType = message.id & 0xf
        
        match msgType:
            case 0x0: # all stop:
                self.stop()
            case 0x1: # twitch forwards
                self.twitch(True)
            case 0x2: # twitch back
                self.twitch(False)
            case 0x3: # drive at speed
                assert len(message.data) == 2
                speed = int.from_bytes(message.data, signed=True, byteorder='big');
                self.vel_cmd(speed)
            case 0x4: # drive to position
                assert len(message.data) == 2
                position = int.from_bytes(message.data, signed=True, byteorder='big');
                self.pos_cmd(position)
            case 0x5: # drive at current
                pass # current = data[0]<<8 | data[1]
            case 0x6: # drive open loop
                pass # current = data[0]<<8 | data[1]; velocity = data[2]<<8 | data[3];
            case 0x7: # home rotor
                pass
            case 0x8: # zero
                pass 
            case 0x9: # get cfg
                assert len(message.data) == 1
                var_index = int.from_bytes(message.data)
                self.get_cfg(var_index)
            case 0xa: # set cfg
                pass 
            case 0xb: # stop and reset
                self.reset()
            case 0xc: # reset resolver
                pass
            case 0xd: # read gate driver fault
                pass
            case 0xe: # manual zero
                pass # zero_position = (data[0]<<8 | data[1])>>2;

    def get_cfg(self, var_index):
        """Reply with configuration data
        """
        match var_index:
            case 0x0: # HAS_RESOLVER
                self.send_message(9, [var_index,self.hasResolver], 2);
            case 0x7: # MIN_INTERVAL
                self.send_message(9, [var_index, (self.minInterval>>8)&0xff, self.minInterval&0xff], 3);

    def send_message(self,command, data, dlc):
        """Send a telemetry or response message
        """
        assert len(data) == dlc;
        assert command < 0x10 and command >= 0;
        self.sendFrame(
                0x400 | self.id << 4 | command,
                data
                )

