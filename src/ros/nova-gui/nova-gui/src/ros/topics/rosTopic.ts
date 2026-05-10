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
  DRIVE_JOINT_STATES = "/drive/joint_states",

  // Arm related topics
  ARM_TELEMETRY = "/cmds/cmd_feedback",
  RFID_DATA = "/electronics/rfid/data",
  KEYBOARD_DATA = "/arm/keyboard/points",
  TYPE_SEQUENCE = "/arm/sequence",

  // Camera Related topics
  CAMERAS = "/camera_directory/cameras",

  // Error Related Topics
  BLCMD_ERRORS = "/blcmds/blcmd_status",

  // Science Related topics
  TOF = "/science/tof/distance",
  AA_POS = "/science/analysis_arm/position",
  THERMAL_DATA = "/science/thermal_data",
  WATER_PUMP_STATUS = "/science/water_pump_status",
  DIAPHRAGM_PUMP_STATUS = "/science/diaphragm_pump_status",
  NIR_DATA = "/science/nir_probe_data",
  MICROSCOPE_SERVO = "/science/microscope_servo_info",
  UV_VIS_SPEC = "/science/uv_vis_spec_data",
  THETA_360_CAM_IMAGE = "/science/theta360cam/image",
  HYDRAPROBE_DATA = "/science/hydraprobe_data",
  RAMAN_SPEC_MSG = "/science/raman_spec_msg",
  RAMAN_MECH_MSG = "/science/raman_mech_msg",
  BME_SENSOR = "/science/bme_sensor",
  AUGER_LEFT_DEPTH = "/science/auger_left/hall_effect",
  AUGER_RIGHT_DEPTH = "/science/auger_right/hall_effect",
  TOOL_ROTATOR_ANGLE = "/science/tool_rotator/position",
  PUMPS_STATUS = "/science/pumps/status",
  CAROUSEL_INNER_FEEDBACK = "/science/carousel_inner/feedback",
  CAROUSEL_OUTER_FEEDBACK = "/science/carousel_outer/feedback",
  LITMUS_DIPPER_STATUS = "/science/litmus_dipper/status",
  CAROUSEL_SEQUENCE_STATUS = "/science/carousel_sequence/status",

  // Mapping Related Topics
  ROVER_LOCATION = "/gps_rover/fix_custom",
  BASE_LOCATION = "/gps_base/fix_custom",
  DRONE_LOCATION = "/gps_drone/fix_custom",
  AUTO_STATUS = "/auto/status",

  // Other Topics
  BATTERY_STATE = "/battery_state",
  ACTIVATED_NODES = "/activated_nodes",
  RADIO_STATUS = "/chassis/radio_status",
  LOCKED_STATUS = "/locked_status",

  // Joy
  DRIVE_JOY = "/drive/joy",
  ARM_JOY_LEFT = "/arm/joy/left",
  ARM_JOY_RIGHT = "/arm/joy/right",

  // Vision Related Topics
  YOLO_DETECTIONS = "/yolo/object_detections",
}
