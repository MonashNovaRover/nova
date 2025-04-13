//
// Created by Bailey Chessum on 4/5/25.
// Helper struct to just collate the different component command interfaces
//

#ifndef NOVA_IK_CONTROLLER_POSEHANDLE_HPP
#define NOVA_IK_CONTROLLER_POSEHANDLE_HPP

#include <hardware_interface/loaned_command_interface.hpp>
#include <tf2/LinearMath/Vector3.h>
#include <tf2/LinearMath/Quaternion.h>
#include <tf2/LinearMath/Transform.h>

namespace nova_twistmapper {

//
  class PoseHandle {
  public:
    struct ComponentHandle {
      ComponentHandle(std::reference_wrapper<hardware_interface::LoanedCommandInterface> command) : command(command) {
      }

      std::reference_wrapper<hardware_interface::LoanedCommandInterface> command;
    };

    struct VectorHandle {
      VectorHandle(std::reference_wrapper<hardware_interface::LoanedCommandInterface> x,
                   std::reference_wrapper<hardware_interface::LoanedCommandInterface> y,
                   std::reference_wrapper<hardware_interface::LoanedCommandInterface> z)
        : x(x), y(y), z(z) {}

      void set_value(const tf2::Vector3 &quat) const;

      ComponentHandle x;
      ComponentHandle y;
      ComponentHandle z;
    };

    struct QuatHandle {
      QuatHandle(std::reference_wrapper<hardware_interface::LoanedCommandInterface> x,
                 std::reference_wrapper<hardware_interface::LoanedCommandInterface> y,
                 std::reference_wrapper<hardware_interface::LoanedCommandInterface> z,
                 std::reference_wrapper<hardware_interface::LoanedCommandInterface> w)
        : x(x), y(y), z(z), w(w) {}

      void set_value(const tf2::Quaternion &quat) const;

      ComponentHandle x;
      ComponentHandle y;
      ComponentHandle z;
      ComponentHandle w;
    };

    PoseHandle(std::reference_wrapper<hardware_interface::LoanedCommandInterface> x,
               std::reference_wrapper<hardware_interface::LoanedCommandInterface> y,
               std::reference_wrapper<hardware_interface::LoanedCommandInterface> z,
               std::reference_wrapper<hardware_interface::LoanedCommandInterface> qx,
               std::reference_wrapper<hardware_interface::LoanedCommandInterface> qy,
               std::reference_wrapper<hardware_interface::LoanedCommandInterface> qz,
               std::reference_wrapper<hardware_interface::LoanedCommandInterface> qw)
               : origin(x, y, z), rotation(qx, qy, qz, qw) {}

    void set_value(const tf2::Transform &transform) const;

    VectorHandle origin;
    QuatHandle rotation;

  /**
   * @brief Assigns command interfaces given the list of available command interfaces and a string suffix for the name
   * of each component
   */
  static PoseHandle pose_handle_from_command_interfaces(
    std::vector<hardware_interface::LoanedCommandInterface> &command_interfaces,
    const std::string &prefix);

protected:
  static std::reference_wrapper<hardware_interface::LoanedCommandInterface> find_command_interface(
    std::vector<hardware_interface::LoanedCommandInterface> &command_interfaces, const std::string &name);

};
}

#endif //NOVA_IK_CONTROLLER_POSEHANDLE_HPP
