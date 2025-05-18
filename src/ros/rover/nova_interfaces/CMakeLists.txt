cmake_minimum_required(VERSION 3.5)
project(nova_interfaces)

# Default to C99
if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 99)
endif()

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find dependencies
find_package(ament_cmake REQUIRED)
# uncomment the following section in order to fill in
# further dependencies manually.
# find_package(<dependency> REQUIRED)
find_package(std_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)

# Find default generators
find_package(rosidl_default_generators REQUIRED)

# Include all interfaces
rosidl_generate_interfaces(${PROJECT_NAME}

  # Message definitions
  "msg/ActiveNodeStatus.msg"
  "msg/ActuatorLimit.msg"
  "msg/DistanceData.msg"
  "msg/DriveInfo.msg"
  "msg/DriveInput.msg"
  "msg/DriveInputStamped.msg"
  "msg/EMCData.msg"
  "msg/Heartbeat.msg"
  "msg/HydraprobeData.msg"
  "msg/KilnData.msg"
  "msg/KilnMassData.msg"
  "msg/KilnTempData.msg"
  "msg/MicroscopeServoInfo.msg"
  "msg/NIRProbeData.msg"
  "msg/RoverPoseGPS.msg"
  "msg/SpectrometerData.msg"
  "msg/UVVisSpecData.msg"
  "msg/RamanSpectrum.msg"
  "msg/RamanState.msg"
  "msg/BMESensor.msg"
  "msg/ArmFkVelocityTargets.msg"

  # Service definitions
  "srv/CacheCommand.srv"
  "srv/GpsOffset.srv"
  "srv/KilnCommand.srv"
  "srv/LoadCellPoller.srv"
  "srv/MoveMicroscopeServo.srv"
  "srv/MoveScimbalCam.srv"
  "srv/RFIDCommand.srv"
  "srv/ScienceCommand.srv"
  "srv/TakeNIRProbeReading.srv"
  "srv/RamanSpec.srv"
  "srv/RamanMech.srv"

  # Action definitions
  "action/Stepper.action"
  "action/Pumps.action"
  "action/ArmPlanPath.action"
  DEPENDENCIES std_msgs geometry_msgs nav_msgs action_msgs
)

# Run build testing package nodes
if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # uncomment the line when a copyright and license is not present in all source files
  #set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # uncomment the line when this package is not in a git repo
  #set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
