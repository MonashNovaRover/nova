__package__ = "autonomous"
#!/usr/bin/python3
  

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team. Child class of the Mapper 
class that maps the 2d surroundings by simply 
fitting height maps over the maximum and minimum
values of the most recent section of the 3d point 
cloud sent by the Rover. 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: points_grid
TOPICS:
  
  - /camera/depth/color/points [sensor_msgs.msg.PointCloud2]
  - /t265/odom/sample
SERVICES:
  - 
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	autonomous
AUTHOR(S):	Max
CREATION:	17/02/2022
EDITED:		17/02/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - a lot 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

import mapping.mapper as mapper
import time
import numpy as np
import math_utils.transform as transform
from config.runtime_params import max_fov_angle

class HeightMapper(mapper.Mapper):
    def __init__(self, length=20, width=20, height=5, resolution=0.1, planner=None, _vis=True):

        # init node with node name points
        super().__init__(length, width, height, resolution, planner, _vis)
        self.map2d = mapper.Grid2D(self.length, self.width) 

    def update_map_pts_only(self, pts):
        """
        :param pts: np.array(n, 6) - refers to x,y,z,r,g,b
        """

        if pts.shape[0] < 10:
            return

        # transform the points
        if self.msg:
            # transforming to the global frame
            full_transform_pts = transform.transform_points(self.msg, pts)
            # transforming pitch and roll to flatten the map, but no yaw or translation
            no_yaw_pts = transform.transform_points_no_yaw(self.msg, pts)

            print(full_transform_pts)
            #self.map3d.add_pc_points_only(full_transform_pts)
            #map_pts = self.map3d.get_as_pc()
            
            obs = self.map2d.pc_to_obstacles(no_yaw_pts)
            rotated_obs = self.map2d.arrange_obstacles(self.msg, obs)
            self.map2d.add_obstacles(self.msg, rotated_obs)
            self.map2d.publish_grid()

            # setting colors proportional to the height of points - hopefully looks cool!
            """if self.vis:
                max_z = 10
                colors = np.array([(abs(map_pts[:, 2]) + 1 / max_z) * 250.0 % 250, np.full(len(map_pts), 0), abs(max_z - abs(map_pts[:,2]) - 1) * 250 % 250]).transpose()
                np.save('basicPCL.npy', colors) 
                # white mode
                # colors = np.array(np.full((len(pts), 3), 255))
                self.pc_pub.pub_pts_colors(map_pts, colors.astype(int))
"""
        if time.perf_counter() - self.previous_plan > 1:
            if self.planner:
                self.previous_plan = time.perf_counter()
                # NEW WAY - HEIGHT MAPPING
                self.planner.get_path(self.map2d.map.astype(float) / 100)


