#!/usr/bin/env python3

from dataclasses import dataclass
from enum import Enum, auto
import time
import depthai as dai

from namedpipe import NamedPipeSink, NamedPipeSource
from anaglyph import Anaglyph


"""
gst-launch-1.0 v4l2src device=/dev/video2 ! \
        "image/jpeg, width=640" ! decodebin ! videoconvert ! \
        "video/x-raw, format=NV12" ! namedpipesink location=/tmp/h264enc0
gst-launch-1.0 namedpipesrc location=/tmp/h264enc0_out ! "video/x-h264"  ! queue ! webrtcsink
"""


class MessageType(Enum):
    CAMERAS = auto
    ENCODERS = auto
    STATS = auto

@dataclass
class Camera:
    name: str
    path: str
    width: int
    height: int
    filterHint: str


@dataclass
class Encoder:
    name: str
    inputPath: str # /tmp/xxxx
    outputPath: str # /tmp/xxx_out
    width: int
    height: int
    inputFilter: str # "video/x-raw, encoding=NV12"
    outputFilter: str # "video/x-h264"


# TODO: make this all arguments from rosparams
inputPipeNames = ("h264enc0", "h264enc1")
inputPipeWidths = (640, 640)
inputPipeHeights = (480, 480)

oakCams = {
        "C": dai.CameraBoardSocket.CAM_A,
        #"L": dai.CameraBoardSocket.CAM_B,
        "R": dai.CameraBoardSocket.CAM_C,
        }
oakRes = (1920,1200)

# TODO: put anaglyph back in, add stereo depth

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
        for camName in oakCams:
            cam = pipeline.create(dai.node.Camera)
            cam.build(oakCams[camName])
            camOut = cam.requestOutput(size=oakRes, type=dai.ImgFrame.Type.NV12, fps=FPS)

            encoder = pipeline.create(dai.node.VideoEncoder)
            #videoEncoder.setBitrate(500*1024) # doesn't seem to have any effect
            encoder.setDefaultProfilePreset(FPS, PROFILE)

            camOut.link(encoder.input)

            pipename = f"/tmp/OAK_{camName}_out"
            encoder.out.link(sink.createNamedPipeOutput(pipename))
            cameras.append(
                    Camera(
                        name="OAK_"+camName,
                        path=pipename,
                        width=oakRes[0],
                        height=oakRes[1],
                        filterHint="video/x-h264"
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
                            inputPath=inpipename,
                            outputPath=outpipename,
                            width=width,
                            height=height,
                            inputFilter = f"video/x-raw, encoding=NV12, width={width}, height={height}",
                            outputFilter = "video/x-h264"
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


from multiprocessing import Process
import multiprocessing as mp
import signal
import os

if __name__ == "__main__":
    mp.set_start_method("spawn") # depthai hangs with fork :/
    # TODO: puppeteer this from our main ros process
    # get cameras
    # get encoders
    # get stats?
    pipe, pipeDepthai = mp.Pipe()
    p = Process(target=run, args=(pipeDepthai,))
    p.start()
    try:
        while True:
            if pipe.poll():
                type_, value = pipe.recv()
                print(type_, value)
            time.sleep(1)
    except KeyboardInterrupt:
        os.kill(p.pid, signal.SIGUSR1) # keyboard interrupt
    p.join()
