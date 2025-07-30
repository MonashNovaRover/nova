# Behaviour Trees
For an introduction on behaviour trees, please visit the [Notion page](https://www.notion.so/Decision-Making-231b7139617180be90ffda7d311d8ff2).

For convenience, instructions detailing how to create custom nodes and behaviour trees will be reiterated here.

## How to Create a Custom Node
To create your own custom node, there are 4 things you must do:

### 1. Create your `.hpp` and `.cpp` implementation under `include/nova_behaviour_tree` and `plugins` respectively
At the bottom of your `.cpp` file, you should have something like:
```cpp
...

#include "behaviortree_cpp/bt_factory.h"
BT_REGISTER_NODES(factory)
{
  factory.registerNodeType<nova_behavior_tree::SnapInCollisionGoalsAction>("SnapInCollisionGoals");
}
```

### 2. Add your node to the CMakeLists.txt
```cmake
...

add_library(nova_snap_in_collision_goals_action_bt_node SHARED plugins/action/snap_in_collision_goals_action.cpp)
list(APPEND plugin_libs nova_snap_in_collision_goals_action_bt_node)

...
```

### 3. Define your node using XML in the bt_nodes.xml
```xml
...

<Action ID="SnapInCollisionGoals">
  <input_port name="goals_offset">Approximate distance goals are offset</input_port>
  <input_port name="max_snap_radius">Maximum radius (m) to snap goals to</input_port>
  <input_port name="input_goals">Original goals to snap if in collision</input_port>
  <output_port name="output_goals">Goals with all in collision goals snapped</output_port>
</Action>

...
```

### 4. Add your node to the bt_navigator.yaml under nav2_arch / nav2_urc
The name is the same as the one you defined in the CMakeLists.txt.
```yaml
...
plugin_lib_names:
  - ...
  - nova_snap_in_collision_goals_action_bt_node
  - ...
```

## Creating a Behaviour Tree

To create a behaviour tree, we generally use Nav2’s [Navigate To Pose](https://docs.nav2.org/behavior_trees/trees/nav_to_pose_recovery.html)
or [Navigate Through Poses](https://docs.nav2.org/behavior_trees/trees/nav_through_poses_recovery.html) as a base. From there, you can
directly edit the XML or you can use [Groot2](https://www.behaviortree.dev/groot/), an IDE for behaviour trees developed by BehaviourTree.CPP.

I tend to prefer directly editing the XML file and then viewing the result using Groot2, as editing via Groot2 results in a lot of unwanted
node metadata being added to your XML file, making it more difficult for you to parse when looking directly at the XML file.

### Installing Groot2

1. Change directory to our nixfiles directory with our `nixfiles` alias
2. Run `nix-build -A pkgs.groot2 -o ~/Builds/groot2 |& nom` to install it under `~/Builds/groot2`.

### Using Groot2

- To run Groot2, simply do `~/Builds/groot2/bin/groot2`.
- Make sure to import our bt_nodes.xml before loading a behaviour tree that uses nodes defined in bt_nodes.xml.
