import numpy as np
import time


class Grid3D:
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

        # the last index stores (last_update, r, g, b)
        self.map = np.zeros((int(length / resolution),
                             int(width / resolution),
                             int(height / resolution),
                             4))

    def get_slices(self, pose_msg, len_down, len_up):
        """
        Gets slices from pose - len_down to pose + len_up. Returns a sub-set (by height) of the self.map
        """
        upper_index = int((pose_msg.pose.pose.position.z + self.height / 2 + len_up) / self.resolution)
        lower_index = int((pose_msg.pose.pose.position.z + self.height / 2 - len_down) / self.resolution)
        return np.sum(self.map[:, :, lower_index:upper_index, 0], axis=2) > 0

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
        Note: this version is slower due to loops, but still here as the newer version is un-tested
        Insert a point-cloud into the map
        :param points: (n, 3) numpy array
        :param colors: (n, 3) numpy array
        :return: None
        """
        t = time.time()
        indexes = self.get_indexes(points)  # returns a
        print("getting indexes took: " + str(time.time() - t))

        t = time.time()
        for i in range(len(points)):
            # only add if the translated point-cloud fits within the bounds of the map we have created
            if indexes[i][0] < self.map.shape[0] and indexes[i][1] < self.map.shape[1] and indexes[i][2] < self.map.shape[2]:
                self.map[indexes[i][0], indexes[i][1], indexes[i][2]] = np.append([t], colors[i])

        print("add_pc took: " + str(time.time() - t))

    def add_pc_fast(self, points, colors):
        """
        Still working on this version - currently doesn't add anything to the map unfortunately
        Insert a point-cloud into the map
        :param points: (n, 3) numpy array
        :param colors: (n, 3) numpy array
        :return: None
        """
        t = time.time()

        # parse only the indexes which are within the map bounds
        indexes = self.get_indexes(points) 
        
        indexes_indexes = np.all(np.array([indexes[:,0] < self.map.shape[0], indexes[:,1] < self.map.shape[1], indexes[:,2] < self.map.shape[2]]).transpose(), axis=1)
        print(indexes_indexes)

        indexes = indexes[indexes_indexes]
        colors = colors[indexes_indexes]

        print("getting indexes took: " + str(time.time() - t))

        t = time.time()
        
        print("indexes shape: " + str(indexes.shape))
        print(colors.shape)

        # fill the all the new points with the new timestamp
        self.map[indexes.transpose()[0], indexes.transpose()[1], indexes.transpose()[2]][:,0] = 1
        # fill all the new points with their colors
        self.map[indexes.transpose()[0], indexes.transpose()[1], indexes.transpose()[2]][:,1:] = colors
        
        print(np.sum(self.map))
        print("adding point-cloud took: " + str(time.time() - t))

    def get_as_pc(self):
        """
        Returns the map as a set of points and colours in a point-cloud
        Inefficient as needs to look through entire map :)))
        :return: ((n, 3) numpy array of points, (n, 3) numpy array of colors)
        """

        t = time.time()

        # the indexes (should be an nx3 array)
        raw_indexes = self.map[:, :, :, 0] > 0
        print(raw_indexes)

        points = np.array(np.where(raw_indexes)).transpose()
        colors = np.array(self.map[raw_indexes][:, 1:])

        print("Extracting points from map took: " + str(time.time() - t))
        print(points.shape)
        
        return self.get_points(np.array(points)), colors
