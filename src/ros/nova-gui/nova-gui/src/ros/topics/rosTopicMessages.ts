import { RosTopic } from "./rosTopic";

/**
 * This Object maps ROS Topics found in `rosTopics.ts` to Plain ROS Message types
 * Not to be confused with `rosMessageTypes.ts`
 *
 * The Topic RosTopics.DEMO_TOPIC is being mapped to "custom_msgs/demo_msg"
 *
 * "custom_msgs/demo_msgs" comes purely from ROS
 */
export const rosTopicMessages = {
  [RosTopic.NULL_TOPIC]: "",
  [RosTopic.POSE]: "geometry_msgs/msg/Pose",

  // Drive Related
  [RosTopic.DRIVE_INFO]: "drive_interfaces/msg/DriveInfo",
  [RosTopic.DRIVE_TELEMETRY]: "blcmd_interfaces/msg/Telemetry",
  [RosTopic.DRIVE_JOINT_STATES]: "sensor_msgs/msg/JointState",

  // Arm Related
  [RosTopic.ARM_TELEMETRY]: "cmd_interfaces/msg/CMDsFeedback",
  [RosTopic.RFID_DATA]: "std_msgs/msg/String",
  [RosTopic.KEYBOARD_DATA]: "arm_interfaces/msg/KeyboardPoints",
  [RosTopic.TYPE_SEQUENCE]: "arm_interfaces/msg/SequencerFeedback",

  // Cameras Related
  [RosTopic.CAMERAS]: "camera_msgs/msg/Cameras",

  // Errors Related
  [RosTopic.BLCMD_ERRORS]: "blcmd_interfaces/msg/BLCMDStatusArray",

  // Science Related
  [RosTopic.TOF]: "sensor_msgs/msg/Range",
  [RosTopic.AA_POS]: "sensor_msgs/msg/Range",
  [RosTopic.THERMAL_DATA]: "science_interfaces/msg/ThermalData",
  [RosTopic.WATER_PUMP_STATUS]: "science_interfaces/msg/EffortStatus",
  [RosTopic.DIAPHRAGM_PUMP_STATUS]: "science_interfaces/msg/EffortStatus",
  [RosTopic.NIR_DATA]: "science_interfaces/msg/NIRProbeData",
  [RosTopic.MICROSCOPE_SERVO]: "science_interfaces/msg/MicroscopeServoInfo",
  [RosTopic.UV_VIS_SPEC]: "science_interfaces/msg/UVVisSpecData",
  [RosTopic.THETA_360_CAM_IMAGE]: "sensor_msgs/msg/CompressedImage",
  [RosTopic.HYDRAPROBE_DATA]: "science_interfaces/msg/HydraprobeData",
  [RosTopic.RAMAN_SPEC_MSG]: "science_interfaces/msg/RamanSpectrum",
  [RosTopic.RAMAN_MECH_MSG]: "science_interfaces/msg/RamanState",
  [RosTopic.BME_SENSOR]: "science_interfaces/msg/BMESensor",
  [RosTopic.AUGER1_DEPTH_SENSOR]: "std_msgs/msg/Bool",
  [RosTopic.AUGER2_DEPTH_SENSOR]: "std_msgs/msg/Bool",
  [RosTopic.TOOL_ROTATOR_ANGLE]: "std_msgs/msg/Float64",
  [RosTopic.PUMPS_STATUS]: "science_interfaces/msg/PumpStatus",
  [RosTopic.CAROUSEL_INNER_FEEDBACK]: "science_interfaces/msg/CarouselFeedback",
  [RosTopic.CAROUSEL_OUTER_FEEDBACK]: "science_interfaces/msg/CarouselFeedback",

  // Maps Related
  [RosTopic.ROVER_LOCATION]: "nova_interfaces/msg/GPSData",
  [RosTopic.BASE_LOCATION]: "nova_interfaces/msg/GPSData",
  [RosTopic.DRONE_LOCATION]: "nova_interfaces/msg/GPSData",
  [RosTopic.AUTO_STATUS]: "nova_interfaces/msg/Status",

  // Other
  [RosTopic.BATTERY_STATE]: "sensor_msgs/msg/BatteryState",
  [RosTopic.ACTIVATED_NODES]: "nova_interfaces/msg/ActiveNodeStatus",
  [RosTopic.RADIO_STATUS]: "nova_interfaces/msg/RadioStatus",
  [RosTopic.LOCKED_STATUS]: "nova_interfaces/msg/LockedStatus",
};
