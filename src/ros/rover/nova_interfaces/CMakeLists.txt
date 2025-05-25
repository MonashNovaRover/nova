cmake_minimum_required(VERSION 3.5)
project(nova_interfaces)

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(std_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
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
  "msg/Status.msg"

  # Service definitions
  "srv/CacheCommand.srv"
  "srv/GpsOffset.srv"
  "srv/KilnCommand.srv"
  "srv/LoadCellPoller.srv"
  "srv/MoveHydraprobe.srv"
  "srv/MoveMicroscopeServo.srv"
  "srv/MoveScimbalCam.srv"
  "srv/RFIDCommand.srv"
  "srv/ScienceCommand.srv"
  "srv/TakeNIRProbeReading.srv"
  "srv/RamanSpec.srv"
  "srv/RamanMech.srv"
  "srv/RGBInput.srv"
  "srv/CartographerCommand.srv"

  # Action definitions
  "action/Pumps.action"
  "action/Stepper.action"
  "action/URC2025Navigator.action"

  DEPENDENCIES std_msgs geometry_msgs nav_msgs action_msgs
)

ament_package()