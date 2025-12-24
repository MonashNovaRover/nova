#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <iostream>
#include <vector>
#include <algorithm>

#include "rclcpp/rclcpp.hpp"
#include "std_srvs/srv/empty.hpp"
#include <cameras3_msgs/srv/camera_operation.hpp>
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
- Camera stream stats service
- IP list service
*/

enum CameraState {STOP = 0, START = 1, PAUSE = 2, RESET = 3};

struct Pipeline
{
  std::string serial;
  //pipeline pointer
};

/* --- Prototype functions --- */
void get_properties(std::string serial);
void create_pipeline(std::string serial);
void pause_pipeline(std::string serial);
void destroy_pipeline(std::string serial);
/* --------------------------- */

class CameraStreamer : public rclcpp::Node
{
  public: CameraStreamer()
    : Node("camera_streamer")
    {
      start_service_ = this->create_service<cameras3_msgs::srv::CameraOperation>(
        "/camera_streamer/stream/start", 
        std::bind(&CameraStreamer::service_callback, 
          this, _1, _2, CameraState::START)
      );
      stop_service_ = this->create_service<cameras3_msgs::srv::CameraOperation>(
        "/camera_streamer/stream/stop", 
        std::bind(&CameraStreamer::service_callback, 
          this, _1, _2, CameraState::STOP)
      );
      pause_service_ = this->create_service<cameras3_msgs::srv::CameraOperation>(
        "/camera_streamer/stream/pause", 
        std::bind(&CameraStreamer::service_callback, 
          this, _1, _2, CameraState::PAUSE)
      );
      RCLCPP_INFO(this->get_logger(), "Cameras3 Streamer Running...");
    }

  rclcpp::Service<cameras3_msgs::srv::CameraOperation>::SharedPtr start_service_;
  rclcpp::Service<cameras3_msgs::srv::CameraOperation>::SharedPtr stop_service_;
  rclcpp::Service<cameras3_msgs::srv::CameraOperation>::SharedPtr pause_service_;
  std::vector<Pipeline> pipelines;

  private: void service_callback(
    const std::shared_ptr<cameras3_msgs::srv::CameraOperation::Request> request,
    std::shared_ptr<cameras3_msgs::srv::CameraOperation::Response> response,
    CameraState state)  
    {
      bool success = false;
      switch (state) {
        case CameraState::START:
          for (std::string serial : request->serials) {
            bool pipeline_exists = std::any_of(pipelines.begin(), pipelines.end(), 
              [serial](const Pipeline p){
                return p.serial == serial;
              });
            if (pipeline_exists) {
              // gstreamer play pipeline if paused, if currently playing remake pipeline
            }
            else {
              // make new pipeline
            }
          }
          break;
        case CameraState::STOP:
          for (std::string serial : request->serials) {
            bool pipeline_exists = std::any_of(pipelines.begin(), pipelines.end(), 
              [serial](const Pipeline p){
                return p.serial == serial;
              });
            if (pipeline_exists) {
              pause_pipeline(serial);
            }
          }
          break;
        case CameraState::PAUSE:
          for (std::string serial : request->serials) {
            bool pipeline_exists = std::any_of(pipelines.begin(), pipelines.end(), 
              [serial](const Pipeline p){
                return p.serial == serial;
              });
            if (pipeline_exists) {
              // gstreamer pause pipeline if playing
            }
          }
          break;
      }
      response->success = success;
    }
};

void get_properties(std::string serial)
{}

void create_pipeline(std::string serial)
{}

void pause_pipeline(std::string serial)
{}

void destroy_pipeline(std::string serial)
{}

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<CameraStreamer>());
  rclcpp::shutdown();
  return 0;
}