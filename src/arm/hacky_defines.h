// Hacky defines to bypass arm modularity
// Delete these after FK / IK is working on cycloidal wrist with ES end effector

// Files where this is included:
// arm_simulator.cpp
// arm_kinematics.cpp

#include <string>
#include <vector>

namespace hack
{
    std::vector<std::string> JOINT_NAMES {"base-rotation", "shoulder", "elbow", "j4", "j5", "j6"};
    std::vector<std::string> ENDPOINT_NAMES {"j4-hook", "squooshy", "gripper", "cam-front", "cam-depth", "cam-screw"};
}
