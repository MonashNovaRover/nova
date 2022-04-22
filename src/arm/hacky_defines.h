// Hacky defines to bypass arm modularity
// Information will be set either by kinematics or input nodes. Possibly using a service so other nodes can query for the info
// Currently assumes the ES end effector

#include <string>
#include <vector>

namespace hack
{
    std::vector<std::string> JOINT_NAMES {"base-rotation", "shoulder", "elbow", "j4", "j5", "j6"};
    std::vector<std::string> ENDPOINT_NAMES {"j4-hook", "squooshy", "gripper", "cam-front", "cam-depth", "cam-screw"};
}
