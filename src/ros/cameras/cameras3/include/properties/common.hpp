#ifndef COMMON_PROPERTY_HEADER
#define COMMON_PROPERTY_HEADER

#include <string>
#include <stdlib.h>
#include <gst/gst.h>
#include "rclcpp/rclcpp.hpp"
#include <camera_msgs/msg/camera.hpp>
#include "pipelines/properties.hpp"
#include "cameras/colors.hpp"

bool link_elements(rclcpp::Node* streamer_node, GstElement* first_element, GstElement* second_element, const std::string serial);

std::string set_property(rclcpp::Node* streamer_node, const camera_msgs::msg::Camera* camera, const std::string element, std::string value);

int set_property(rclcpp::Node* streamer_node, const camera_msgs::msg::Camera* camera, const std::string element, int value);

bool set_property(rclcpp::Node* streamer_node, const camera_msgs::msg::Camera* camera, const std::string element, bool value);

template<typename properties> bool verify_v4lresolution(const properties props) {
  GstDeviceMonitor *monitor = gst_device_monitor_new();
  gst_device_monitor_add_filter(monitor, "Video/Source", NULL);

  GList *devices = gst_device_monitor_get_devices(monitor);
  for (GList *l = devices; l != NULL; l = l->next) {
      GstDevice *device = (GstDevice *)l->data;
      GstStructure *device_props = gst_device_get_properties(device);
      const gchar *path = gst_structure_get_string(device_props, "device.path");
      int valid_width = 1280, valid_height = 720, framerate_n = 30, framerate_d = 1;
      std::string valid_mime;

      if (std::string(path) == props->device) {
          GstCaps* caps = gst_device_get_caps(device);
          for (guint i = 0; i < gst_caps_get_size(caps); i++) {
              const GstStructure* str = gst_caps_get_structure(caps, i);
              valid_mime = std::string(gst_structure_get_name(str));

              // Width
              const GValue* width_val = gst_structure_get_value(str, "width");
              bool width_ok = false;

              if (G_VALUE_HOLDS_INT(width_val)) {
                  width_ok = (valid_width == props->width);
                  valid_width = g_value_get_int(width_val);
              } else if (GST_VALUE_HOLDS_INT_RANGE(width_val)) {
                  width_ok = (props->width >= gst_value_get_int_range_min(width_val) &&
                              props->width <= gst_value_get_int_range_max(width_val));
                  valid_width = gst_value_get_int_range_min(width_val);
              } else if (GST_VALUE_HOLDS_LIST(width_val)) {
                  for (guint j = 0; j < gst_value_list_get_size(width_val); ++j) {
                      const GValue* v = gst_value_list_get_value(width_val, j);
                      if (g_value_get_int(v) == props->width) {
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
                  height_ok = (valid_height == props->height);
                  valid_height = g_value_get_int(height_val);
              } else if (GST_VALUE_HOLDS_INT_RANGE(height_val)) {
                  height_ok = (props->height >= gst_value_get_int_range_min(height_val) &&
                               props->height <= gst_value_get_int_range_max(height_val));
                  valid_height = gst_value_get_int_range_max(height_val);
              } else if (GST_VALUE_HOLDS_LIST(height_val)) {
                  for (guint j = 0; j < gst_value_list_get_size(height_val); ++j) {

                      const GValue* v = gst_value_list_get_value(height_val, j);
                      if (g_value_get_int(v) == props->height) {
                          height_ok = true;
                          valid_height = g_value_get_int(v);
                          break;
                      }
                  }
              }

              // Framerate
              gst_structure_get_fraction(str, "framerate", &framerate_n, &framerate_d);

              if ((valid_mime == props->mime) && width_ok && height_ok && (framerate_n == props->framerate) && (framerate_d == props->framerate_denominator)) {
                g_list_free_full(devices, gst_object_unref);
                gst_object_unref(monitor);
                return true;
              }
          }
          g_list_free_full(devices, gst_object_unref);
          gst_object_unref(monitor);
          gst_caps_unref(caps);

          props->mime = valid_mime;
          props->width = valid_width;
          props->height = valid_height;
          props->framerate = framerate_n;
          props->framerate_denominator = framerate_d;

          return false;
      }
  }
  g_list_free_full(devices, gst_object_unref);
  gst_object_unref(monitor);
  return true;
}

template<typename properties> void display_resolution(rclcpp::Node* streamer_node, const properties props, camera_msgs::msg::Camera* camera, const int crop_width) {
  if (props->verify_resolution) {
    if (verify_v4lresolution(props)) {
        RCLCPP_INFO(streamer_node->get_logger(), "%sInitialized pipeline: %s%s%s for %s%s%s with profile: %s%s %dx%d@%.2gfps%s", C_QUIET, C_INPUT, camera->pipeline_type.c_str(), C_QUIET, C_TITLE, props->serial.c_str(), C_QUIET, C_MODE, camera->profile.c_str(), (int)(((float)props->width-(float)2*crop_width)/(float)props->downscale), (int)((float)props->height/(float)props->downscale), (double) props->framerate/props->framerate_denominator/props->downrate, C_RESET);
    } else {
        RCLCPP_ERROR(streamer_node->get_logger(), "%sWrong resolution!%s Fallback pipeline: %s%s%s for %s%s%s with profile: %s%s %dx%d@%.2gfps%s", C_FAIL, C_QUIET, C_INPUT, camera->pipeline_type.c_str(), C_QUIET, C_TITLE, props->serial.c_str(), C_QUIET, C_MODE, camera->profile.c_str(), (int)(((float)props->width-(float)2*crop_width)/(float)props->downscale), (int)((float)props->height/(float)props->downscale), (double) props->framerate/props->framerate_denominator/props->downrate, C_RESET);
    }
  } else {
      RCLCPP_INFO(streamer_node->get_logger(), "%sInitialized pipeline: %s%s%s for %s%s%s with profile: %s%s %dx%d@%.2gfps%s", C_QUIET, C_INPUT, camera->pipeline_type.c_str(), C_QUIET, C_TITLE, props->serial.c_str(), C_QUIET, C_MODE, camera->profile.c_str(), (int)(((float)props->width-(float)2*crop_width)/(float)props->downscale), (int)((float)props->height/(float)props->downscale), (double) props->framerate/props->framerate_denominator/props->downrate, C_RESET);

  }
}

void get_profile(rclcpp::Node* node, camera_msgs::msg::Camera* camera);

#endif
