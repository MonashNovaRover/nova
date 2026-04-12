#include <string>
#include <stdlib.h>
#include <cstring>
#include <gst/gst.h>
#include "rclcpp/rclcpp.hpp"
#include <camera_msgs/msg/camera.hpp>
#include <systemd/sd-device.h>

#include "properties/common.hpp"
#include "pipelines/pipelines.hpp"

bool link_elements(rclcpp::Node* streamer_node, GstElement* first_element, GstElement* second_element, const std::string serial) {
   if (!gst_element_link(first_element, second_element)) {
      RCLCPP_ERROR(streamer_node->get_logger(), "Could not link %s to %s for %s", gst_object_get_name(GST_OBJECT(first_element)), gst_object_get_name(GST_OBJECT(second_element)), serial.c_str());
      return false;
   }
   return true;
}

std::string set_property(rclcpp::Node* streamer_node, const std::string serial, const std::string profile, const std::string original_serial, const std::string element, const std::string default_value) {
    // Get property
    std::string value;
    streamer_node->get_parameter_or<std::string>((std::string(PIPELINE_PREFIX) + "." + serial + "." + element).c_str(), value, default_value);
    if (value != default_value) return value;
    if (profile != "NULL") {
      streamer_node->get_parameter_or<std::string>((std::string(PROFILE_PREFIX) + "." + profile + "." + element).c_str(), value, default_value);
      if (value != default_value) return value;
    }
    return value;
    streamer_node->get_parameter_or<std::string>((std::string(DEFAULT_PREFIX) + "." + original_serial + "." + element).c_str(), value, default_value);
}

int set_property(rclcpp::Node* streamer_node, const std::string serial, const std::string profile, const std::string original_serial, const std::string element, const int default_value) {
    // Get property
    int value;
    streamer_node->get_parameter_or((std::string(PIPELINE_PREFIX) + "." + serial + "." + element).c_str(), value, default_value);
    if (value != default_value) return value;
    if (profile != "NULL") {
      streamer_node->get_parameter_or((std::string(PROFILE_PREFIX) + "." + profile + "." + element).c_str(), value, default_value);
      if (value != default_value) return value;
    }
    streamer_node->get_parameter_or((std::string(DEFAULT_PREFIX) + "." + original_serial + "." + element).c_str(), value, default_value);
    return value;
}

bool set_property(rclcpp::Node* streamer_node, const std::string serial, const std::string profile, const std::string original_serial, const std::string element, const bool default_value) {
    // Get property
    bool value;
    streamer_node->get_parameter_or((std::string(PIPELINE_PREFIX) + "." + serial + "." + element).c_str(), value, default_value);
    if (value != default_value) return value;
    if (profile != "NULL") {
      streamer_node->get_parameter_or((std::string(PROFILE_PREFIX) + "." + profile + "." + element).c_str(), value, default_value);
      if (value != default_value) return value;
    }
    streamer_node->get_parameter_or((std::string(DEFAULT_PREFIX) + "." + original_serial + "." + element).c_str(), value, default_value);
    return value;
}

