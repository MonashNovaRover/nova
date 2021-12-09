import numpy as np
import time


class ArrayGrid:
    def __init__(self, length, width, height, resolution):

        """
        An array grid is a dense representation of a 3D occupancy grid
        :param length: refers to the x direction in a left handed coordinate system
        :param width: refers to the y direction a in left handed coordinate system
        :param height: refers to the z (up) direction
        :param resolution: ok then
        """

        # these are how large we allow our map to be in meters.
        self.length = length
        self.width = width
        self.height = height

        # this is the length of each cube (grid cell) in meters
        self.resolution = resolution

        # the last index 5 stores (count, last_update, r, g, b)
        self.map = np.zeros((int(length / resolution),
                             int(width / resolution),
                             int(height / resolution),
                             5))

    def get_indexes(self, points):
        """
        Gets the array indexes of 3D floating point points.
        We assume that (0.0,0.0,0.0) lies in the center of self.map
        - i.e. (self.length // 2, self.width // 2, self.height // 2) is the array index of (0.0, 0.0, 0.0),
        - i.e. (self.length // 2 + a, self.width // 2 + b, self.height // 2 + c)
         is the array index of (self.resolution * a, self.resolution * b, self.resolution * c), and so forth
        :param points: an (n, 3) numpy array
        :return: an (n, 3) numpy array of indexes
        """

        # standardising to be all positive values
        pos = points + np.array([self.length / 2, self.width / 2, self.height / 2])

        # getting indexes simply by dividing by resolution and taking as integer
        ind = (pos / self.resolution).astype(int)

        return ind

    def get_points(self, indexes):
        """
        Gets the points (bottom left corner of a voxel) corresponding to a set of indexes
        :param indexes: an (n, 3) numpy array of indexes
        :return: an (n, 3) numpy array of points referred to by those indexes
        """
        return indexes * self.resolution - np.array([self.length / 2, self.width / 2, self.height / 2])

    def add_pc(self, points, colors):
        """
        Insert a pc
        :param points: (n, 3) numpy array
        :param colors: (n, 3) numpy array
        :return: None
        """
        t = time.time()
        indexes = self.get_indexes(points)
        print("getting indexes took: " + str(time.time() - t))

        # todo: if there's a way to do this efficiently (i.e. without looping) that would probably save a lot of time
        for i in range(len(points)):
            
            # only add if the translated point-cloud fits within the bounds of the map we have created
            if indexes[i][0] < self.map.shape[0] and indexes[i][1] < self.map.shape[1] and indexes[i][2] < self.map.shape[2]:
                count = self.map[indexes[i][0], indexes[i][1], indexes[i][2]][0]
                self.map[indexes[i][0], indexes[i][1], indexes[i][2]] = np.append([count + 1, t], colors[i])
        
        print("add_pc took: " + str(time.time() - t))

    def get_as_pc(self):
        """
        Returns the map as a set of points and colours in a point-cloud
        Inefficient as needs to look through entire map :)))
        :return: ((n, 3) numpy array of points, (n, 3) numpy array of colors)
        """

        t = time.time()
        points = []
        colors = []
        for l in range(self.map.shape[0]):
            for w in range(self.map.shape[1]):
                for h in range(self.map.shape[2]):
                    if self.map[l, w, h][0]:
                        points.append([l, w, h])
                        colors.append(self.map[l, w, h, 2:])
        print("looking through map took: " + str(time.time() - t))

        return self.get_points(np.array(points)), np.array(colors)
