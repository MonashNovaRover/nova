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
        super().__init__(name, bus, canIdMask=0xff0,canIdMatch=id_<<4,aliveTimeout=-1)
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

        self.minInterval = 122
        self.resRatio = 1;
        self.incRatio = 1;
        # config
        self.hasResolver = int(bool(hasResolver)) # ensure this is 0 or 1
        # TODO: don't hard code this
        if bus == "can1": # ARM
            match (id_):
                case 1:
                    self.minInterval = 233
                    self.resRatio = 4
                    self.incRatio = 100
                case 2:
                    self.minInterval = 233
                    self.resRatio = 5.375
                    self.incRatio = 100
                case 3:
                    self.minInterval = 233
                    self.resRatio = 5.375
                    self.incRatio = 100
                case 4:
                    self.minInterval = 122
                    self.resRatio = 4
                    self.incRatio = 8
                case 5:
                    self.minInterval = 122
                    self.resRatio = 1.25
                    self.incRatio = 10
                case 6:
                    self.minInterval = 122
                    self.resRatio = 1.25
                    self.incRatio = 10
        else:
            panic

        self.EPR = 8192
        self.clock_speed = 100*1e6

        self.multiturn = multiturn


        # in radians, corresponding to 0x8000
        if self.multiturn:
            self.zero_offset = 0x2000
            self.max_pos = (2**4 /2)*math.pi; 
        else:
            self.zero_offset = 0x0
            self.max_pos = math.pi;

        self.max_vel = self.clock_speed * 2*math.pi / (self.minInterval*self.EPR)

        # these should be 2pi but something is wrong so its off by a factor of 2 and i have to compensate
        self.registerAttr("Pos", lambda:f"{360*self.pos/(math.pi*self.resRatio) :+06.1f}", 6, units="°")
        self.registerAttr("Vel", lambda:f"{360*self.vel/(math.pi*self.incRatio) :+06.1f}", 6, units="°/s")

        self.outbox = [] # messages to be sent in format of argument lambda functions

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
        pos = int(0x8000 * self.pos / self.max_pos) + self.zero_offset
        data.append((pos >> 8) & 0xff)
        data.append(pos & 0xff)

        data.append(0) # not implemented in electrial's firmware
        data.append(0)

        self.send_message(3, data, 4)

    def update(self):
        """Process anything that needs to be done before outputs update
        """
        pass # everything is in spin

    def spin(self):
        """Process any new messages and track our state.
        """
        super().spin() # process can messages

        while self.outbox:
            # send an outgoing message
            self.outbox.pop()()

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
        speed = speed / 2.4 # Jono Factor
        self.target_vel = self.max_vel*speed/0x8000
        self.pos_control = False
        self.vel_control = True
        self.prev_command_time = time.time()

    def pos_cmd(self, pos):
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
                position = int.from_bytes(message.data, signed=True, byteorder='big')-self.zero_offset;
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
                self.outbox.append(lambda: self.send_message(9, [var_index,self.hasResolver], 2))
            case 0x7: # MIN_INTERVAL
                self.outbox.append(lambda: self.send_message(9, [var_index, (self.minInterval>>8)&0xff, self.minInterval&0xff], 3))

    def send_message(self,command, data, dlc):
        """Send a telemetry or response message
        """
        assert len(data) == dlc;
        assert command < 0x10 and command >= 0;
        self.sendFrame(
                0x400 | self.id << 4 | command,
                data
                )

