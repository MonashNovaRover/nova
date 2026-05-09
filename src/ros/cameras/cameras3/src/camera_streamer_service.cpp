#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <any>
#include <thread>

#include "rclcpp/rclcpp.hpp"
#include "std_srvs/srv/empty.hpp"
#include <gst/gst.h>
#include <gst/gl/gl.h>
#include <gst/gl/gstglcontext.h>
#include <stdlib.h>

#include <camera_msgs/srv/camera_operation.hpp>
#include <camera_msgs/srv/camera_profile_selection.hpp>
#include <camera_msgs/srv/get_camera_stream_stats.hpp>
#include <camera_msgs/srv/get_ip_list.hpp>
#include <camera_msgs/msg/camera.hpp>
#include <camera_msgs/msg/cameras.hpp>

#include "cameras/globals.hpp"
#include "pipelines/properties.hpp"
#include "pipelines/pipelines.hpp"
#include "properties/common.hpp"

#include "properties/sources.hpp"
#include "properties/sinks.hpp"

#include "properties/capsfilters.hpp"
#include "properties/cpufilters.hpp"
#include "properties/glfilters.hpp"
#include "properties/decoders.hpp"
#include "properties/encoders.hpp"

#include "cameras/colors.hpp"

using namespace std::placeholders;

enum CameraState {STOP = 0, START = 1, PAUSE = 2};

class CameraStreamer : public rclcpp::Node
{
  public: CameraStreamer()
    : Node("camera_streamer", 
      rclcpp::NodeOptions()
        .allow_undeclared_parameters(true)
        .automatically_declare_parameters_from_overrides(true)
    )
  {
    start_service_ = this->create_service<camera_msgs::srv::CameraOperation>(
      SERVICE_START, 
      std::bind(&CameraStreamer::operation_callback, 
        this, _1, _2, CameraState::START)
    );
    stop_service_ = this->create_service<camera_msgs::srv::CameraOperation>(
      SERVICE_STOP, 
      std::bind(&CameraStreamer::operation_callback, 
        this, _1, _2, CameraState::STOP)
    );
    pause_service_ = this->create_service<camera_msgs::srv::CameraOperation>(
      SERVICE_PAUSE, 
      std::bind(&CameraStreamer::operation_callback, 
        this, _1, _2, CameraState::PAUSE)
    );
    profile_service_ = this->create_service<camera_msgs::srv::CameraProfileSelection>(
      SERVICE_PROFILE, 
      std::bind(&CameraStreamer::profile_callback,
        this, _1, _2)
    );
    stats_service_ = this->create_service<camera_msgs::srv::GetCameraStreamStats>(
      SERVICE_STATS, 
      std::bind(&CameraStreamer::stats_callback, 
        this, _1, _2)
    );
    ips_service_ = this->create_service<camera_msgs::srv::GetIPList>(
      SERVICE_IPS, 
      std::bind(&CameraStreamer::ips_callback, 
        this, _1, _2)
    );

    subscription_ = this->create_subscription<camera_msgs::msg::Cameras>(
      TOPIC_CAMERAS, discover_qos, std::bind(&CameraStreamer::topic_callback, this, _1));

    RCLCPP_INFO(this->get_logger(), "%sCameras3 Streamer Running...%s", C_QUIET, C_RESET);
  }

  rclcpp::Service<camera_msgs::srv::CameraOperation>::SharedPtr start_service_;
  rclcpp::Service<camera_msgs::srv::CameraOperation>::SharedPtr stop_service_;
  rclcpp::Service<camera_msgs::srv::CameraOperation>::SharedPtr pause_service_;
  rclcpp::Service<camera_msgs::srv::CameraProfileSelection>::SharedPtr profile_service_;
  rclcpp::Service<camera_msgs::srv::GetCameraStreamStats>::SharedPtr stats_service_;
  rclcpp::Service<camera_msgs::srv::GetIPList>::SharedPtr ips_service_;
  rclcpp::Subscription<camera_msgs::msg::Cameras>::SharedPtr subscription_;
  std::unordered_map<std::string, std::unique_ptr<Pipeline>> pipelines;
  const std::unordered_set<std::string> profiles = {"default", "super", "still", "snail", "emergency"};

  // Initialize gstreamer opengl
  GstGLDisplay *gl_display = gst_gl_display_new();
  
