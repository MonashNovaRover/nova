#!/usr/bin/env python3

import socket
import time
import depthai as dai

from io import BytesIO

from PIL import Image
import numpy as np

from oakenc.formats import rgb2nv12


class UdpSource(dai.node.ThreadedHostNode):
    def __init__(self):
        super().__init__()
        self.outputs = {}
        self.partialFrames = {}
        self.running = True

    def createUDPInput(self, port, listenip="127.0.0.1"):
        output = self.createOutput()
        output.setPossibleDatatypes([
            (dai.DatatypeEnum.ImgFrame, True),
            (dai.DatatypeEnum.Buffer, False)
        ])

        # gstreamer can only do udp, not named pipe...
        # need https://github.com/aler9/gst-namedpipe (or from c,
        # maybe you can set the socket for udpsink manually to a named pipe socket)
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.setblocking(False)
        sock.bind((listenip, port))
        print(f"listening for packets on {listenip}:{port}...")

        self.outputs[(listenip, port)] = sock, output
        self.partialFrames[(listenip,port)] = b''
        return output

    def run(self):
        # TODO: can we use callbacks instead of polling our udp sockets?
        while self.running:
            for output in self.outputs:
                self.processPackets(output)
            time.sleep(0.01)


    def onStop(self):
        self.running = False

    def processPackets(self, ipPort):
        sock, output = self.outputs[ipPort]
        try:
            data = sock.recv(2**16)
        except socket.error:
            return

        self.partialFrames[ipPort] += data
        JPEG_START = b'\xff\xd8\xff\xe0'
        JPEG_END = b'\xff\xd9'

        start = self.partialFrames[ipPort].find(JPEG_START)
        end = self.partialFrames[ipPort].find(JPEG_END)
        # if end < start then i think we will drop a frame but that shouldn't occur.

        while start != -1 and end != -1:
            # we have a whole frame
            frame = self.partialFrames[ipPort][start:end+len(JPEG_END)]
            self.partialFrames[ipPort] = self.partialFrames[ipPort][end+len(JPEG_END):]

            if (frame):
                self.send(frame, output)
        
            start = self.partialFrames[ipPort].find(JPEG_START)
            end = self.partialFrames[ipPort].find(JPEG_END)


    def send(self, data, output):
        img = Image.open(BytesIO(data))

        try:
            rgb = np.asarray(img)
        except OSError:
            return # bad frame, just skip it

        nv12 = rgb2nv12(rgb)
        frame = dai.ImgFrame()
        frame.setData(nv12)
        frame.setType(dai.ImgFrame.Type.NV12)
        # Changing image size at runtime doesn't work for some reason, you need to restart the whole thing :/
        frame.setWidth(img.width)
        frame.setHeight(img.height)
        # Send the message
        output.send(frame)


class UdpSink(dai.node.ThreadedHostNode):
    def __init__(self):
        super().__init__()
        self.inputs = {} # (ip,port): input

        self.lasttime = {}
        self.bytes = {}

        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.running = True

    def createUDPOutput(self, port, ip="127.0.0.1"):
        self.inputs[(ip,port)] = self.createInput(blocking=False)
        self.inputs[(ip,port)].addCallback(lambda buffer: self.pollInput((ip,port), buffer))
        self.lasttime[(ip,port)] = time.time()
        self.bytes[(ip,port)] = 0

        return self.inputs[(ip,port)]

    def run(self):
        while self.running:
            time.sleep(0.1)


    def onStop(self):
        self.running = False

    def pollInput(self, ipPort, buffer):

        buffer = buffer.getData().tobytes()
        self.bytes[ipPort] += len(buffer)
        if (time.time() - self.lasttime[ipPort]) > 1:
            print(ipPort, self.bytes[ipPort]//1000, "kB/s")
            self.bytes[ipPort] = 0
            self.lasttime[ipPort]=time.time()

        # deal with limits on udp payload size.
        # https://github.com/aler9/gst-namedpipe would work around this but we'd have to package it.

        idx = 0
        fragmentsize= 2**16 - 1000
        while idx < len(buffer):
            self.sock.sendto(buffer[idx:(idx+fragmentsize)], ipPort)
            idx += fragmentsize

        del buffer

