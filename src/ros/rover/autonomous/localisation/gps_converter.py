#!usr/bin/python3
__package__ = "autonomous"
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Converts between latitude and longitude (returned
 by dgps, and used to provide goal locations and
 display on gui), and x, y coordinates used in the
 Nova frame
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: gps_converter
TOPICS:
  - /autonomous/goal_coords [sensor_msgs/NavSatFix]
  - /autonomous/goal_pub    [sens
SERVICES:
  - /autonomous/dgps_to_xyz [PointTransform]
  - /autonomous/xyz_to_dgpz [PointTransform]
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	Autonomous
AUTHOR(S):	Max Tory
CREATION:	08/05/2021
EDITED:		08/05/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

import math
from geographiclib.geodesic import Geodesic
from geomag import declination


class GpsConverter():
    def __init__(self):
        # geodesic distance function
        self.g_to_d = Geodesic.WGS84.Inverse
        self.d_to_g = Geodesic.WGS84.Direct

        # The coordinate that we build our relative position around
        self.gps_0_coord = None
        # Offset of true north from magnetic north
        self.magnetic_declination = 0.

    def get_local_coord(self, lat, lon):
        """
        Take a gps coordinate and convert it into the local frame in m.
        x = North
        y = West
        +yaw = counter-clockwise
        """
        if self.gps_0_coord is None:
            self.gps_0_coord = (lat, lon)
            self.magnetic_declination = math.radians(declination(*self.gps_0_coord))
            return 0., 0.   # No local frame to compare to

        geodesic_dist_info = self.g_to_d(*self.gps_0_coord, lat, lon)
        dist = geodesic_dist_info['s12']   # geodesic distance
        # azimuth (compass heading) of the geodesic as seen from source and destination
        az1 = geodesic_dist_info['azi1']
        az2 = geodesic_dist_info['azi2']
        # average heading between the two points in the nova frame (counter-clockwise positive)
        # this hopefully gives us a first order, rather than 0 order approximation of the coordinates
        avg_yaw_nova = - (az1 + az2) * 0.5

        x = dist * math.cos(math.radians(avg_yaw_nova))
        y = dist * math.sin(math.radians(avg_yaw_nova))

        return x, y

    def get_global_coord(self, x, y):
        """
        Take coordinate in local frame and convert it into a global gps coordinate
        """
        if self.gps_0_coord is None:
            return None  # there is no starting coordinate to transform into
        # azimuth (compass heading) of the geodesic as seen from current location
        az1 = math.degrees(-math.atan2(y, x))
        dist = math.sqrt(x ** 2 + y ** 2)  # dist from 0

        geodesic_global_info = self.d_to_g(*self.gps_0_coord, az1, dist)

        return geodesic_global_info['lat2'], geodesic_global_info['lon2']


def main(args=None):
    rclpy.init(args=args)
    conversion_service = GpsConverter()
    rclpy.spin(conversion_service)
    rclpy.shutdown()


if __name__ == '__main__':
    main()
