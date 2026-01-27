#!/usr/bin/env python3

import time
import socket
import depthai as dai

from io import BytesIO

from PIL import Image
import numpy as np

from namedpipe import NamedPipeSink, NamedPipeSource
from anaglyph import Anaglyph


"""
gst-launch-1.0 v4l2src device=/dev/video2 ! \
        "image/jpeg, width=640" ! decodebin ! videoconvert ! \
        "video/x-raw, format=NV12" ! namedpipesink location=/tmp/h264enc0
gst-launch-1.0 namedpipesrc location=/tmp/h264enc0_out ! "video/x-h264"  ! queue ! webrtcsink
"""

inputPipeNames = ("h264enc0", "h264enc1")
# TODO: specify heights as well.
inputPipeWidths = (640, 640)

oakCams = {
        "C": dai.CameraBoardSocket.CAM_A,
        #"L": dai.CameraBoardSocket.CAM_B,
        "R": dai.CameraBoardSocket.CAM_C,
        }
oakRes = (1920,1200)

doAnaglyph = 0

FPS=20
PROFILE = dai.VideoEncoderProperties.Profile.H264_MAIN # or H265_MAIN, H264_MAIN, MJPEG H264_BASELINE H264_HIGH

#TODO: support 2 oak cameras at once
# TODO: bitrate: investigate VBR CBR settings

def run():
    with dai.Pipeline(dai.Device(maxUsbSpeed=dai.UsbSpeed.SUPER)) as pipeline:
        outputsToEncode = {}

        for camName in oakCams:
            cam = pipeline.create(dai.node.Camera)
            cam.build(oakCams[camName])
            camOut = cam.requestOutput(size=oakRes, type=dai.ImgFrame.Type.NV12, fps=FPS)

            outputsToEncode[f"OAK_{camName}"] = camOut

        if doAnaglyph:
            anaglyph = Anaglyph()
            outputsToEncode["OAK_C"].link(anaglyph.left)
            outputsToEncode["OAK_R"].link(anaglyph.right)
            outputsToEncode["OAK_3D"] = anaglyph.output

        source = NamedPipeSource()
        for i, name in enumerate(inputPipeNames):
            outputsToEncode[name] = source.createNamedPipeInput(f"/tmp/{name}", width=inputPipeWidths[i])


        # ENCODERS & UDP OUTPUT
        sink = NamedPipeSink()
        for name in outputsToEncode:
            output = outputsToEncode[name]

            encoder = pipeline.create(dai.node.VideoEncoder)
            #videoEncoder.setBitrate(500*1024) # doesn't seem to have any effect
            encoder.setDefaultProfilePreset(FPS, PROFILE)

            output.link(encoder.input)

            encoder.out.link(sink.createNamedPipeOutput(f"/tmp/{name}_out"))
        pipeline.start()
        try:
            while pipeline.isRunning():
                time.sleep(1)
        except KeyboardInterrupt:
            pipeline.stop()


from multiprocessing import Process
import multiprocessing as mp
import signal
import os

if __name__ == "__main__":
    mp.set_start_method("spawn") # depthai hangs with fork :/
    # TODO: puppeteer this from our main ros process
    p = Process(target=run, args=())
    p.start()
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        os.kill(p.pid, signal.SIGUSR1) # keyboard interrupt
    p.join()
