/**
 * Script to convert Realsense2 Pointclouds to Ros2 PointCloud2 messages
 * as rapidly as possible. Uses pybind11 to expose a function to python, 
 * which is called by the depth_camera node in the cameras module of the
 * autonomous ros package
 * Author: Max Tory
 * Written: 07/03/2023
 * Edited: 07/03/2023
*/

#include <sensor_msgs/msg/point_cloud2.hpp>
#include <std_msgs/msg/header.hpp>
#include <librealsense2/rs.hpp>
#include <string.h>
#include <pybind11/pybind11.h>


namespace py = pybind11;

sensor_msgs::msg::PointCloud2 rs2PointCloudToRos2PointCloud2(const rs2::points& points, const std_msgs::msg::Header& header)
{
    sensor_msgs::msg::PointCloud2 ros2_pointcloud;
    ros2_pointcloud.header = header;
    ros2_pointcloud.height = 1;
    ros2_pointcloud.width = points.size();
    ros2_pointcloud.is_dense = true;
    ros2_pointcloud.is_bigendian = false;
    ros2_pointcloud.point_step = 12;
    ros2_pointcloud.row_step = ros2_pointcloud.point_step * ros2_pointcloud.width;

    ros2_pointcloud.fields.resize(4);
    ros2_pointcloud.fields[0].name = "x";
    ros2_pointcloud.fields[0].offset = 0;
    ros2_pointcloud.fields[0].datatype = sensor_msgs::msg::PointField::FLOAT32;
    ros2_pointcloud.fields[0].count = 1;
    ros2_pointcloud.fields[1].name = "y";
    ros2_pointcloud.fields[1].offset = sizeof(float);
    ros2_pointcloud.fields[1].datatype = sensor_msgs::msg::PointField::FLOAT32;
    ros2_pointcloud.fields[1].count = 1;
    ros2_pointcloud.fields[2].name = "z";
    ros2_pointcloud.fields[2].offset = sizeof(float) * 2;
    ros2_pointcloud.fields[2].datatype = sensor_msgs::msg::PointField::FLOAT32;
    ros2_pointcloud.fields[2].count = 1;

    ros2_pointcloud.data.resize(ros2_pointcloud.row_step * ros2_pointcloud.height);
    uint8_t* data_ptr = ros2_pointcloud.data.data();

    for (int i = 0; i < points.size(); ++i)
    {
        const rs2::vertex& point = points.get_vertices()[i];

        memcpy(data_ptr, &point.x, sizeof(float));
        data_ptr += sizeof(float);

        memcpy(data_ptr, &point.y, sizeof(float));
        data_ptr += sizeof(float);

        memcpy(data_ptr, &point.z, sizeof(float));
        data_ptr += sizeof(float);
    }

    return ros2_pointcloud;
}


PYBIND11_MODULE(rs2_ros2, m) {
    m.doc() = "Realsense2 to ROS2 PointCloud2 conversion";

    m.def("rs2_to_ros2_cloud", &rs2PointCloudToRos2PointCloud2, "Convert a RealSense2 PointCloud to a ROS2 PointCloud2 message"); 
}
