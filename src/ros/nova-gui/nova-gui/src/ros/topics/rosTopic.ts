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
  TELEMETRY = "/control/telemetry",

  // Camera Related topics
  CAMERAS = "/camera_directory/cameras",

  // Science Related
  KILN_DATA = "/science/kiln_data",
}
