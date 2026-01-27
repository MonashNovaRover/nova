#!/usr/bin/env python3

import os
import fcntl
import time
import depthai as dai

import numpy as np

class NamedPipeSource(dai.node.ThreadedHostNode):
    def __init__(self):
        super().__init__()
        self.outputs = {}
        self.widths = {}
        self.heights = {}
        self.running = True
        self.createdpipes = []

    def createNamedPipeInput(self, location, width, height):
        output = self.createOutput()
        output.setPossibleDatatypes([
            (dai.DatatypeEnum.ImgFrame, True),
            (dai.DatatypeEnum.Buffer, False)
        ])

        try:
            os.mkfifo(location, 0o600)
            self.createdpipes.append(location)
        except FileExistsError:
            pass #print(location,"already exists, reusing")

        pipefd = os.open(location, os.O_RDONLY | os.O_NONBLOCK)
        print(f"opened pipe `{location}` as input...")

        pipeSize = int(open("/proc/sys/fs/pipe-max-size").read())
        # make the pipe big so we don't get partial writes - gst-namedpipe can't cope with that.
        fcntl.fcntl(pipefd, fcntl.F_SETPIPE_SZ, pipeSize)

        #os.read(pipefd, pipeSize) # clear pipe

        self.outputs[pipefd] = output
        self.widths[pipefd] = width
        self.heights[pipefd] = height
        return output

    def run(self):
        # TODO: can we use callbacks instead of polling our fds?
        while self.running:
            for fd in self.outputs:
                self.processPackets(fd)
            time.sleep(0.01)

        for fd in self.outputs:
            os.close(fd)
        for pipe in self.createdpipes:
            # this may not be the right choice if we don't want to have to restart gstreamer when we restart
            os.unlink(pipe)

    def onStop(self):
        self.running = False

    def __getValidSize(self, width, height):
        return (height *width * 3) //2

    def processPackets(self, fd):
        output = self.outputs[fd]

        try:
            size = os.read(fd, 4)
        except BlockingIOError:
            return # try again later
        if len(size) != 4:
            return
        size = int.from_bytes(size, byteorder="little")
        width = self.widths[fd]
        height = self.heights[fd]

        if self.__getValidSize(width,height) != size:
            print(f"invalid size width {width}, height {height}, size {size}") 
            #TODO: we can search for the 4 bytes with the correct size to sync up again.
            return


        data = os.read(fd, size)
        if len(data) != size:
            print("WARN: didn't get enough data", len(data), size)

        nv12 = np.frombuffer(data, dtype=np.uint8)

        frame = dai.ImgFrame()
        frame.setData(nv12)
        frame.setType(dai.ImgFrame.Type.NV12)
        # Changing image size at runtime doesn't work for some reason, you need to restart the whole thing :/
        frame.setWidth(width)
        frame.setHeight(height)
        # Send the message
        output.send(frame)


class NamedPipeSink(dai.node.ThreadedHostNode):
    def __init__(self):
        super().__init__()
        self.inputs = {} # fd: input

        self.lasttime = {}
        self.bytes = {}

        self.running = True
        self.createdpipes = []

    def createNamedPipeOutput(self, location):
        try:
            os.mkfifo(location, 0o600)
            self.createdpipes.append(location)
        except FileExistsError:
            pass #print(location,"already exists, reusing")
        print(f"opened pipe `{location}` as output...")

        # to open the fifo writeonly there must be a reader connected.
        # open it readonly as well temporarily to work around this
        # we will get BrokenPipeError if nobody is listening later
        # instead of an OSError now and no fd we can reuse.
        infd = os.open(location, os.O_RDONLY | os.O_NONBLOCK)
        outfd = os.open(location, os.O_WRONLY)
        os.close(infd)

        # make the pipe big so we don't get partial writes which gst-namedpipe gets confused by
        fcntl.fcntl(outfd, fcntl.F_SETPIPE_SZ, int(open("/proc/sys/fs/pipe-max-size").read()))

        self.inputs[outfd] = self.createInput(blocking=False)
        self.inputs[outfd].addCallback(lambda buffer: self.pollInput(outfd, buffer))
        self.lasttime[outfd] = time.time()
        self.bytes[outfd] = 0

        return self.inputs[outfd]

    def run(self):
        while self.running:
            time.sleep(0.1)

        for fd in self.inputs:
            os.close(fd)
        for pipe in self.createdpipes:
            os.unlink(pipe)

    def onStop(self):
        self.running = False

    def pollInput(self, fd, buffer):

        buffer = buffer.getData().tobytes()
        self.bytes[fd] += len(buffer)
        if (time.time() - self.lasttime[fd]) > 1:
            print(fd, self.bytes[fd]//1000, "kB/s") # TODO: print location?
            self.bytes[fd] = 0
            self.lasttime[fd]=time.time()

        try:
            packet = self.__mkPacket(buffer)
            os.write(fd, packet)
        except BrokenPipeError:
            pass # nobody was listening, drop the frame
        del buffer

    def __mkPacket(self, buffer):
        return len(buffer).to_bytes(4, byteorder="little")+buffer


