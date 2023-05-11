cmake_minimum_required(VERSION 3.5)
project(core)

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
find_package(geometry_msgs REQUIRED)

# Find default generators
find_package(rosidl_default_generators REQUIRED)
find_package(std_msgs REQUIRED)

# Include all interfaces
rosidl_generate_interfaces(${PROJECT_NAME}

  # Message definitions
  "msg/AlvarMarker.msg"
  "msg/AlvarMarkers.msg"
  "msg/ArmControlScheme.msg"
  "msg/EndEffectorInput.msg"
  "msg/CamsConnected.msg"
  "msg/CMDFeedback.msg"
  "msg/CMDsFeedback.msg"
  "msg/DistanceData.msg"
  "msg/DriveInfo.msg"
  "msg/DriveInput.msg"
  "msg/HydraprobeData.msg"
  "msg/EMCData.msg"
  "msg/InputGamepad.msg"
  "msg/InputJoystick.msg"
  "msg/Point2D.msg"
  "msg/RadioStatus.msg"
  "msg/RoverPoseGPS.msg"
  "msg/SpectrometerData.msg"
  "msg/AutonomousGoal.msg"
  "msg/AutonomousGoalArray.msg"
  "msg/PivotWheelData.msg"
  "msg/SingleTelemetry.msg"
  "msg/Telemetry.msg"
  "msg/BLCMDStatus.msg"
  "msg/BLCMDStatusArray.msg"
  "msg/BLCMDReset.msg"
  "msg/ActuatorLimit.msg"

  "msg/Heartbeat.msg"
  "msg/KilnMassData.msg"
  "msg/KilnTempData.msg"
  "msg/WheelOdometry.msg"
  "msg/RoverPose.msg"

  "srv/ArmConfigInfo.srv"
  "srv/CameraCommand.srv"
  "srv/CMDCommand.srv"
  "srv/GimbalCommand.srv"
  "srv/PIDTune.srv"
  "srv/ScienceCommand.srv"
  "srv/LoadCellPoller.srv"
  "srv/RFIDCommand.srv"
  "srv/PathPlanningRequest.srv"
  "srv/StringTrigger.srv"

  # Action definitions
  DEPENDENCIES std_msgs geometry_msgs
)

# Add all the installation directories
install(DIRECTORY
  launch
  params
  rviz
  urdf
  meshes
  DESTINATION share/${PROJECT_NAME}
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
