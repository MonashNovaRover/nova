/**
 * Script to convert Realsense2 Pointclouds to Ros2 PointCloud2 messages
 * as rapidly as possible. Uses pybind11 to expose a function to python, 
 * which is called by the depth_camera node in the cameras module of the
 * autonomous ros package
 * Author: Max Tory
 * Written: 07/03/2023
 * Edited: 07/03/2023
*/

#include "rclcpp/rclcpp.hpp"
#include "builtin_interfaces/msg/time.hpp"
#include "sensor_msgs/msg/point_cloud2.hpp"
#include "std_msgs/msg/header.hpp"
#include <librealsense2/rs.hpp>
#include <string.h>
#include <vector>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>


namespace py = pybind11;

std::vector<uint8_t> rs2VertsToBufferArray(const std::vector<std::vector<float>>& verts)
{
    // There are 4 bytes in a float32 and 1 byte in a uint8
    const size_t UINT8_PER_FLOAT32 = 4;
    std::vector<uint8_t> data(UINT8_PER_FLOAT32 * verts.size() * 3, 0);
    uint8_t* data_ptr = data.data();

    for (size_t i = 0; i < verts.size(); ++i)
    {
        const std::vector<float> point = verts[i];

        memcpy(data_ptr, &point[0], sizeof(float));
        data_ptr += sizeof(float);

        memcpy(data_ptr, &point[1], sizeof(float));
        data_ptr += sizeof(float);

        memcpy(data_ptr, &point[2], sizeof(float));
        data_ptr += sizeof(float);
    }

    return data;
}

PYBIND11_MODULE(rs2_ros2, m) {
    m.doc() = "Realsense2 to ROS2 PointCloud2 conversion";

    m.def("rs2_verts_to_buffer", &rs2VertsToBufferArray, "Convert RealSense2 PointCloud vertices to a data buffer of uint8s"); 
}
