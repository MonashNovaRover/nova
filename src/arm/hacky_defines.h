// Hacky defines to bypass arm modularity
// Delete these after FK / IK is working on cycloidal wrist with ES end effector
// Information will be set either by kinematics or input nodes

#include <string>
#include <vector>

namespace hack
{
    std::vector<std::string> JOINT_NAMES {"base-rotation", "shoulder", "elbow", "j4", "j5", "j6"};
    std::vector<std::string> ENDPOINT_NAMES {"j4-hook", "squooshy", "gripper", "cam-front", "cam-depth", "cam-screw"};
}
