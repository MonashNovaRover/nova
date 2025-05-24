// Copyright (c) 2025 Monash Nova Rover
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @brief Base class for conditions that check whether an object has been detected.
 * Performs filtering on raw detections by clustering them based on proximity and
 * only publishing clusters that have at least x detections in the last y seconds.
 * 
 * Works specifically for subscribing to a topic on which detections are published.
 * 
 * When inheriting from this class, you should:
 * - Implement the callback method to process the incoming messages
 *    - IMOPRTANT: The callback method must add the received detections to the `raw_detections_` vector.
 * - Optionally override the log_detections() method to log detection information
 * - Optionally override the clear_processed_detections() method to clear any information
 * relating to processed detections
 * 
 * @authors Terry Tian
 * @date 25/05/2025 (Created)
 * @date 25/05/2025 (Last Modified)
 */

#ifndef NOVA_BEHAVIOR_TREE__PLUGINS__CONDITION__DETECTED_CONDITION_BASE_HPP_
#define NOVA_BEHAVIOR_TREE__PLUGINS__CONDITION__DETECTED_CONDITION_BASE_HPP_

#include <string>
#include <vector>
#include <functional>
#include <queue>

#include "geometry_msgs/msg/pose_stamped.hpp"
#include "geometry_msgs/msg/pose.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp/callback_group.hpp"
#include "rclcpp/executors/multi_threaded_executor.hpp"
#include "rclcpp/subscription.hpp"
#include "tf2/LinearMath/Vector3.h"
#include "tf2_geometry_msgs/tf2_geometry_msgs.hpp"

#include "behaviortree_cpp/condition_node.h"

namespace nova_behavior_tree
{

  using namespace geometry_msgs::msg;

  struct Cluster
  {
    tf2::Vector3 centroid;
    std::queue<PoseStamped> goals;
  };

  template<typename MsgT>
  class DetectedConditionBase : public BT::ConditionNode
  {

  public:
    typedef PoseStamped Goal;
    typedef std::vector<Goal> Goals;

    /**
     * @brief A constructor for nova_behavior_tree::DetectedConditionBase
     * @param condition_name Name for the XML tag for this node
     * @param conf BT node configuration
     */
    DetectedConditionBase(
      const std::string &condition_name,
      const BT::NodeConfiguration &conf)
      : BT::ConditionNode(condition_name, conf) {}

    /**
     * @brief The main override required by a BT action
     * @return BT::NodeStatus Status of tick execution
     */
    virtual BT::NodeStatus tick() override
    {
      if (!initialized_)
      {
        initialize();
      }

      callback_group_executor_.spin_some();

      if (process_detections())
      {
        return BT::NodeStatus::SUCCESS;
      }
      return BT::NodeStatus::FAILURE;
    }

    /**
     * @brief Creates list of BT ports
     * @return BT::PortsList Containing node-specific ports
     */
    static BT::PortsList providedPorts()
    {
      return 
      {
        BT::InputPort<std::string>("detections_topic", "Topic to subscribe to for detections"),
        BT::InputPort<int>("min_detections", 2, "Minimum number of detections within buffer time to be considered valid"),
        BT::InputPort<double>("buffer_time", 5.0, "Keep track of detections in the last n seconds"),
        BT::InputPort<double>("cluster_radius", 1.5, "Radius for a detection to be considered part of a cluster"),
        BT::OutputPort<Goals>("detections", "Detected poses"),
      };
    }
  
