#!/usr/bin/env python3

import time
import socket
import depthai as dai

from io import BytesIO

from PIL import Image
import numpy as np

from udp import UdpSink, UdpSource
from anaglyph import Anaglyph

streamOak = True
streamUdp = False
udpInputPorts = (4996,) # we will listen to these and convert encode them
currentPort = 5000 # first output port, currently just use sequential ports.
oakRes = (1920,1200)

FPS=10
PROFILE = dai.VideoEncoderProperties.Profile.H264_MAIN # or H265_MAIN, H264_MAIN, MJPEG H264_BASELINE H264_HIGH

with dai.Pipeline(dai.Device(maxUsbSpeed=dai.UsbSpeed.SUPER_PLUS)) as pipeline:


    outputsToEncode = {}

    if streamOak:
        oakCams = {
                "C": dai.CameraBoardSocket.CAM_A,
                "L": dai.CameraBoardSocket.CAM_B,
                "R": dai.CameraBoardSocket.CAM_C,
                }
        for camName in oakCams:
            cam = pipeline.create(dai.node.Camera)
            cam.build(oakCams[camName])
            camOut = cam.requestOutput(size=oakRes, type=dai.ImgFrame.Type.NV12, fps=FPS)

            outputsToEncode[f"OAK {camName}"] = camOut

        anaglyph = Anaglyph()
        outputsToEncode["OAK C"].link(anaglyph.left)
        outputsToEncode["OAK R"].link(anaglyph.right)
        outputsToEncode["OAK 3D"] = anaglyph.output

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

