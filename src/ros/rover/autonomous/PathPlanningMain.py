#!/usr/bin/env
import numpy as np
import matplotlib.pyplot as plt
from PIL import Image as image
from matplotlib.pyplot import figure
import heapq
import time
import pandas as pd

from ArrayMap import ArrayMap
from PathPlanner import PathPlanner

imgLenX = 20
imgLenY = 20

start = (15,11)
end = (10,2)


wayName = ['A', 'B', 'C'] 
wayPose = [[1,1], [3,3], [5,5]] 

#Process image
arc = ArrayMap('arc.png')
img_raw = arc.createCrop()
# arc.show()
arc.createDistribution(500)
arc.createObstacle(target = 207, above = 0, below = 4)
# arc.show()
scale_X, scale_Y = arc.createScale(imgLenX, imgLenY)
padding = 0.5
img_edited = arc.createPadding(int(np.floor(padding / scale_X)))
arc.show()

# Plan path
A_B = PathPlanner(img_edited, start, end)
A_B.scale(scale_X, scale_Y)
r1, r2 = A_B.run(4.2, waStar = True)
r3 = A_B.stringPull(img_edited, r1)
paths = [r1, r2, r3]#, r3, r4]
A_B.plot(img_raw, paths)


# Not the best way but will be redundant soon
wayPoints = {}
for i in range(len(wayName)):
    wayPoints[str(wayName[i])] = wayPose[i]
wayPoints

# Iterate path planning
frames = []
for i in range(len(wayPoints)):
    for j in range(len(wayPoints)):
        route = []
        start = wayPoints[wayName[i]]
        end = wayPoints[wayName[j]]
        if start != end:
            print(start)
            aStar = PathPlanner(img_edited, start, end)
            aStar.scale(scale_X, scale_Y)
            route = aStar.run(2.2)
            newdf = pd.DataFrame(route, columns = [str(wayName[i])+str(wayName[j])+'x', str(wayName[i])+str(wayName[j])+'y'])
            #print(route)
            print(route[0])

            frames.append(newdf)
result = pd.concat(frames, axis=1)
result = result.fillna(0)


#result.to_excel("output.xlsx")
