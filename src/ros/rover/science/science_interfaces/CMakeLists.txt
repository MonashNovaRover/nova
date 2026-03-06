cmake_minimum_required(VERSION 3.5)
project(science_interfaces)

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(rosidl_default_generators REQUIRED)

# uncomment the following section in order to fill in
# further dependencies manually.
# e.g.
# find_package(std_msgs REQUIRED)

#find_package(<dependency> REQUIRED)

# Include all interfaces
rosidl_generate_interfaces(${PROJECT_NAME}
  # Message definitions
  "msg/ActuatorLimit.msg"
  "msg/BMESensor.msg"
  "msg/EMCData.msg"
  "msg/HydraprobeData.msg"
  "msg/EffortStatus.msg"
  "msg/KilnData.msg"
  "msg/MicroscopeServoInfo.msg"
  "msg/NIRProbeData.msg"
  "msg/RamanSpectrum.msg"
  "msg/RamanState.msg"
  "msg/SpectrometerData.msg"
  "msg/UVVisSpecData.msg"

  # Service definitions
  "srv/CacheCommand.srv"
  "srv/EffortCommand.srv"
  "srv/KilnCommand.srv"
  "srv/MoveHydraprobe.srv"
  "srv/MoveMicroscopeServo.srv"
  "srv/MoveScimbalCam.srv"
  "srv/RamanSpec.srv"
  "srv/RamanMech.srv"
  "srv/ScienceCommand.srv"
  "srv/SetPosition.srv"
  "srv/TakeNIRProbeReading.srv"

  # Action definitions
  "action/Pumps.action"
  "action/Stepper.action"

  # add any msg dependencies here
  # e.g. DEPENDENCIES std_msgs geometry_msgs
  DEPENDENCIES
)

ament_package()