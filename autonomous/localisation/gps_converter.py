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

import rclpy
from rclpy.node import Node
import math
from geographiclib.geodesic import Geodesic
from core.msg import AutonomousInfo
from core.srv import PointTransform
from config.ros_config import auto_goals_info, auto_goals_gps, xyz_to_gps_topic, gps_to_xyz_topic


class GpsConverter(Node):
    def __init__(self):
        super().__init__("gps_converter")
        # geodesic distance function
        self.g_to_d = Geodesic.WGS84.Inverse
        self.d_to_g = Geodesic.WGS84.Direct

        # The coordinate that we build our relative position around
        self.gps_0_coord = None

        self.goal_subscriber = self.create_subscription(AutonomousInfo, auto_goals_gps, self.goal_callback, 10)
        self.goal_publisher = self.create_publisher(AutonomousInfo, auto_goals_info, 10)

        self.gps_to_point_service = self.create_service(PointTransform, gps_to_xyz_topic, self.g_to_d_service)
        self.point_to_gps_service = self.create_service(PointTransform, xyz_to_gps_topic, self.d_to_g_service)

    def get_local_coord(self, lat, lon):
        """
        Take a gps coordinate and convert it into the local frame in m.
        x = North
        y = West
        +yaw = counter-clockwise
        """
        if self.gps_0_coord is None:
            self.gps_0_coord = (lat, lon)
            return 0., 0.   # No local frame to compare to

        geodesic_dist_info = self.g_to_d(*self.gps_0_coord, lat, lon)
        dist = geodesic_dist_info['s12']   # geodesic distance
        # azimuth (compass heading) of the geodesic as seen from source and destination
        az1 = geodesic_dist_info['azi1']
        az2 = geodesic_dist_info['azi2']
        # average heading between the two points in the nova frame (counter-clockwise positive)
        # this hopefully gives us a first order, rather than 0 order approximation of the coordinates
        avg_yaw_nova = - (az1 + az2) * 0.5

        x = dist * math.cos(avg_yaw_nova)
        y = dist * math.sin(avg_yaw_nova)

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

    def goal_callback(self, gps_goal_info):
        """
        Take a gps goal coordinate, convert it into the local frame, and publish as an x, y goal
        """
        local_goal_info = AutonomousInfo()

        local_goal_info.ids = [iD for iD in gps_goal_info.ids]  # same ar tag ids

        local_goal_info.position.x, local_goal_info.position.y = \
            self.get_local_coord(gps_goal_info.position.x, gps_goal_info.position.y)

        self.goal_publisher.publish(local_goal_info)

    def g_to_d_service(self, request, response):
        response.transform.x, response.transform.y = self.get_local_coord(request.point.x, request.point.y)

    def d_to_g_service(self, request, response):
        response.transform.x, response.transform.y = self.get_global_coord(request.point.x, request.point.y)


def main(args=None):
    rclpy.init(args=args)
    conversion_service = GpsConverter()
    rclpy.spin(conversion_service)
    rclpy.shutdown()


if __name__ == '__main__':
    main()
