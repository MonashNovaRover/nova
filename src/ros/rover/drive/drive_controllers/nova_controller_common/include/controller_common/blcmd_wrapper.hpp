#ifndef NOVA_CONTROLLER_COMMON__BLCMD_WRAPPER_HPP_
#define NOVA_CONTROLLER_COMMON__BLCMD_WRAPPER_HPP_

#include <functional>
#include <vector>

#include "hardware_interface/loaned_state_interface.hpp"
#include "hardware_interface/loaned_command_interface.hpp"
#include "controller_interface/controller_interface.hpp"
#include "rclcpp/rclcpp.hpp"

namespace nova_controller_common
{
  struct WheelHandle
  {
    std::reference_wrapper<const hardware_interface::LoanedStateInterface> state;
    std::reference_wrapper<hardware_interface::LoanedCommandInterface> command;
  };

  class BLCMDWrapper
  {
  public:
    BLCMDWrapper() = default;

    controller_interface::CallbackReturn configure_drive_and_pivots(
        const std::vector<std::string> &left_drive_names, const std::vector<std::string> &right_drive_names,
        const std::vector<std::string> &left_pivot_names, const std::vector<std::string> &right_pivot_names,
        const char *drive_feedback_type, const char *pivot_feedback_type)
    {
      // Configure the handles based on the new names and feedback types
    }
    
  private:
    controller_interface::CallbackReturn configure_drive_and_pivot(
        const std::vector<std::string> &wheel_names, std::vector<WheelHandle> &registered_handles,
        const char *feedback_type)
    {
      // Register the wheel handles based on the provided names and feedback type
    }

    std::vector<WheelHandle> registered_left_drive_handles_;
    std::vector<WheelHandle> registered_right_drive_handles_;
    std::vector<WheelHandle> registered_left_pivot_handles_;
    std::vector<WheelHandle> registered_right_pivot_handles_;
  };

} // namespace nova_controller_common

#endif // NOVA_CONTROLLER_COMMON__BLCMD_WRAPPER_HPP_