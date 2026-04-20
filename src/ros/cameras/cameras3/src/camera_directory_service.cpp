#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <iostream>
#include <vector>
#include <chrono>
#include <unordered_map>
#include <utility>
#include <systemd/sd-device.h>

#include "rclcpp/rclcpp.hpp"
#include "std_srvs/srv/empty.hpp"
#include <camera_msgs/msg/camera.hpp>
#include <camera_msgs/msg/cameras.hpp>

#include "cameras/globals.hpp"
#include "cameras/colors.hpp"

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
    this->get_configuration();

    // publish once
    this->publish_cameras();
    RCLCPP_INFO(this->get_logger(), "%sPolling v4l capture devices every %s%dms%s", C_QUIET, C_MODE, POLLING_PERIOD, C_RESET);
  }

  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::Publisher<camera_msgs::msg::Cameras>::SharedPtr publisher_;
  rclcpp::Service<std_srvs::srv::Empty>::SharedPtr service_;
  std::vector<std::string> blacklist;
  std::unordered_map<std::string, std::string> serial_remaps;
  std::string platform;
  std::string task;
  std::unordered_map<std::string, std::string> serial_overrides;
  std::unordered_map<std::string, std::string> camera_map;
  size_t last_device_count = -1;

  private: void get_configuration()
  {
    blacklist = this->get_parameter_or<std::vector<std::string>>("blacklist", std::vector<std::string>());
    std::map<std::string, rclcpp::Parameter> serial_remaps_parameters;
    this->get_parameters("serial_remaps", serial_remaps_parameters);
    for (const auto& kv: serial_remaps_parameters) {
      serial_remaps[kv.first] = kv.second.as_string();
    }

    std::stringstream log;

    if (this->get_parameter<std::string>("platform", platform)) {
      log << C_QUIET << "Using platform root from " << C_SUBTITLE << platform << C_RESET << "\n";
    } else {
      log << C_QUIET << "Node argument \"" << C_INPUT << "platform" << C_QUIET << "\" is empty" << C_RESET << "\n";
    }

    if (this->get_parameter<std::string>("task", task)) {
      log << C_QUIET << "Using task serials from " << C_SUBTITLE << task << C_RESET;
    } else {
      log << C_QUIET << "Node argument \"" << C_INPUT << "task" << C_QUIET << "\" is empty" << C_RESET;
    }

    // Pretty print cameras
    RCLCPP_INFO(this->get_logger(), "%s", log.str().c_str());

    if (platform.empty()) {
      RCLCPP_WARN(this->get_logger(), "%sSkipping serial_overrides...%s", C_FAIL, C_RESET);
    } else {
      std::map<std::string, std::pair<std::string, std::string>> path_map;
      std::map<std::string, std::string> root_map;
      
      // load platform specific root remap
      std::map<std::string, rclcpp::Parameter> platform_roots;
      this->get_parameters("serial_overrides.platform_roots", platform_roots);
      for (const auto& platform_kv: platform_roots) {
        std::string platform_name = platform_kv.first.substr(0, platform_kv.first.find('.'));
        if (platform_name == platform) {
          std::map<std::string, rclcpp::Parameter> platform_root_map;
          this->get_parameters("serial_overrides.platform_roots."+ platform_name, platform_root_map);
          for (const auto& root_kv: platform_root_map) {
            root_map[root_kv.second.as_string()] = root_kv.first;
            // e.g mast: "platform-3530000.xhci-0:1"
          }
          break;
        }
      }

      // load default payload-bus path remaps 
      std::map<std::string, rclcpp::Parameter> default_payloads;
      this->get_parameters("serial_overrides.default_paths", default_payloads);
      for (const auto& default_kv: default_payloads) {
        std::string payload_name = default_kv.first.substr(0, default_kv.first.find('.'));
        std::map<std::string, rclcpp::Parameter> default_payload_paths;
        this->get_parameters("serial_overrides.default_paths." + payload_name, default_payload_paths);
        for (const auto& path_kv: default_payload_paths) {
          path_map[path_kv.second.as_string()] = {payload_name, path_kv.first};
          // e.g payload_1: {mast, 1:1.0}

          // Add defaults to serial overrides first
          std::string key = root_map[payload_name] + "." + path_kv.first;
          serial_overrides[key] = path_kv.second.as_string();
        }
      }

      // load task specific serial overrides
      std::map<std::string, rclcpp::Parameter> path_params;
      this->get_parameters("serial_overrides.task_paths." + task, path_params);
      for (const auto& override : path_params) {
        std::string root;
        std::string path;
        if (path_map.find(override.first) != path_map.end()){
          // existing default override found
          path = path_map[override.first].second;
          root = root_map[path_map[override.first].first];
        } else {
          // no default override found, assume yaml in the following form:
          // task_name:
          //   path: remap
          // this will become "task_name.path: remap" after get_parameters()
          int pos = override.first.find(".");
          path = override.first.substr(pos+1);
          root = root_map[override.first.substr(0, pos)];
        }
        std::string key = root + "." + path;
        serial_overrides[key] = override.second.as_string();
      }
    }
  }

  private: void publish_cameras()
  {
    /*
      Add each camera to a cameras message and publish their final serial and dev node.    
    */ 

    auto message = camera_msgs::msg::Cameras();
    std::vector<V4lDevice> devices = find_v4l_capture_devices();
    std::unordered_map<std::string, std::string> new_camera_map;

    std::stringstream log;
    if (devices.size() != last_device_count) {
      log << C_MODE << "Detected Cameras:" << C_RESET;
    }

    for (V4lDevice device : devices) {
      // if device is in blacklist, skip
      if (std::find(blacklist.begin(), blacklist.end(), device.serial) != blacklist.end()) continue;

      // add to message
      camera_msgs::msg::Camera camera = camera_msgs::msg::Camera();

      // get final serial with remaps and overrides
      std::string serial = device.serial;
      if (serial_overrides.find(device.path) != serial_overrides.end()){
        serial = serial_overrides[device.path];
      }
      if (serial_remaps.find(serial) != serial_remaps.end()){
        serial = serial_remaps[serial];
      }

      // Find how many of the current serial exist
      //const int serial_count = new_camera_map.count(device.serial);

      // support multiple cameras of the same type
      //camera.serial = serial + std::to_string(serial_count); 

      camera.serial = serial;
      camera.node = device.devname;
      camera.original_serial = device.serial;
      message.cameras.emplace_back(camera);

      if (devices.size() != last_device_count) {
        log << "\n - " << C_TITLE << serial << C_RESET;
        if (serial != device.serial)
        {
          log << C_QUIET " remapped from " << C_INPUT << device.serial << C_RESET;
        }
        log << C_QUIET " located at " << C_MODE << device.path << C_RESET;

        // check if new camera or serial changed
        if (camera_map.find(serial) == camera_map.end()) {
          new_camera_map[serial] = device.devname;
          camera_map = new_camera_map;
        }
      }
    }
    if (devices.size() != last_device_count) {
      log << "\n" << C_QUIET "Publishing " << C_SUBTITLE << devices.size() << C_QUIET " Cameras..." << C_RESET;
      last_device_count = devices.size();
      camera_map = new_camera_map;

      // Pretty print cameras
      RCLCPP_INFO(this->get_logger(), "%s", log.str().c_str());
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
  /*
    Finds all V4L2 capture devices on the system and returns a vector of V4lDevice structs.
    @return std::vector<V4lDevice> vector of V4lDevice structs representing the found devices
  */
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

    // Ignore if the serial already exists in cameras. We should fix this eventually
    // Check if address is a valid camera stream
    if (capabilities && strstr(capabilities, ":capture:")) {
      matches.emplace_back(v4ldevice);
      //seen_serials.emplace(serial);
    }
  }

  sd_device_enumerator_unref(enumerator);
  sd_device_unref(device);
  return matches;
}

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<CameraDirectory>());
  rclcpp::shutdown();
  return 0;
}