  protected:
    /**
     * @brief Method to read parameters and initialize class variables
     */
    void initialize()
    {
      node_ = config().blackboard->get<rclcpp::Node::SharedPtr>("node");
      
      getInput("detections_topic", detections_topic_);
      getInput("min_detections", min_detections_);
      getInput("buffer_time", buffer_time_);
      getInput("cluster_radius", cluster_radius_);

      callback_group_ = node_->create_callback_group(
        rclcpp::CallbackGroupType::MutuallyExclusive, 
        false
      );
      callback_group_executor_.add_callback_group(callback_group_, node_->get_node_base_interface());

      rclcpp::SubscriptionOptions sub_option;
      sub_option.callback_group = callback_group_;

      subscription_ = node_->create_subscription<MsgT>(
        detections_topic_, 
        rclcpp::SystemDefaultsQoS(), 
        std::bind(&DetectedConditionBase::callback, this, std::placeholders::_1), 
        sub_option
      );

      initialized_ = true;
    }

    /**
     * @brief Callback method for the subscription to the detection topic, must be implemented by derived classes
     * IMPORTANT: This method must add the received detections to the `raw_detections_` vector.
     * 
     * @param msg The message received from the topic
     */
    virtual void callback(const typename MsgT::SharedPtr msg) = 0;

    /**
     * @brief Clear processed detections info, called at the end of process_detections()
     */
    virtual void clear_processed_detections()
    {
      raw_detections_.clear();
    }

    /**
     * @brief Method to log detection information, can be overridden by derived classes
     */
    virtual void log_detections() {};

    rclcpp::Node::SharedPtr node_;
    rclcpp::CallbackGroup::SharedPtr callback_group_;
    rclcpp::executors::MultiThreadedExecutor callback_group_executor_;
    typename rclcpp::Subscription<MsgT>::SharedPtr subscription_;

    std::string detections_topic_;
    int min_detections_;
    double buffer_time_;
    double cluster_radius_;
    Goals raw_detections_;

  private:
    /**
     * @brief Filters and publsihes confident detections
     * @return true if at least one detection is valid, false otherwise
     */
    bool process_detections()
    {
      if (raw_detections_.empty())
      {
        return false;
      }

      // cluster detections based on proximity
      for (const auto &goal : raw_detections_)
      {
        bool found_cluster = false;
        tf2::Vector3 detected_point;
        tf2::fromMsg(goal.pose.position, detected_point);
        // try to find an existing cluster for the detected point
        for (auto &cluster : detection_clusters_)
        {
          if (cluster.centroid.distance(detected_point) < cluster_radius_)
          {
            cluster.centroid = (cluster.centroid * cluster.goals.size() + detected_point) / (cluster.goals.size() + 1);
            cluster.goals.push(goal);
            found_cluster = true;
            break;
          }
        }
        // create a new cluster if no existing one was found
        if (!found_cluster)
        {
          Cluster new_cluster;
          new_cluster.centroid = detected_point;
          new_cluster.goals.push(goal);
          detection_clusters_.push_back(new_cluster);
        }
      }

      // remove outdated detections
      for (size_t i = 0; i < detection_clusters_.size();)
      {
        while (!detection_clusters_[i].goals.empty() && 
               (node_->now() - detection_clusters_[i].goals.front().header.stamp).seconds() > buffer_time_)
        {
          detection_clusters_[i].goals.pop();
        }

        if (detection_clusters_[i].goals.empty())
        {
          detection_clusters_.erase(detection_clusters_.begin() + i);
        }
        else
        {
          ++i; // only increment if we didn't erase
        }
      }
      
      // filter clusters based on minimum detections
      Goals filtered_detections_;
      for (const auto &cluster : detection_clusters_) {
        if (cluster.goals.size() >= min_detections_) {
          Goal detection_goal = cluster.goals.back();
          tf2::toMsg(cluster.centroid, detection_goal.pose.position);
          filtered_detections_.push_back(detection_goal);
        }
      }
      
      // publish the filtered detections
      setOutput("detections", filtered_detections_);
      log_detections();
      clear_processed_detections();
      
      return true;
    }

    std::vector<Cluster> detection_clusters_;
    bool initialized_ = false;
  };

} // namespace nova_behavior_tree

#endif // NOVA_BEHAVIOR_TREE__PLUGINS__CONDITION__DETECTED_CONDITION_BASE_HPP_
