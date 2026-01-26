#!/usr/bin/env python3

import depthai as dai

import numpy as np
import cv2 as cv

def rgb2nv12(array):
    # numpy array 3d
    i420 = cv.cvtColor(array, cv.COLOR_RGB2YUV_I420)
    height = array.shape[0]
    width = array.shape[1]

    Y = i420[:height, :]
    U = i420[height:height+height//4, :]
    V = i420[height+height//4:, :]

    U = np.reshape(U, (-1,1))
    V = np.reshape(V, (-1,1))

    UV = np.hstack((U,V))

    UV = np.reshape(UV, (-1,width))

    #print(UV.shape)

    NV12 = np.vstack((Y,UV))
    return NV12





class Anaglyph(dai.node.ThreadedHostNode):
    """
    Red-Cyan 3D Glasses mode!
    """
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

            rgbL = bufferL.getCvFrame() # RGB
            rgbR = bufferR.getCvFrame() # RGB


            frame.setWidth(bufferL.getWidth())
            frame.setHeight(bufferL.getHeight())

            bufferL = np.reshape(bufferL.getData(), (bufferL.getWidth(),-1,1))
            bufferR = np.reshape(bufferR.getData(), (bufferR.getWidth(),-1,1))

            rR = rgbR[:,:,0]
            gL = rgbL[:,:,1]
            bL = rgbL[:,:,2]

            rgb = np.stack((rR,gL, bL), axis=2)

            nv12 = rgb2nv12(rgb)

            frame.setData(nv12)
            frame.setType(dai.ImgFrame.Type.NV12)

            self.output.send(frame)