  private: void start_pipeline(const std::unique_ptr<Pipeline>& pipeline)
  {
    // get pipeline properties and use them to create the pipeline
    if (pipeline->camera->pipeline_type == "v4lfallback")
    {
      std::unique_ptr<v4lfallbackPipelineProperties> props = get_v4lfallback_pipeline_properties(this, pipeline->camera);
      pipeline->gst_pipeline = v4lfallback_pipeline(this, props);
    } else if (pipeline->camera->pipeline_type == "h264passthrough") {
      std::unique_ptr<h264passthroughPipelineProperties> props = get_h264passthrough_pipeline_properties(this, pipeline->camera);
      pipeline->gst_pipeline = h264passthrough_pipeline(this, props);
    } else if (pipeline->camera->pipeline_type == "vp8software") {
      std::unique_ptr<vpXsoftwarePipelineProperties> props = get_vpXsoftware_pipeline_properties(this, pipeline->camera, 8);
      pipeline->gst_pipeline = vpXsoftware_pipeline(this, props, 8);
    } else if (pipeline->camera->pipeline_type == "vp8softwareGL") {
      std::unique_ptr<vpXsoftwareGLPipelineProperties> props = get_vpXsoftwareGL_pipeline_properties(this, pipeline->camera, 8);
      pipeline->gst_pipeline = vpXsoftwareGL_pipeline(this, props, 8);
      GstContext *gl_context = gst_context_new("gst.gl.GLDisplay", TRUE);
      gst_context_set_gl_display(gl_context, gl_display);
      gst_element_set_context(pipeline->gst_pipeline, gl_context);
      gst_context_unref(gl_context);
    } else if (pipeline->camera->pipeline_type == "vp9software") {
      std::unique_ptr<vpXsoftwarePipelineProperties> props = get_vpXsoftware_pipeline_properties(this, pipeline->camera, 9);
      pipeline->gst_pipeline = vpXsoftware_pipeline(this, props, 9);
    } else if (pipeline->camera->pipeline_type == "vp9softwareGL") {
      std::unique_ptr<vpXsoftwareGLPipelineProperties> props = get_vpXsoftwareGL_pipeline_properties(this, pipeline->camera, 9);
      pipeline->gst_pipeline = vpXsoftwareGL_pipeline(this, props, 9);
      GstContext *gl_context = gst_context_new("gst.gl.GLDisplay", TRUE);
      gst_context_set_gl_display(gl_context, gl_display);
      gst_element_set_context(pipeline->gst_pipeline, gl_context);
      gst_context_unref(gl_context);
    }
  }

  private: void change_profile_properties(const std::unique_ptr<Pipeline>& pipeline)
  {
    // get pipeline properties and use them to create the pipeline
    if (pipeline->camera->pipeline_type == "v4lfallback")
    {
      std::unique_ptr<v4lfallbackPipelineProperties> props = get_v4lfallback_pipeline_properties(this, pipeline->camera);

    } else if (pipeline->camera->pipeline_type == "h264passthrough") {
      std::unique_ptr<h264passthroughPipelineProperties> props = get_h264passthrough_pipeline_properties(this, pipeline->camera);
      
    } else if (pipeline->camera->pipeline_type == "vp8software") {
      std::unique_ptr<vpXsoftwarePipelineProperties> props = get_vpXsoftware_pipeline_properties(this, pipeline->camera, 8);
      set_vpXsoftware_pipeline_properties(pipeline->gst_pipeline, props, 8);
    } else if (pipeline->camera->pipeline_type == "vp8softwareGL") {
      std::unique_ptr<vpXsoftwareGLPipelineProperties> props = get_vpXsoftwareGL_pipeline_properties(this, pipeline->camera, 8);
      set_vpXsoftwareGL_pipeline_properties(pipeline->gst_pipeline, props, 8);
    } else if (pipeline->camera->pipeline_type == "vp9software") {
      std::unique_ptr<vpXsoftwarePipelineProperties> props = get_vpXsoftware_pipeline_properties(this, pipeline->camera, 9);
      set_vpXsoftware_pipeline_properties(pipeline->gst_pipeline, props, 9);
    } else if (pipeline->camera->pipeline_type == "vp9softwareGL") {
      std::unique_ptr<vpXsoftwareGLPipelineProperties> props = get_vpXsoftwareGL_pipeline_properties(this, pipeline->camera, 9); 
      set_vpXsoftwareGL_pipeline_properties(pipeline->gst_pipeline, props, 9);
    }
  }

