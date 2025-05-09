/**
 * A Single Enum which Represents all ROS Topics that Bifrost can Listen to
 *
 * To Listen to New Topics from Bifrost, Add thhe Topic as an enum below
 *
 * An Example Topic: 'topics/demo' has been added here as DEMO_TOPIC and used throughout the examples
 */

export enum RosTopic {
  // NULL TOPIC to Test Rosbridge Connections
  NULL_TOPIC = "",
  // ROS Topics
  POSE = "/pose",

  // Drive related topics
  DRIVE_INFO = "/drive/drive_info",
  DRIVE_TELEMETRY = "/drive/telemetry",

  // Arm related topics
  ARM_TELEMETRY = "/cmds/cmd_feedback",
  RFID_DATA = "/electronics/rfid/data",

  // Camera Related topics
  CAMERAS = "/camera_directory/cameras",

  // Error Related Topics
  BLCMD_ERRORS = "/blcmds/blcmd_status",

  // Science Related topics
  TOF = "/science/analysis_arm",
  KILN_DATA = "/science/kiln_data",
  NIR_DATA = "/science/nir_probe_data",
  MICROSCOPE_SERVO = "/science/microscope_servo_info",
  UV_VIS_SPEC = "/science/uv_vis_spec_data",
  THETA_360_CAM_IMAGE = "/science/theta360cam/image",
  HYDRAPROBE_DATA = "/science/hydraprobe_data",
  RAMAN_SPEC_MSG = "/science/raman_spec_msg",
  RAMAN_MECH_MSG = "/science/raman_mech_msg",
  BME_SENSOR = "/science/bme_sensor",

  // Mapping Related Topics

  // These Topics are for ED and Science
  ROVER_LOCATION = "/fix",
  BASE_LOCATION = "/gps_base/fix",

  BATTERY_STATE = "/battery_state",
  ACTIVATED_NODES = "/activated_nodes",
}
