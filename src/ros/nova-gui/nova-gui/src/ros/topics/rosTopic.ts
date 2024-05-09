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
  TOF = "/control/analysis_platform",
  KILN_DATA = "/science/kiln_data",
  NIR_DATA = "/science/nir_probe_data",
  MICROSCOPE_SERVO = "/science/microscope_servo_info",
  RAMAN_SPEC_MSG = "/science/raman_spec_msg",
  UV_VIS_SPEC = "/science/uv_vis_spec_data",
}