  private: void get_pipeline_type(const std::unique_ptr<Pipeline>& pipeline)
  {

    // Get pipeline type
    const std::string default_string = "v4lfallback";

    // Get serial first
    if (this->get_parameter<std::string>((std::string(PIPELINE_PREFIX) + "." + pipeline->camera->serial + ".pipeline_type").c_str(), pipeline->camera->pipeline_type)) return;

    if (!pipeline->camera->profile.empty()) {

      // Get type from serial
      if (this->get_parameter<std::string>((std::string(PROFILE_PREFIX) + "." + pipeline->camera->original_serial + "." + pipeline->camera->profile + ".pipeline_type").c_str(), pipeline->camera->pipeline_type)) return;

      // Get type from unknown
      if (this->get_parameter<std::string>((std::string(PROFILE_PREFIX) + "." + std::string(UNKNOWN_PROFILE_PREFIX) + "." + pipeline->camera->profile + ".pipeline_type").c_str(), pipeline->camera->pipeline_type)) return;
    }

    // Get default last
    if (this->get_parameter<std::string>((std::string(DEFAULT_PREFIX) + "." + pipeline->camera->original_serial + ".pipeline_type").c_str(), pipeline->camera->pipeline_type)) return;
    pipeline->camera->pipeline_type = default_string;
  }

  private: void topic_callback(const camera_msgs::msg::Cameras msg)
  {
    for (camera_msgs::msg::Camera camera : msg.cameras) {
      // Make the pipeline if it doesn't exist
      if (this->pipelines.find(camera.serial) == pipelines.end()) {
        std::unique_ptr<Pipeline> pipeline = std::make_unique<Pipeline>();
        pipeline->camera = std::make_unique<camera_msgs::msg::Camera>();
        pipeline->camera->serial = camera.serial;
        pipeline->camera->node = camera.node;
        pipeline->camera->original_serial = camera.original_serial;

        // Get pipeline
        get_profile(this, pipeline->camera);

        // Get pipeline_type
        this->get_pipeline_type(pipeline);
        this->start_pipeline(pipeline);

        bool autostart = false;
        this->get_parameter("autostart", autostart);

        // auto start if true
        if (autostart) {
          gst_element_set_state(pipeline->gst_pipeline, GST_STATE_PLAYING);
        }
        this->pipelines[camera.serial] = std::move(pipeline);
      }
    }
  }


  private: void operation_callback(
    const std::shared_ptr<camera_msgs::srv::CameraOperation::Request> request,
    std::shared_ptr<camera_msgs::srv::CameraOperation::Response> response,
    CameraState state)
  {
    response->success = true;
    switch (state) {
      case CameraState::START:
        for (std::string serial : request->serials) {
          if (this->pipelines.find(serial) != pipelines.end()) {
            std::unique_ptr<Pipeline>& pipeline = pipelines[serial];
            if (this->pipelines[serial]->gst_pipeline != nullptr) {
            // gstreamer play pipeline if paused
              RCLCPP_INFO(this->get_logger(), "%sResuming %s%s%s", C_QUIET, C_TITLE, serial.c_str(), C_RESET); 
              GstElement *source_valve = gst_bin_get_by_name(GST_BIN(pipeline->gst_pipeline), "source_valve");
              g_object_set(source_valve, "drop", false, NULL);
              gst_object_unref(source_valve);
              gst_element_set_state(pipeline->gst_pipeline, GST_STATE_PLAYING);
            } else {
              // start pipeline if the gst bin doesn't exist yet
              this->start_pipeline(pipeline);
              gst_element_set_state(pipeline->gst_pipeline, GST_STATE_PLAYING);
            }
          } else {
          // otherwise report error
            RCLCPP_ERROR(this->get_logger(), "%sIssue with pipeline of: %s%s%s", C_QUIET, C_FAIL, serial.c_str(), C_RESET);
            response->success = false;
          }
        }
        break;
      case CameraState::STOP:
        for (std::string serial : request->serials) {
          if (this->pipelines.find(serial) != pipelines.end() && this->pipelines[serial]->gst_pipeline != nullptr) {
            std::unique_ptr<Pipeline>& pipeline = pipelines[serial];

            GstElement *source_valve = gst_bin_get_by_name(GST_BIN(pipeline->gst_pipeline), "source_valve");
            g_object_set(source_valve, "drop", true, NULL);
            gst_object_unref(source_valve);
            gst_element_set_state(pipeline->gst_pipeline, GST_STATE_PAUSED);


            RCLCPP_INFO(this->get_logger(), "%sStopping %s%s%s", C_QUIET, C_TITLE, serial.c_str(), C_RESET);
          } else {
            RCLCPP_INFO(this->get_logger(), "%sIssue with pipeline of: %s%s%s", C_QUIET, C_FAIL, serial.c_str(), C_RESET);
            response->success = false;
          }
        }
        break;
      case CameraState::PAUSE:
        for (std::string serial : request->serials) {
          if (this->pipelines.find(serial) != pipelines.end() && this->pipelines[serial]->gst_pipeline != nullptr) {
            std::unique_ptr<Pipeline>& pipeline = pipelines[serial];
            gst_element_set_state(pipeline->gst_pipeline, GST_STATE_PAUSED);
            RCLCPP_INFO(this->get_logger(), "%sPausing %s%s%s", C_QUIET, C_TITLE, serial.c_str(), C_RESET);
          } else {
            RCLCPP_INFO(this->get_logger(), "%sIssue with pipeline of: %s%s%s", C_QUIET, C_FAIL, serial.c_str(), C_RESET);
            response->success = false;
          }
        }
        break;
    }
  }

