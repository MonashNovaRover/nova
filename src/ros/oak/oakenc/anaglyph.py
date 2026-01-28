#!/usr/bin/env python3

import depthai as dai

import numpy as np
import cv2 as cv

from oakenc.formats import rgb2nv12

class Anaglyph(dai.node.ThreadedHostNode):
    """
    Red-Cyan 3D Glasses mode!
    """
    def __init__(self):
        super().__init__()
        self.left = self.createInput(blocking=False)
        self.right = self.createInput(blocking=False)
        self.output = self.createOutput()

        self.running = True

    def onStop(self):
        self.running = False

    def run(self):
        while self.running:
            frame = dai.ImgFrame()

            bufferL = self.left.get()
            bufferR = self.right.get()

            bgrL = bufferL.getCvFrame()
            bgrR = bufferR.getCvFrame()


            frame.setWidth(bufferL.getWidth())
            frame.setHeight(bufferL.getHeight())

            rL = bgrL[:,:,2]
            gR = bgrR[:,:,1]
            bR = bgrR[:,:,0]

            rgb = np.stack((rL,gR, bR), axis=2)

            nv12 = rgb2nv12(rgb.astype(np.uint8))

            frame.setData(nv12)
            frame.setType(dai.ImgFrame.Type.NV12)

            self.output.send(frame)