bool verify_resolution(const std::string device_name, std::string* mime, int* width, int* height, int* framerate, int* framerate_denominator) {
  GstDeviceMonitor *monitor = gst_device_monitor_new();
  gst_device_monitor_add_filter(monitor, "Video/Source", NULL);

  GList *devices = gst_device_monitor_get_devices(monitor);
  for (GList *l = devices; l != NULL; l = l->next) {
      GstDevice *device = (GstDevice *)l->data;
      GstStructure *device_props = gst_device_get_properties(device);
      const gchar *path = gst_structure_get_string(device_props, "device.path");
      int valid_width = 1280, valid_height = 720, framerate_n = 30, framerate_d = 1;
      std::string valid_mime;

      if (std::string(path) == device_name) {
          GstCaps* caps = gst_device_get_caps(device);
          for (guint i = 0; i < gst_caps_get_size(caps); i++) {
              const GstStructure* str = gst_caps_get_structure(caps, i);
              valid_mime = std::string(gst_structure_get_name(str));

              // Width
              const GValue* width_val = gst_structure_get_value(str, "width");
              bool width_ok = false;

              if (G_VALUE_HOLDS_INT(width_val)) {
                  width_ok = (valid_width == *width);
                  valid_width = g_value_get_int(width_val);
              } else if (GST_VALUE_HOLDS_INT_RANGE(width_val)) {
                  width_ok = (*width >= gst_value_get_int_range_min(width_val) &&
                              *width <= gst_value_get_int_range_max(width_val));
                  valid_width = gst_value_get_int_range_min(width_val);
              } else if (GST_VALUE_HOLDS_LIST(width_val)) {
                  for (guint j = 0; j < gst_value_list_get_size(width_val); ++j) {
                      const GValue* v = gst_value_list_get_value(width_val, j);
                      if (g_value_get_int(v) == *width) {
                          width_ok = true;
                          valid_width = g_value_get_int(v);
                          break;
                      }
                  }
              }

              // Height
              const GValue* height_val = gst_structure_get_value(str, "height");
              bool height_ok = false;

              if (G_VALUE_HOLDS_INT(height_val)) {
                  height_ok = (valid_height == *height);
                  valid_height = g_value_get_int(height_val);
              } else if (GST_VALUE_HOLDS_INT_RANGE(height_val)) {
                  height_ok = (*height >= gst_value_get_int_range_min(height_val) &&
                               *height <= gst_value_get_int_range_max(height_val));
                  valid_height = gst_value_get_int_range_max(height_val);
              } else if (GST_VALUE_HOLDS_LIST(height_val)) {
                  for (guint j = 0; j < gst_value_list_get_size(height_val); ++j) {

                      const GValue* v = gst_value_list_get_value(height_val, j);
                      if (g_value_get_int(v) == *height) {
                          height_ok = true;
                          valid_height = g_value_get_int(v);
                          break;
                      }
                  }
              }

              // Framerate
              gst_structure_get_fraction(str, "framerate", &framerate_n, &framerate_d);

              if ((valid_mime == *mime) && width_ok && height_ok && (framerate_n == *framerate) && (framerate_d == *framerate_denominator)) {
                g_list_free_full(devices, gst_object_unref);
                gst_object_unref(monitor);
                return true;
              }
          }
          g_list_free_full(devices, gst_object_unref);
          gst_object_unref(monitor);

          *mime = valid_mime;
          *width = valid_width;
          *height = valid_height;
          *framerate = framerate_n;
          *framerate_denominator = framerate_d;

          return false;
      }
  }
  g_list_free_full(devices, gst_object_unref);
  gst_object_unref(monitor);
  return true;
}

void match_lost_devname(rclcpp::Node* streamer_node, std::unordered_map<std::string, Pipeline*>* pipelines) {
  sd_device_enumerator *enumerator = NULL;
  sd_device *device = NULL;

  // Create new device enumerator object and add filters
  sd_device_enumerator_new(&enumerator);
  sd_device_enumerator_add_match_subsystem(enumerator, "video4linux", 1);


  std::unordered_set<std::string> used_devnames;


  // Iterate through the devices found
  for (device = sd_device_enumerator_get_device_first(enumerator); device != NULL; device = sd_device_enumerator_get_device_next(enumerator)) {
    const char *dev_name, *serial = NULL, *capabilities = NULL;

    // Get the kernel name of the device (e.g., "video0")
    sd_device_get_devname(device, &dev_name);
    sd_device_get_property_value(device, "ID_SERIAL", &serial);
    sd_device_get_property_value(device, "ID_V4L_CAPABILITIES", &capabilities);

    std::string dev_string(dev_name), serial_string(serial);

    // Iterate through pipelines to check if devname is already in use
    for (const auto& [serial, pipeline] : *pipelines) {
        if (dev_string == pipeline->props->node) {
            used_devnames.emplace(dev_string);
        }
    }

    // Iterate through pipelines to check if devname is already in use
    for (const auto& [serial, pipeline] : *pipelines) {
        if ((capabilities && strstr(capabilities, ":capture:")) && (serial_string == pipeline->props->original_serial) && (used_devnames.find(dev_string) == used_devnames.end())) {
            GstElement *source = gst_bin_get_by_name(GST_BIN(pipeline->gst_pipeline), "video-source");
            g_object_set(source,
              "device", dev_string.c_str(),
              NULL);
            break;
        }
    }
  }

  sd_device_enumerator_unref(enumerator);
  sd_device_unref(device);
}

