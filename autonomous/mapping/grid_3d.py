__package__ = "autonomous"
from config.runtime_params import min_point_density 
import numpy as np
import time


class Grid3D:
    def __init__(self, length, width, height, resolution, has_color=True):
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

        self.has_color = has_color

        if self.has_color:
            # the last index stores (last_update, r, g, b)
            self.map = np.zeros((int(length / resolution),
                                 int(width / resolution),
                                 int(height / resolution),
                                 4))

        else:
            # the last index stores (last_update)
            self.map = np.zeros((int(length / resolution),
                                 int(width / resolution),
                                 int(height / resolution),
                                 1))

    def get_slices(self, pose_msg, len_down, len_up):
        """
        Gets slices from pose - len_down to pose + len_up. Returns a sub-set (by height) of the self.map
        """
        upper_index = int((pose_msg.pose.pose.position.z + self.height / 2 + len_up) / self.resolution)
        lower_index = int((pose_msg.pose.pose.position.z + self.height / 2 - len_down) / self.resolution)
        return np.sum(self.map[:, :, lower_index:upper_index, 0], axis=2) > 0

    def extract_z(self, height_m):
        height_index = int(height_m / self.resolution)
        return self.map[:, :, height_index:].sum(axis=2).astype(bool).astype(float)
        #return self.map[:, :, height_index]

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
        ind = (pos / self.resolution).round().astype(int)

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
        Still working on this version - currently doesn't add anything to the map unfortunately
        Insert a point-cloud into the map
        :param points: (n, 3) numpy array
        :param colors: (n, 3) numpy array
        :return: None
        """
        t = time.time()

        # parse only the indexes which are within the map bounds
        indexes = self.get_indexes(points) 
        
        indexes_indexes = np.all(np.array([indexes[:, 0] < self.map.shape[0], indexes[:, 1] < self.map.shape[1], indexes[:, 2] < self.map.shape[2]]).transpose(), axis=1)

        indexes = indexes[indexes_indexes]
        colors = colors[indexes_indexes]

        # print("getting " + str(indexes.shape[0]) + " indexes took: " + str(time.time() - t) + "   (" + str(10000 * (time.time() - t) / indexes.shape[0]) + " s per 10k indexes)")

        # t = time.time()

        # fill the all the new points with the new timestamp and colors
        to_add = np.concatenate((np.full((colors.shape[0], 1), 1), colors), axis=1)
        self.map[indexes.transpose()[0], indexes.transpose()[1], indexes.transpose()[2]] = to_add

        # print("adding " + str(colors.shape[0]) + " points took: " + str(time.time() - t) + "   (" + str(10000 * (time.time() - t) / colors.shape[0]) + " s per 10k indexes)")
        # print("Map size: " + str(self.map.shape[0] * self.map.shape[1] * self.map.shape[2]))


    def add_pc_points_only(self, points):
        """
        Still working on this version - currently doesn't add anything to the map unfortunately
        Insert a point-cloud into the map
        :param points: (n, 3) numpy array
        :return: None
        """
        t = time.time()

        # parse only the indexes which are within the map bounds
        indexes = self.get_indexes(points)

        valid_indexes = np.all(np.array([indexes[:, 0] < self.map.shape[0], indexes[:, 1] < self.map.shape[1],
                                           indexes[:, 2] < self.map.shape[2]]).transpose(), axis=1)

        indexes = indexes[valid_indexes]

        # print("getting " + str(indexes.shape[0]) + " indexes took: " + str(time.time() - t) + "   (" + str(
        # 10000 * (time.time() - t) / indexes.shape[0]) + " s per 10k indexes)")

        t = time.time()

        # numpy god method - gives us back all the unique indices and the number of times
        # they repeated. For removing dodgy noise points
        indexes, counts = np.unique(indexes, return_counts=True, axis=0)
        counts //= min_point_density # anything below min_point_density -> 0
        self.map[indexes[:,0], indexes[:,1], indexes[:,2]] = counts.reshape(len(counts), 1)

        # print("adding " + str(indexes.shape[0]) + " points took: " + str(time.time() - t) + "   (" + str(
        # 10000 * (time.time() - t) / indexes.shape[0]) + " s per 10k indexes)")
        # print("Map size: " + str(self.map.shape[0] * self.map.shape[1] * self.map.shape[2]))

    def get_as_pc(self):
        """
        Returns the map as a set of points and colours in a point-cloud
        Inefficient as needs to look through entire map :)))
        :return: ((n, 3) numpy array of points, (n, 3) numpy array of colors)
        """

        t = time.time()

        if self.has_color:
            # the indexes (should be an nx3 array)
            raw_indexes = self.map[:, :, :, 0] > 0

            points = np.array(np.where(raw_indexes)).transpose()
            colors = np.array(self.map[raw_indexes][:, 1:])

            # print("Extracting points from map took: " + str(time.time() - t))

            return self.get_points(np.array(points)), colors

        else:
            # the indexes (should be an nx3 array)
            raw_indexes = self.map[:, :, :, 0] > 0

            points = np.array(np.where(raw_indexes)).transpose()

            #print("Extracting points from map took: " + str(time.time() - t))

            return self.get_points(np.array(points))
