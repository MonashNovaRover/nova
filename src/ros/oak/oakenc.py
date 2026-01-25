#!/usr/bin/env python3

import time
import socket
import depthai as dai

from io import BytesIO

from PIL import Image
import numpy as np

streamOak = True
streamUdp = False
udpInputPorts = (4996,) # we will listen to these and convert encode them
currentPort = 5000 # first output port, currently just use sequential ports.

FPS=10
PROFILE = dai.VideoEncoderProperties.Profile.H264_MAIN # or H265_MAIN, H264_MAIN, MJPEG H264_BASELINE H264_HIGH


class UdpSource(dai.node.ThreadedHostNode):
    def __init__(self):
        super().__init__()
        self.outputs = {}
        self.reconstructedFrames = {}

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
        sock.bind((listenip, port))
        print(f"listening for packets on {listenip}:{port}...")

        self.outputs[(listenip, port)] = sock, output
        self.reconstructedFrames[(listenip,port)] = b''
        return output

    def run(self):
        while True:
            for output in self.outputs:
                self.processPackets(output)

    def processPackets(self, ipPort):
        sock, output = self.outputs[ipPort]
        data, sender = sock.recvfrom(2**16)
        if not data:
            return

        self.reconstructedFrames[ipPort] += data
        JPEG_START = b'\xff\xd8\xff\xe0'
        JPEG_END = b'\xff\xd9'

        start = self.reconstructedFrames[ipPort].find(JPEG_START)
        end = self.reconstructedFrames[ipPort].find(JPEG_END)
        # if end < start then i think we will drop a frame but that shouldn't occur.

        while start != -1 and end != -1:
            # we have a whole frame
            frame = self.reconstructedFrames[ipPort][start:end+len(JPEG_END)]
            self.reconstructedFrames[ipPort] = self.reconstructedFrames[ipPort][end+len(JPEG_END):]

            if (frame):
                self.send(frame, output)
        
            start = self.reconstructedFrames[ipPort].find(JPEG_START)
            end = self.reconstructedFrames[ipPort].find(JPEG_END)


    def send(self, data, output):
        img = Image.open(BytesIO(data))

        try:
            rgb = np.asarray(img)
        except OSError:
            return # bad frame, just skip it
        # TODO: if we can't send a frame, resend last good frame

        frame = dai.ImgFrame()
        frame.setData(rgb)
        frame.setType(dai.ImgFrame.Type.RGB888i)
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

    def createUDPOutput(self, port, ip="127.0.0.1"):
        self.inputs[(ip,port)] = self.createInput()
        self.lasttime[(ip,port)] = time.time()
        self.bytes[(ip,port)] = 0

        return self.inputs[(ip,port)]

    def run(self):
        while True:
            for input_ in self.inputs:
                self.pollInput(input_)

    def pollInput(self, ipPort):
        buffer = self.inputs[ipPort].get()
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



with dai.Pipeline(dai.Device(maxUsbSpeed=dai.UsbSpeed.SUPER_PLUS)) as pipeline:


    outputsToEncode = {}

    if streamOak:
        # OAK RGB Centre
        cam_rgb = pipeline.create(dai.node.Camera)
        cam_rgb.build(dai.CameraBoardSocket.CAM_A)
        camOut = cam_rgb.requestOutput(size=(1920,1200), type=dai.ImgFrame.Type.NV12, fps=FPS)

        outputsToEncode["OAK RGB"] = camOut

        # OAK RGB Left
        cam_L = pipeline.create(dai.node.Camera)
        cam_L.build(dai.CameraBoardSocket.CAM_B)
        outL = cam_L.requestOutput(size=(1920,1200), type=dai.ImgFrame.Type.NV12, fps=FPS)

        outputsToEncode["OAK L"] = outL

        # OAK RGB Right
        cam_R = pipeline.create(dai.node.Camera)
        cam_R.build(dai.CameraBoardSocket.CAM_C)
        outR = cam_R.requestOutput(size=(1920,1200), type=dai.ImgFrame.Type.NV12, fps=FPS)

        outputsToEncode["OAK R"] = outR

        """
        # OAK Depth
        stereo = pipeline.create(dai.node.StereoDepth)
        outL_mono = cam_L.requestOutput(size=(1280,800), type=dai.ImgFrame.Type.YUV400p, fps=FPS)
        outR_mono = cam_R.requestOutput(size=(1280,800), type=dai.ImgFrame.Type.YUV400p, fps=FPS)

        outL_mono.link(stereo.left)
        outR_mono.link(stereo.right)

        stereo.setRectification(True)
        stereo.setExtendedDisparity(True)
        stereo.setLeftRightCheck(True)

        manip = pipeline.create(dai.node.ImageManip)
        manip.initialConfig.setFrameType(dai.ImgFrame.Type.YUV400p) # this conversion doesn't seem to work.
        stereo.depth.link(manip.inputImage)

        outputsToEncode[f"OAK D"] = manip.out
        """


    # If one src stops then they all get frozen. maybe repeat last frame to avoid this?
    source = UdpSource()
    if streamUdp:
        for port in udpInputPorts:
            manip = pipeline.create(dai.node.ImageManip)
            manip.setMaxOutputFrameSize(1200*1980*2) #random number tbh. it means you can do bigger resolution but also it ends up broken with the lower section of the image missing...
            manip.initialConfig.setFrameType(dai.ImgFrame.Type.NV12)
            source.createUDPInput(port).link(manip.inputImage)

            outputsToEncode[f"UDP {port}"] = manip.out


    # ENCODERS & UDP OUTPUT
    sink = UdpSink()
    for name in outputsToEncode:
        output = outputsToEncode[name]

        # multiple streams goes bad at high framerates :/
        encoder = pipeline.create(dai.node.VideoEncoder)
        #videoEncoder.setBitrate(500*1024) # doesn't seem to have any effect
        encoder.setDefaultProfilePreset(FPS, PROFILE)

        output.link(encoder.input)

        encoder.out.link(sink.createUDPOutput(currentPort))
        currentPort+=1

    pipeline.start()

    # Doing nothing here, just keeping the host feeding the watchdog
    while pipeline.isRunning():
        try:
            # this is where you'd be clever and change the bitrate dynamically if changing the bitrate actually changed the encoder's output bitrate.
            time.sleep(0.1)
        except KeyboardInterrupt:
            break

