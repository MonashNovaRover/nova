#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <iostream>
#include <vector>
#include <chrono>
#include <unordered_map>
#include <systemd/sd-device.h>

#include "rclcpp/rclcpp.hpp"
#include "std_srvs/srv/empty.hpp"
#include <camera_msgs/msg/camera.hpp>
#include <camera_msgs/msg/cameras.hpp>

#include "cameras/cameras.hpp"
#include "cameras/directory_parameters.hpp"

using namespace std::placeholders;


struct V4lDevice {
  std::string model;
  std::string serial;
  std::string devname;
  std::string path;
};

std::vector<V4lDevice> find_v4l_capture_devices(void);

class CameraDirectory : public rclcpp::Node
{
  public: CameraDirectory() 
    : Node("camera_directory", 
      rclcpp::NodeOptions()
        .allow_undeclared_parameters(true)
        .automatically_declare_parameters_from_overrides(true)
    )
  {
    // set up camera directory publisher
    rclcpp::QoS publisher_qos(1);
    publisher_qos.reliability(RMW_QOS_POLICY_RELIABILITY_RELIABLE);
    publisher_qos.durability(RMW_QOS_POLICY_DURABILITY_TRANSIENT_LOCAL);
    publisher_ = this->create_publisher<camera_msgs::msg::Cameras>(TOPIC_CAMERAS, publisher_qos);
    timer_ = this->create_wall_timer(std::chrono::milliseconds(POLLING_PERIOD), std::bind(&CameraDirectory::publish_cameras, this));

    // setup service
    service_ = this->create_service<std_srvs::srv::Empty>(SERVICE_DISCOVERY, std::bind(&CameraDirectory::service_callback, this, _1, _2));
    
    // setup parameters
    param_listener = std::make_shared<camera_directory_service::ParamListener>(get_node_parameters_interface());
    this->get_configuration();

    // publish once
    this->publish_cameras();
    RCLCPP_INFO(this->get_logger(), "Polling v4l capture devices every %dms", POLLING_PERIOD);
  }

  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::Publisher<camera_msgs::msg::Cameras>::SharedPtr publisher_;
  rclcpp::Service<std_srvs::srv::Empty>::SharedPtr service_;
  std::shared_ptr<camera_directory_service::ParamListener> param_listener;
  std::vector<std::string> blacklist;
  std::unordered_map<std::string, std::string> serial_remaps;
  std::unordered_map<std::string, std::string> serial_overrides;
  std::unordered_map<std::string, std::string> camera_map;
  size_t last_device_count;

  private: void get_configuration()
  {
    camera_directory_service::Params params = param_listener->get_params();

    std::map<std::string, rclcpp::Parameter> serial_remaps_parameters;
    this->get_parameters("serial_remaps", serial_remaps_parameters);
    for (const auto& kv: serial_remaps_parameters) {
      serial_remaps[kv.first] = kv.second.as_string();
    }

    std::map<std::string, rclcpp::Parameter> serial_overrides_roots_parameters;
    this->get_parameters("serial_overrides.roots", serial_overrides_roots_parameters);
    std::unordered_map<std::string, std::string> serial_override_roots;
    for (const auto& kv : serial_overrides_roots_parameters) {
        const std::string& root = kv.first;
        const std::string& name = kv.second.as_string();

        std::string param_prefix = "serial_overrides.paths." + name;
        std::map<std::string, rclcpp::Parameter> path_params;
        this->get_parameters(param_prefix, path_params);

        for (const auto& path_pair : path_params) {
            const std::string& path = path_pair.first;
            const std::string& serial = path_pair.second.as_string();
            std::string key = root + "." + path;
            serial_overrides[key] = serial;
        }
    }
  }

  private: void publish_cameras()
  {
    auto message = camera_msgs::msg::Cameras();
    std::vector<V4lDevice> devices = find_v4l_capture_devices();
    std::unordered_map<std::string, std::string> new_camera_map;
    for (V4lDevice device : devices) {
      std::string serial = device.serial;
      if (serial_overrides.find(device.path) != serial_overrides.end()){
        serial = serial_overrides[device.path];
      }
      if (serial_remaps.find(serial) != serial_remaps.end()){
        serial = serial_remaps[serial];
      }
      auto camera = camera_msgs::msg::Camera();
      camera.serial = serial;
      camera.node = device.devname;
      message.cameras.push_back(camera);
      if (camera_map.find(serial) == camera_map.end())
      {
        new_camera_map[serial] = device.devname;
        if (serial != device.serial) RCLCPP_INFO(this->get_logger(), "New device found: %s serial remapped to: %s", device.serial.c_str(), serial.c_str());
        else RCLCPP_INFO(this->get_logger(), "New device found: %s", serial.c_str());
        camera_map = new_camera_map;
      }
    }
    if (devices.size() != last_device_count) {
      RCLCPP_INFO(this->get_logger(), "Publishing %ld Cameras...", devices.size());
      last_device_count = devices.size();
      camera_map = new_camera_map;
    }
    publisher_->publish(message);
  }

  private: void service_callback(
    const std::shared_ptr<std_srvs::srv::Empty::Request>,
    std::shared_ptr<std_srvs::srv::Empty::Response>)
  {
    this->get_configuration();
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
    const char *dev_node, *model_id = NULL, *serial = NULL, *path_id = NULL, *capabilities = NULL;

    // Get the kernel name of the device (e.g., "video0")
    sd_device_get_devname(device, &dev_node);

    // Get device properties
    sd_device_get_property_value(device, "ID_MODEL", &model_id);
    sd_device_get_property_value(device, "ID_SERIAL", &serial);
    sd_device_get_property_value(device, "ID_PATH", &path_id);
    sd_device_get_property_value(device, "ID_V4L_CAPABILITIES", &capabilities);

    V4lDevice v4ldevice;
    v4ldevice.devname = dev_node;
    v4ldevice.model = model_id;
    v4ldevice.serial = serial;
    v4ldevice.path = path_id;

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