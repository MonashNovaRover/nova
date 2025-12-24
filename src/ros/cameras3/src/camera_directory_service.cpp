#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <iostream>
#include <vector>
#include <systemd/sd-device.h>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
//#include <camera_msgs/msg/Camera.hpp>
//#include <camera_msgs/msg/Cameras.hpp>

using namespace std::chrono_literals;

struct V4lDevice {
  std::string model;
  std::string serial;
  std::string devname;
};

std::vector<V4lDevice> find_v4l_capture_devices(void);

class CameraDirectory : public rclcpp::Node
{
  public:
    CameraDirectory()
    : Node("camera_directory")
    {
      publisher_ = this->create_publisher<std_msgs::msg::String>("/camera_directory/cameras", 1);
      timer_ = this->create_wall_timer(
      500ms, std::bind(&CameraDirectory::timer_callback, this));
    }

  private:
    void timer_callback()
    {
      auto message = std_msgs::msg::String();
      std::vector<V4lDevice> devices = find_v4l_capture_devices();
      message.data = "Camera path: " + devices[0].devname;
      RCLCPP_INFO(this->get_logger(), "Publishing: '%s'", message.data.c_str());
      publisher_->publish(message);
    }
    rclcpp::TimerBase::SharedPtr timer_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
};

std::vector<V4lDevice> find_v4l_capture_devices() {
    sd_device_enumerator *enumerator = NULL;
    sd_device *device = NULL;
    std::vector<V4lDevice> matches;

    // Create new device enumerator object and add filters
    sd_device_enumerator_new(&enumerator);
    sd_device_enumerator_add_match_subsystem(enumerator, "video4linux", 1);

    // Iterate through the devices found
    for (device = sd_device_enumerator_get_device_first(enumerator); device != NULL; device = sd_device_enumerator_get_device_next(enumerator)) {
      const char *dev_node, *model_id = NULL, *serial = NULL, *capabilities = NULL;

      // Get the kernel name of the device (e.g., "video0")
      sd_device_get_devname(device, &dev_node);

      // Get device properties
      sd_device_get_property_value(device, "ID_MODEL", &model_id);
      sd_device_get_property_value(device, "ID_SERIAL", &serial);
      sd_device_get_property_value(device, "ID_V4L_CAPABILITIES", &capabilities);

      V4lDevice v4ldevice;
      v4ldevice.devname = dev_node;
      v4ldevice.model = model_id;
      v4ldevice.serial = serial;

      if (capabilities && strstr(capabilities, ":capture:")) {
        matches.push_back(v4ldevice);
      }
    }

    sd_device_enumerator_unref(enumerator);
    return matches;
}

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<CameraDirectory>());
  rclcpp::shutdown();
  return 0;
}