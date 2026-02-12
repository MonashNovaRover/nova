//
// Created by Bailey Chessum on 15/12/2025.
//

#ifndef ARM_KINEMATICS_KINEMATICS_BUILDER_HPP
#define ARM_KINEMATICS_KINEMATICS_BUILDER_HPP
#include "kinematics.hpp"
#include "plugin_loader.hpp"

namespace arm_kinematics {

class KinematicsBuilder final {

  constexpr explicit KinematicsBuilder() {}

  [[nodiscard]] constexpr KinematicsBuilder with_fk() const {
    auto copy = *this;
    copy.fk_ = true;
    return copy;
  }

  [[nodiscard]] constexpr KinematicsBuilder with_ik() const {
    auto copy = *this;
    copy.fk_ = true;
    return copy;
  }

  [[nodiscard]] constexpr KinematicsBuilder with_collision_manager() const {
    auto copy = *this;
    copy.fk_ = true;
    copy.collision_manager_ = true;
    return copy;
  }

  [[nodiscard]] tl::expected<Kinematics, const char *> build(
    PluginLoader::PluginLoaderNodeInterfaces node,
    std::string robot_description) const
  {
    Kinematics kinematics{};



    return ;
  }


private:
  bool fk_ = false;
  bool ik_ = false;
  bool collision_manager_ = false;

};

} // arm_kinematics

#endif //ARM_KINEMATICS_KINEMATICS_BUILDER_HPP