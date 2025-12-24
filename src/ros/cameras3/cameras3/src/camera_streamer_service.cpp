#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <iostream>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include <cameras3_msgs/msg/camera.hpp>
#include <cameras3_msgs/msg/cameras.hpp>

using namespace std::chrono_literals;
using namespace std::placeholders;

/*
CLONE CAMERAS2 Streamer Service TODO:
- Start streaming service
- Stop streaming service
- Pause streaming service
- Create gstreamer pipeline per camera
- Configurable parameters using "generate_parameter_library"
- ROS2 Topic pipeline from parameters
- gst-launch-1.0 string to pipeline
*/


class CameraStreamer : public rclcpp::Node
{
  public: CameraStreamer()
    : Node("camera_streamer")
    {
    }
};


int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<CameraStreamer>());
  rclcpp::shutdown();
  return 0;
}