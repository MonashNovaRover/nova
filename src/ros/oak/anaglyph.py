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

            bgrL = bufferL.getCvFrame()
            bgrR = bufferR.getCvFrame()


            frame.setWidth(bufferL.getWidth())
            frame.setHeight(bufferL.getHeight())


            # slower but more accurate? if we can run a shader on oak we'd want to do this I think
            #https://github.com/dolphin-emu/dolphin/blob/master/Data/Sys/Shaders/Anaglyph/dubois.glsl

            bR = bgrR[:,:,0]
            gR = bgrR[:,:,1]
            rL = bgrL[:,:,2]

            rgb = np.stack((rL,gR, bR), axis=2)

            nv12 = rgb2nv12(rgb.astype(np.uint8))

            frame.setData(nv12)
            frame.setType(dai.ImgFrame.Type.NV12)

            self.output.send(frame)

