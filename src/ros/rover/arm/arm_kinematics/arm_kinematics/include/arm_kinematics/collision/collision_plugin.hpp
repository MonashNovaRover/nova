//
// Created by Bailey Chessum on 15/10/2025.
//

#ifndef ARM_KINEMATICS_COLLISION_PLUGIN_HPP
#define ARM_KINEMATICS_COLLISION_PLUGIN_HPP

#include <arm_kinematics/kinematics_plugins/forward_kinematics_plugin.hpp>

namespace arm_kinematics {

/**
 * Base class for plugins used to perform inverse kinematics.
 */
class CollisionPlugin {
public:
  bool initialize(ForwardKinematicsPlugin::SharedPtr fk) {
    if (!fk)
      return false;

    fk_ = fk;

    return on_initialize();
  }

  [[nodiscard]] const ForwardKinematicsPlugin::SharedPtr & get_fk() const noexcept;

protected:
  /**
   * Called when the collision plugin is created. Override this to add any set up logic for the collision plugin.
   *
   * \returns True if initialization was successful. False otherwise.
   */
  virtual bool on_initialize() = 0;

private:
  ForwardKinematicsPlugin::SharedPtr fk_ = nullptr;

};

} // arm_kinematics

#endif //ARM_KINEMATICS_COLLISION_PLUGIN_HPP
