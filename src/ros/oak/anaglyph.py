#!/usr/bin/env python3

import depthai as dai

import numpy as np
import cv2 as cv



class Anaglyph(dai.node.ThreadedHostNode):
    def __init__(self):
        super().__init__()
        self.left = self.createInput()
        self.right = self.createInput()
        self.output = self.createOutput()

    def run(self):
        while True:
            frame = dai.ImgFrame()

            bufferL = self.left.get()
            bufferR = self.right.get()
            frameL = bufferL.getCvFrame()
            frameR = bufferR.getCvFrame()

            frame.setWidth(bufferL.getWidth())
            frame.setHeight(bufferL.getHeight())

            bufferL = bufferL.getData()
            bufferR = bufferR.getData()
            # COLOR_YUV2RGB_NV12 
            # COLOR_RGB2YUV_I420
            print(1, type(frameL), frameL.shape)

            rgbL = cv.cvtColor(frameL, cv.COLOR_YUV2RGB_NV12)
            print(2)
            nv12L = cv.cvtColor(rgbL, cv.COLOR_RGB2YUV_I420)
            print(3)

            # convert NV12 -> RGB
            # extract channels etc


            frame.setData((nv12L))
            frame.setType(dai.ImgFrame.Type.NV12)

            self.output.send(frame)

