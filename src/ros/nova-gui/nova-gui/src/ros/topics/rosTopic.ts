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
  DRIVE_INFO = "/control/drive_info",
  DRIVE_TELEMETRY = "/control/telemetry",

  // Arm related topics
  ARM_TELEMETRY = "/electronics/cmd_feedback",
  RFID_DATA = "/electronics/rfid/data",

  // Camera Related topics
  CAMERAS = "/camera_directory/cameras",

  // Science Related topics
  KILN_DATA = "/science/kiln_data",
  NIR_DATA = "/science/nir_probe_data",
  MICROSCOPE_SERVO = "/science/microscope_servo_info",
}
