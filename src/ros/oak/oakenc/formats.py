
import cv2 as cv
import numpy as np

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
