#!/usr/bin/env python3

from dataclasses import dataclass
from enum import Enum, auto
import time
import depthai as dai

from camera_msgs.msg import Camera, Cameras, Encoder, Encoders

from oakenc.namedpipe import NamedPipeSink, NamedPipeSource
from oakenc.anaglyph import Anaglyph


"""
gst-launch-1.0 v4l2src device=/dev/video2 ! \
        "image/jpeg, width=640" ! decodebin ! videoconvert ! \
        "video/x-raw, format=NV12" ! namedpipesink location=/tmp/h264enc0
gst-launch-1.0 namedpipesrc location=/tmp/h264enc0_out ! "video/x-h264"  ! queue ! webrtcsink
"""


class MessageType(Enum):
    CAMERAS = 0
    ENCODERS = auto
    STATS = auto


# TODO: make this all arguments from rosparams
inputPipeNames = ("h264enc0", "h264enc1")
inputPipeWidths = (640, 640)
inputPipeHeights = (480, 480)

oakCams = {
        "C": dai.CameraBoardSocket.CAM_A,
        #"L": dai.CameraBoardSocket.CAM_B,
        "R": dai.CameraBoardSocket.CAM_C,
        #"3D": None # this is really expensive on cpu, need to optimise (use gpu?)
        }
oakRes = (1920,1200)

# TODO: add stereo depth

FPS=20
PROFILE = dai.VideoEncoderProperties.Profile.H264_MAIN # or H265_MAIN, H264_MAIN, MJPEG H264_BASELINE H264_HIGH

#TODO: support 2 oak cameras at once
# TODO: bitrate: investigate VBR CBR settings

def run(pipe):
    with dai.Pipeline(dai.Device(maxUsbSpeed=dai.UsbSpeed.SUPER)) as pipeline:
        source = NamedPipeSource()
        sink = NamedPipeSink()

        outputsToEncode = {}
        cameras = []
        encoders = []

        # Cameras
        anaglyphLeft = "C"
        anaglyphRight = "R"
        for camName in oakCams:
            if camName == "3D":
                a = Anaglyph()
                left.link(a.left)
                right.link(a.right)
                camOut = a.output

            else:
                cam = pipeline.create(dai.node.Camera)
                cam.build(oakCams[camName])
                camOut = cam.requestOutput(size=oakRes, type=dai.ImgFrame.Type.NV12, fps=FPS)
            if camName == anaglyphLeft:
                left = camOut
            if camName == anaglyphRight:
                right = camOut

            encoder = pipeline.create(dai.node.VideoEncoder)
            #videoEncoder.setBitrate(500*1024) # doesn't seem to have any effect
            encoder.setDefaultProfilePreset(FPS, PROFILE)

            camOut.link(encoder.input)

            pipename = f"/tmp/OAK_{camName}_out"
            encoder.out.link(sink.createNamedPipeOutput(pipename))
            cameras.append(
                    Camera(
                        serial="OAK_"+camName,
                        node=pipename,
                        outputcaps="video/x-h264"
                        )
                    )

        # Encoders
        for i, name in enumerate(inputPipeNames):
            inpipename = f"/tmp/{name}"
            width = inputPipeWidths[i]
            height = inputPipeHeights[i]
            output = source.createNamedPipeInput(inpipename, width=width, height=height)

            encoder = pipeline.create(dai.node.VideoEncoder)
            #videoEncoder.setBitrate(500*1024) # doesn't seem to have any effect
            encoder.setDefaultProfilePreset(FPS, PROFILE)

            output.link(encoder.input)

            outpipename = f"/tmp/{name}_out"
            encoder.out.link(sink.createNamedPipeOutput(outpipename))
            encoders.append(
                    Encoder(name=name,
                            inputpath=inpipename,
                            outputpath=outpipename,
                            inputcaps = f"video/x-raw, encoding=NV12, width={width}, height={height}",
                            outputcaps = "video/x-h264"
                            )
                    )

        pipeline.start()

        pipe.send((MessageType.ENCODERS,encoders))
        pipe.send((MessageType.CAMERAS,cameras))
        #TODO: put telemetry in pipe

        try:
            while pipeline.isRunning():
                time.sleep(1)
        except KeyboardInterrupt:
            pipeline.stop()


