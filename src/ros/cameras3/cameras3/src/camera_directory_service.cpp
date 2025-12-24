#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <iostream>
#include <vector>
#include <systemd/sd-device.h>

#include "rclcpp/rclcpp.hpp"
//#include "std_msgs/msg/string.hpp"
#include "std_srvs/srv/empty.hpp"
#include <cameras3_msgs/msg/camera.hpp>
#include <cameras3_msgs/msg/cameras.hpp>

#include "cameras3/cameras3.hpp"

//using namespace std::chrono_literals;
using namespace std::placeholders;

/*
CLONE CAMERAS2 Directory Service TODO:
- Serial remaps, overrides
- Watch for changes to camera list

New Features Directory Service TODO:
- Configurable parameters using "generate_parameter_library"
*/

struct V4lDevice {
  std::string model;
  std::string serial;
  std::string devname;
};

std::vector<V4lDevice> find_v4l_capture_devices(void);



class CameraDirectory : public rclcpp::Node
{
  public: CameraDirectory() : Node("camera_directory")
  {
    // set up camera directory publisher
    rclcpp::QoS publisher_qos(1);
    publisher_qos.reliability(RMW_QOS_POLICY_RELIABILITY_RELIABLE);
    publisher_qos.durability(RMW_QOS_POLICY_DURABILITY_TRANSIENT_LOCAL);
    publisher_ = this->create_publisher<cameras3_msgs::msg::Cameras>(TOPIC_CAMERAS, publisher_qos);
    // timer_ = this->create_wall_timer(10000ms, std::bind(&CameraDirectory::publish_cameras, this));

    // setup service
    service_ = this->create_service<std_srvs::srv::Empty>(SERVICE_DISCOVERY, std::bind(&CameraDirectory::service_callback, this, _1, _2));
    
    // publish once
    this->publish_cameras();
  }

  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::Publisher<cameras3_msgs::msg::Cameras>::SharedPtr publisher_;
  rclcpp::Service<std_srvs::srv::Empty>::SharedPtr service_;

  private: void publish_cameras()
  {
    auto message = cameras3_msgs::msg::Cameras();
    std::vector<V4lDevice> devices = find_v4l_capture_devices();
    for (V4lDevice device : devices) {
      auto camera = cameras3_msgs::msg::Camera();
      camera.serial = device.serial;
      camera.node = device.devname;
      message.cameras.push_back(camera);
    }
    RCLCPP_INFO(this->get_logger(), "Publishing %ld Cameras...", devices.size());
    publisher_->publish(message);
  }

  private: void service_callback(
    const std::shared_ptr<std_srvs::srv::Empty::Request>,
    std::shared_ptr<std_srvs::srv::Empty::Response>)
  {
    this->publish_cameras();
  }
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