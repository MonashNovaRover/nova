"""
Stores a voxel grid using a KDTree. 
Each Voxel 
"""

import time
from sklearn.neighbors import KDTree


class Voxel:
    
    def __init__(self, x, y, z, r, g, b):
        self.creation_time = time.time() 

        self.x = x
        self.y = y
        self.z = z

        self.r = r
        self.g = g
        self.b = b
    
    def __getitem__(self, index):
        if index == 0:
            return self.x
        elif index == 1:
            return self.y
        return self.z

    def __eq__(self, other):
        return self.x == other.x and self.y == other.y and self.z == other.z    

class D3Tree:
    """
    Note: this data structure acts as a node, so a full tree will be many of these linked recursively
    A KD Tree where k = 3 (for euclidean data)
    """

    def __init__(self, payload, axis=0, is_leaf=True):
        """
        :param resolution: the length of each cube
        """

        # this is the actual data being stored - it must be array-like
        self.payload = payload

        self.is_leaf = is_leaf

        self.left = None
        self.right = None

    def insert(payload):
        """
        recursively searches and creates new sub tree as needed
        """
        # check if exists, if so do nothing
        if self.payload == payload:
            return None
        
        # compare along axis
        if payload.x >= self.payload.x:
            # recurse right 
            pass
        else:
            # recurse left
            pass

