// Copyright (c) 2021 Samsung Research America
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

#include <memory>

#include "geometry_msgs/msg/pose_stamped.hpp"
#include "nav2_util/robot_utils.hpp"

#include "nova_behavior_tree/emit_data_action_base.hpp"
#include "nova_behavior_tree/nav2_utils.hpp"

namespace nova_behavior_tree
{

    class DummyDetectCubeAction : public EmitDataActionBase<geometry_msgs::msg::PoseStamped>
    {
    public:
        DummyDetectCubeAction(
            const std::string &xml_tag_name,
            const BT::NodeConfiguration &conf)
            : EmitDataActionBase<geometry_msgs::msg::PoseStamped>(xml_tag_name, conf)
        {
        }

        void initialize() override
        {
            EmitDataActionBase<geometry_msgs::msg::PoseStamped>::initialize();

            tf_ = config().blackboard->get<std::shared_ptr<tf2_ros::Buffer>>("tf_buffer");
            node_->get_parameter("transform_tolerance", transform_tolerance_);
            nav2_util::getCurrentPose(current_pose_, *tf_, "map", "base_link", transform_tolerance_);
            
            cube_pose_ = current_pose_;
            cube_pose_.pose.position.x += 5.0;
            cube_pose_.pose.position.y += 3.0;
        }

    protected:
        geometry_msgs::msg::PoseStamped generateData() override
        {
            RCLCPP_INFO(node_->get_logger(), "Detected cube after %.2lfs%s", delay_, poseStampedToString(cube_pose_).c_str());
            return cube_pose_;
        }

    private:
        std::shared_ptr<tf2_ros::Buffer> tf_;
        double transform_tolerance_;
        geometry_msgs::msg::PoseStamped current_pose_;
        geometry_msgs::msg::PoseStamped cube_pose_;
    };

} // namespace nova_behavior_tree

#include "behaviortree_cpp/bt_factory.h"
BT_REGISTER_NODES(factory)
{
  factory.registerNodeType<nova_behavior_tree::DummyDetectCubeAction>("DummyDetectCube");
}