  private: void profile_callback(
    const std::shared_ptr<camera_msgs::srv::CameraProfileSelection::Request> request,
    std::shared_ptr<camera_msgs::srv::CameraProfileSelection::Response> response)  
  {
    response->success = true;
    for (std::string serial : request->serials) { 
      if (this->pipelines.find(serial) != pipelines.end() && this->pipelines[serial]->gst_pipeline != nullptr) {
        std::unique_ptr<Pipeline>& pipeline = pipelines[serial];

        bool correct_camera = false;

        // From presets
        std::string task;
        if (this->get_parameter("task", task)) {
          if (this->get_parameter<std::string>((std::string(PRESET_PREFIX) + "." + task + "." + request->profile + "." + pipeline->camera->serial).c_str(), pipeline->camera->profile)){
            correct_camera = true;
          }
        }

        // Set directly
        if (!correct_camera) {
          std::string validate_profile;
          if (profiles.find(request->profile) != profiles.end()) {
            pipeline->camera->profile = request->profile;
          } else {
            RCLCPP_ERROR(this->get_logger(), "%sWrong profile %s%s%s given to pipeline of: %s%s%s", C_QUIET, C_FAIL, request->profile.c_str(), C_QUIET, C_FAIL, serial.c_str(), C_RESET);
            response->success = false;
            return;
          }
        }

        // Change a subset of properties that can be changed in runtime
        change_profile_properties(pipeline);

        response->success = true;

        bool autostart = false;
        this->get_parameter("autostart", autostart);

        // auto start if true
        RCLCPP_INFO(this->get_logger(), "%sApplied %s%s%s to profile: %s%s%s", C_QUIET, C_TITLE, pipeline->camera->serial.c_str(), C_QUIET, C_MODE, pipeline->camera->profile.c_str(), C_RESET);
      }
    }
  }

  private: void stats_callback(
    const std::shared_ptr<camera_msgs::srv::GetCameraStreamStats::Request>,
    std::shared_ptr<camera_msgs::srv::GetCameraStreamStats::Response> response)  
  {
    /* TODO: convert this python into c++ (its not used for now anyway)
      result = {
        serial: self._camera_bins[serial].webrtc_stats
        for serial in (request.serials if request.serials else self._camera_bins.keys())
        if serial in self._camera_bins
      }
      response.result_json = json.dumps(result, indent=None if request.indent == 0 else request.indent)
    */
    response->result_json = "NOT IMPLEMENTED";
  }

  private: void ips_callback(
    const std::shared_ptr<camera_msgs::srv::GetIPList::Request>,
    std::shared_ptr<camera_msgs::srv::GetIPList::Response> response)  
  {
    /* TODO: Convert this python to c++ (its not used for now anyway)
      if_addrs = psutil.net_if_addrs()
      if_stats = psutil.net_if_stats()
      addresses = {
        interface: address
        for interface, addresses in if_addrs.items()
        if (
            address := next(
                (address.address for address in addresses if address.family == AddressFamily.AF_INET),
                None,
            )
        )
        is not None
      }

      def key(entry: tuple[str, str]) -> int:
        interface = entry[0]
        address = entry[1]
        flags = set(if_stats[interface].flags.split(","))
        if "loopback" in flags:  # Local
            return 0
        elif address.startswith("192.168.1"):  # Nova radios
            return 1
        elif address.startswith("192.168.0"):  # Nova Wi-Fi
            return 2
        else:  # Other
            return 3

      response.ips = [address for interface, address in sorted(addresses.items(), key=key)]
      return response
    */
   std::vector<std::string> ips;
   response->ips = ips;
  }
};

int main(int argc, char * argv[])
{
  gst_init(&argc, &argv);
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<CameraStreamer>());
  rclcpp::shutdown();
  return 0;
}

