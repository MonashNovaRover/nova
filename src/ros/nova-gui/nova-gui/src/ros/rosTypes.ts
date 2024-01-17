/* eslint-disable */
// These files were generated using "ros-typescript-generator"
export enum IRosRclInterfacesParameterTypeConst {
  PARAMETER_NOT_SET = 0,
  PARAMETER_BOOL = 1,
  PARAMETER_INTEGER = 2,
  PARAMETER_DOUBLE = 3,
  PARAMETER_STRING = 4,
  PARAMETER_BYTE_ARRAY = 5,
  PARAMETER_BOOL_ARRAY = 6,
  PARAMETER_INTEGER_ARRAY = 7,
  PARAMETER_DOUBLE_ARRAY = 8,
  PARAMETER_STRING_ARRAY = 9,
}

export enum IRosStatisticsMsgsStatisticDataTypeConst {
  STATISTICS_DATA_TYPE_UNINITIALIZED = 0,
  STATISTICS_DATA_TYPE_AVERAGE = 1,
  STATISTICS_DATA_TYPE_MINIMUM = 2,
  STATISTICS_DATA_TYPE_MAXIMUM = 3,
  STATISTICS_DATA_TYPE_STDDEV = 4,
  STATISTICS_DATA_TYPE_SAMPLE_COUNT = 5,
}

export interface IRosActionMsgsCancelGoalRequest {
  goal_info: IRosActionMsgsGoalInfo;
}

export interface IRosActionMsgsCancelGoalResponse {
  return_code: number;
  goals_canceling: IRosActionMsgsGoalInfo[];
}

export enum IRosActionMsgsCancelGoalResponseConst {
  ERROR_NONE = 0,
  ERROR_REJECTED = 1,
  ERROR_UNKNOWN_GOAL_ID = 2,
  ERROR_GOAL_TERMINATED = 3,
}

export interface IRosActionMsgsGoalInfo {
  goal_id: IRosUniqueIdentifierMsgsUuid;
  stamp: { sec: number; nanosec: number };
}

export interface IRosActionMsgsGoalStatus {
  goal_info: IRosActionMsgsGoalInfo;
  status: number;
}

export enum IRosActionMsgsGoalStatusConst {
  STATUS_UNKNOWN = 0,
  STATUS_ACCEPTED = 1,
  STATUS_EXECUTING = 2,
  STATUS_CANCELING = 3,
  STATUS_SUCCEEDED = 4,
  STATUS_CANCELED = 5,
  STATUS_ABORTED = 6,
}

export interface IRosActionMsgsGoalStatusArray {
  status_list: IRosActionMsgsGoalStatus[];
}

export interface IRosActionlibMsgsGoalId {
  stamp: { sec: number; nanosec: number };
  id: string;
}

export interface IRosActionlibMsgsGoalStatus {
  goal_id: IRosActionlibMsgsGoalId;
  status: number;
  text: string;
}

export enum IRosActionlibMsgsGoalStatusConst {
  PENDING = 0,
  ACTIVE = 1,
  PREEMPTED = 2,
  SUCCEEDED = 3,
  ABORTED = 4,
  REJECTED = 5,
  PREEMPTING = 6,
  RECALLING = 7,
  RECALLED = 8,
  LOST = 9,
}

export interface IRosActionlibMsgsGoalStatusArray {
  header: IRosStdMsgsHeader;
  status_list: IRosActionMsgsGoalStatus[];
}

export interface IRosBuiltinInterfacesDuration {
  sec: number;
  nanosec: number;
}

export interface IRosBuiltinInterfacesTime {
  sec: number;
  nanosec: number;
}

export interface IRosCompositionInterfacesListNodesResponse {
  full_node_names: string[];
  unique_ids: number[];
}

export interface IRosCompositionInterfacesLoadNodeRequest {
  package_name: string;
  plugin_name: string;
  node_name: string;
  node_namespace: string;
  log_level: number;
  remap_rules: string[];
  parameters: IRosRclInterfacesParameter[];
  extra_arguments: IRosRclInterfacesParameter[];
}

export interface IRosCompositionInterfacesLoadNodeResponse {
  success: boolean;
  error_message: string;
  full_node_name: string;
  unique_id: number;
}

export interface IRosCompositionInterfacesUnloadNodeRequest {
  unique_id: number;
}

export interface IRosCompositionInterfacesUnloadNodeResponse {
  success: boolean;
  error_message: string;
}

export interface IRosDiagnosticMsgsAddDiagnosticsRequest {
  load_namespace: string;
}

export interface IRosDiagnosticMsgsAddDiagnosticsResponse {
  success: boolean;
  message: string;
}

export interface IRosDiagnosticMsgsDiagnosticArray {
  header: IRosStdMsgsHeader;
  status: IRosDiagnosticMsgsDiagnosticStatus[];
}

export interface IRosDiagnosticMsgsDiagnosticStatus {
  level: number;
  name: string;
  message: string;
  hardware_id: string;
  values: IRosDiagnosticMsgsKeyValue[];
}

export enum IRosDiagnosticMsgsDiagnosticStatusConst {
  OK = 0,
  WARN = 1,
  ERROR = 2,
  STALE = 3,
}

export interface IRosDiagnosticMsgsKeyValue {
  key: string;
  value: string;
}

export interface IRosDiagnosticMsgsSelfTestResponse {
  id: string;
  passed: number;
  status: IRosDiagnosticMsgsDiagnosticStatus[];
}

export interface IRosGeometryMsgsAccel {
  linear: IRosGeometryMsgsVector3;
  angular: IRosGeometryMsgsVector3;
}

export interface IRosGeometryMsgsAccelStamped {
  header: IRosStdMsgsHeader;
  accel: IRosGeometryMsgsAccel;
}

export interface IRosGeometryMsgsAccelWithCovariance {
  accel: IRosGeometryMsgsAccel;
  covariance: number[];
}

export interface IRosGeometryMsgsAccelWithCovarianceStamped {
  header: IRosStdMsgsHeader;
  accel: IRosGeometryMsgsAccelWithCovariance;
}

export interface IRosGeometryMsgsInertia {
  m: number;
  com: IRosGeometryMsgsVector3;
  ixx: number;
  ixy: number;
  ixz: number;
  iyy: number;
  iyz: number;
  izz: number;
}

export interface IRosGeometryMsgsInertiaStamped {
  header: IRosStdMsgsHeader;
  inertia: IRosGeometryMsgsInertia;
}

export interface IRosGeometryMsgsPoint {
  x: number;
  y: number;
  z: number;
}

export interface IRosGeometryMsgsPoint32 {
  x: number;
  y: number;
  z: number;
}

export interface IRosGeometryMsgsPointStamped {
  header: IRosStdMsgsHeader;
  point: IRosGeometryMsgsPoint;
}

export interface IRosGeometryMsgsPolygon {
  points: IRosGeometryMsgsPoint32[];
}

export interface IRosGeometryMsgsPolygonStamped {
  header: IRosStdMsgsHeader;
  polygon: IRosGeometryMsgsPolygon;
}

export interface IRosGeometryMsgsPose {
  position: IRosGeometryMsgsPoint;
  orientation: IRosGeometryMsgsQuaternion;
}

export interface IRosGeometryMsgsPose2D {
  x: number;
  y: number;
  theta: number;
}

export interface IRosGeometryMsgsPoseArray {
  header: IRosStdMsgsHeader;
  poses: IRosGeometryMsgsPose[];
}

export interface IRosGeometryMsgsPoseStamped {
  header: IRosStdMsgsHeader;
  pose: IRosGeometryMsgsPose;
}

export interface IRosGeometryMsgsPoseWithCovariance {
  pose: IRosGeometryMsgsPose;
  covariance: number[];
}

export interface IRosGeometryMsgsPoseWithCovarianceStamped {
  header: IRosStdMsgsHeader;
  pose: IRosGeometryMsgsPoseWithCovariance;
}

export interface IRosGeometryMsgsQuaternion {
  x: number;
  y: number;
  z: number;
  w: number;
}

export interface IRosGeometryMsgsQuaternionStamped {
  header: IRosStdMsgsHeader;
  quaternion: IRosGeometryMsgsQuaternion;
}

export interface IRosGeometryMsgsTransform {
  translation: IRosGeometryMsgsVector3;
  rotation: IRosGeometryMsgsQuaternion;
}

export interface IRosGeometryMsgsTransformStamped {
  header: IRosStdMsgsHeader;
  child_frame_id: string;
  transform: IRosGeometryMsgsTransform;
}

export interface IRosGeometryMsgsTwist {
  linear: IRosGeometryMsgsVector3;
  angular: IRosGeometryMsgsVector3;
}

export interface IRosGeometryMsgsTwistStamped {
  header: IRosStdMsgsHeader;
  twist: IRosGeometryMsgsTwist;
}

export interface IRosGeometryMsgsTwistWithCovariance {
  twist: IRosGeometryMsgsTwist;
  covariance: number[];
}

export interface IRosGeometryMsgsTwistWithCovarianceStamped {
  header: IRosStdMsgsHeader;
  twist: IRosGeometryMsgsTwistWithCovariance;
}

export interface IRosGeometryMsgsVector3 {
  x: number;
  y: number;
  z: number;
}

export interface IRosGeometryMsgsVector3Stamped {
  header: IRosStdMsgsHeader;
  vector: IRosGeometryMsgsVector3;
}

export interface IRosGeometryMsgsWrench {
  force: IRosGeometryMsgsVector3;
  torque: IRosGeometryMsgsVector3;
}

export interface IRosGeometryMsgsWrenchStamped {
  header: IRosStdMsgsHeader;
  wrench: IRosGeometryMsgsWrench;
}

export interface IRosLifecycleMsgsChangeStateRequest {
  transition: IRosLifecycleMsgsTransition;
}

export interface IRosLifecycleMsgsChangeStateResponse {
  success: boolean;
}

export interface IRosLifecycleMsgsGetAvailableStatesResponse {
  available_states: IRosLifecycleMsgsState[];
}

export interface IRosLifecycleMsgsGetAvailableTransitionsResponse {
  available_transitions: IRosLifecycleMsgsTransitionDescription[];
}

export interface IRosLifecycleMsgsGetStateResponse {
  current_state: IRosLifecycleMsgsState;
}

export interface IRosLifecycleMsgsState {
  id: number;
  label: string;
}

export enum IRosLifecycleMsgsStateConst {
  PRIMARY_STATE_UNKNOWN = 0,
  PRIMARY_STATE_UNCONFIGURED = 1,
  PRIMARY_STATE_INACTIVE = 2,
  PRIMARY_STATE_ACTIVE = 3,
  PRIMARY_STATE_FINALIZED = 4,
  TRANSITION_STATE_CONFIGURING = 10,
  TRANSITION_STATE_CLEANINGUP = 11,
  TRANSITION_STATE_SHUTTINGDOWN = 12,
  TRANSITION_STATE_ACTIVATING = 13,
  TRANSITION_STATE_DEACTIVATING = 14,
  TRANSITION_STATE_ERRORPROCESSING = 15,
}

export interface IRosLifecycleMsgsTransition {
  id: number;
  label: string;
}

export enum IRosLifecycleMsgsTransitionConst {
  TRANSITION_CREATE = 0,
  TRANSITION_CONFIGURE = 1,
  TRANSITION_CLEANUP = 2,
  TRANSITION_ACTIVATE = 3,
  TRANSITION_DEACTIVATE = 4,
  TRANSITION_UNCONFIGURED_SHUTDOWN = 5,
  TRANSITION_INACTIVE_SHUTDOWN = 6,
  TRANSITION_ACTIVE_SHUTDOWN = 7,
  TRANSITION_DESTROY = 8,
  TRANSITION_ON_CONFIGURE_SUCCESS = 10,
  TRANSITION_ON_CONFIGURE_FAILURE = 11,
  TRANSITION_ON_CONFIGURE_ERROR = 12,
  TRANSITION_ON_CLEANUP_SUCCESS = 20,
  TRANSITION_ON_CLEANUP_FAILURE = 21,
  TRANSITION_ON_CLEANUP_ERROR = 22,
  TRANSITION_ON_ACTIVATE_SUCCESS = 30,
  TRANSITION_ON_ACTIVATE_FAILURE = 31,
  TRANSITION_ON_ACTIVATE_ERROR = 32,
  TRANSITION_ON_DEACTIVATE_SUCCESS = 40,
  TRANSITION_ON_DEACTIVATE_FAILURE = 41,
  TRANSITION_ON_DEACTIVATE_ERROR = 42,
  TRANSITION_ON_SHUTDOWN_SUCCESS = 50,
  TRANSITION_ON_SHUTDOWN_FAILURE = 51,
  TRANSITION_ON_SHUTDOWN_ERROR = 52,
  TRANSITION_ON_ERROR_SUCCESS = 60,
  TRANSITION_ON_ERROR_FAILURE = 61,
  TRANSITION_ON_ERROR_ERROR = 62,
  TRANSITION_CALLBACK_SUCCESS = 97,
  TRANSITION_CALLBACK_FAILURE = 98,
  TRANSITION_CALLBACK_ERROR = 99,
}

export interface IRosLifecycleMsgsTransitionDescription {
  transition: IRosLifecycleMsgsTransition;
  start_state: IRosLifecycleMsgsState;
  goal_state: IRosLifecycleMsgsState;
}

export interface IRosLifecycleMsgsTransitionEvent {
  timestamp: number;
  transition: IRosLifecycleMsgsTransition;
  start_state: IRosLifecycleMsgsState;
  goal_state: IRosLifecycleMsgsState;
}

export interface IRosNavMsgsGetMapResponse {
  map: IRosNavMsgsOccupancyGrid;
}

export interface IRosNavMsgsGetPlanRequest {
  start: IRosGeometryMsgsPoseStamped;
  goal: IRosGeometryMsgsPoseStamped;
  tolerance: number;
}

export interface IRosNavMsgsGetPlanResponse {
  plan: IRosNavMsgsPath;
}

export interface IRosNavMsgsGridCells {
  header: IRosStdMsgsHeader;
  cell_width: number;
  cell_height: number;
  cells: IRosGeometryMsgsPoint[];
}

export interface IRosNavMsgsLoadMapRequest {
  map_url: string;
}

export interface IRosNavMsgsLoadMapResponse {
  map: IRosNavMsgsOccupancyGrid;
  result: number;
}

export enum IRosNavMsgsLoadMapResponseConst {
  RESULT_SUCCESS = 0,
  RESULT_MAP_DOES_NOT_EXIST = 1,
  RESULT_INVALID_MAP_DATA = 2,
  RESULT_INVALID_MAP_METADATA = 3,
  RESULT_UNDEFINED_FAILURE = 255,
}

export interface IRosNavMsgsMapMetaData {
  map_load_time: { sec: number; nanosec: number };
  resolution: number;
  width: number;
  height: number;
  origin: IRosGeometryMsgsPose;
}

export interface IRosNavMsgsOccupancyGrid {
  header: IRosStdMsgsHeader;
  info: IRosNavMsgsMapMetaData;
  data: number[];
}

export interface IRosNavMsgsOdometry {
  header: IRosStdMsgsHeader;
  child_frame_id: string;
  pose: IRosGeometryMsgsPoseWithCovariance;
  twist: IRosGeometryMsgsTwistWithCovariance;
}

export interface IRosNavMsgsPath {
  header: IRosStdMsgsHeader;
  poses: IRosGeometryMsgsPoseStamped[];
}

export interface IRosNavMsgsSetMapRequest {
  map: IRosNavMsgsOccupancyGrid;
  initial_pose: IRosGeometryMsgsPoseWithCovarianceStamped;
}

export interface IRosNavMsgsSetMapResponse {
  success: boolean;
}

export interface IRosRclInterfacesDescribeParametersRequest {
  names: string[];
}

export interface IRosRclInterfacesDescribeParametersResponse {
  descriptors: IRosRclInterfacesParameterDescriptor[];
}

export interface IRosRclInterfacesFloatingPointRange {
  from_value: number;
  to_value: number;
  step: number;
}

export interface IRosRclInterfacesGetParameterTypesRequest {
  names: string[];
}

export interface IRosRclInterfacesGetParameterTypesResponse {
  types: number[];
}

export interface IRosRclInterfacesGetParametersRequest {
  names: string[];
}

export interface IRosRclInterfacesGetParametersResponse {
  values: IRosRclInterfacesParameterValue[];
}

export interface IRosRclInterfacesIntegerRange {
  from_value: number;
  to_value: number;
  step: number;
}

export interface IRosRclInterfacesListParametersRequest {
  prefixes: string[];
  depth: number;
}

export enum IRosRclInterfacesListParametersRequestConst {
  DEPTH_RECURSIVE = 0,
}

export interface IRosRclInterfacesListParametersResponse {
  result: IRosRclInterfacesListParametersResult;
}

export interface IRosRclInterfacesListParametersResult {
  names: string[];
  prefixes: string[];
}

export interface IRosRclInterfacesLog {
  stamp: { sec: number; nanosec: number };
  level: number;
  name: string;
  msg: string;
  file: string;
  function: string;
  line: number;
}

export enum IRosRclInterfacesLogConst {
  DEBUG = 10,
  INFO = 20,
  WARN = 30,
  ERROR = 40,
  FATAL = 50,
}

export interface IRosRclInterfacesParameter {
  name: string;
  value: IRosRclInterfacesParameterValue;
}

export interface IRosRclInterfacesParameterDescriptor {
  name: string;
  type: number;
  description: string;
  additional_constraints: string;
  read_only: boolean;
  dynamic_typing: boolean;
  floating_point_range: IRosRclInterfacesFloatingPointRange[];
  integer_range: IRosRclInterfacesIntegerRange[];
}

export interface IRosRclInterfacesParameterEvent {
  stamp: { sec: number; nanosec: number };
  node: string;
  new_parameters: IRosRclInterfacesParameter[];
  changed_parameters: IRosRclInterfacesParameter[];
  deleted_parameters: IRosRclInterfacesParameter[];
}

export interface IRosRclInterfacesParameterEventDescriptors {
  new_parameters: IRosRclInterfacesParameterDescriptor[];
  changed_parameters: IRosRclInterfacesParameterDescriptor[];
  deleted_parameters: IRosRclInterfacesParameterDescriptor[];
}

export interface IRosRclInterfacesParameterValue {
  type: number;
  bool_value: boolean;
  integer_value: number;
  double_value: number;
  string_value: string;
  byte_array_value: number[];
  bool_array_value: boolean[];
  integer_array_value: number[];
  double_array_value: number[];
  string_array_value: string[];
}

export interface IRosRclInterfacesSetParametersAtomicallyRequest {
  parameters: IRosRclInterfacesParameter[];
}

export interface IRosRclInterfacesSetParametersAtomicallyResponse {
  result: IRosRclInterfacesSetParametersResult;
}

export interface IRosRclInterfacesSetParametersRequest {
  parameters: IRosRclInterfacesParameter[];
}

export interface IRosRclInterfacesSetParametersResponse {
  results: IRosRclInterfacesSetParametersResult[];
}

export interface IRosRclInterfacesSetParametersResult {
  successful: boolean;
  reason: string;
}

export interface IRosRmwDdsCommonGid {
  data: number[];
}

export interface IRosRmwDdsCommonNodeEntitiesInfo {
  node_namespace: string;
  node_name: string;
  reader_gid_seq: IRosRmwDdsCommonGid[];
  writer_gid_seq: IRosRmwDdsCommonGid[];
}

export interface IRosRmwDdsCommonParticipantEntitiesInfo {
  gid: IRosRmwDdsCommonGid;
  node_entities_info_seq: IRosRmwDdsCommonNodeEntitiesInfo[];
}

export interface IRosRosgraphMsgsClock {
  clock: { sec: number; nanosec: number };
}

export interface IRosSensorMsgsBatteryState {
  header: IRosStdMsgsHeader;
  voltage: number;
  temperature: number;
  current: number;
  charge: number;
  capacity: number;
  design_capacity: number;
  percentage: number;
  power_supply_status: number;
  power_supply_health: number;
  power_supply_technology: number;
  present: boolean;
  cell_voltage: number[];
  cell_temperature: number[];
  location: string;
  serial_number: string;
}

export enum IRosSensorMsgsBatteryStateConst {
  POWER_SUPPLY_STATUS_UNKNOWN = 0,
  POWER_SUPPLY_STATUS_CHARGING = 1,
  POWER_SUPPLY_STATUS_DISCHARGING = 2,
  POWER_SUPPLY_STATUS_NOT_CHARGING = 3,
  POWER_SUPPLY_STATUS_FULL = 4,
  POWER_SUPPLY_HEALTH_UNKNOWN = 0,
  POWER_SUPPLY_HEALTH_GOOD = 1,
  POWER_SUPPLY_HEALTH_OVERHEAT = 2,
  POWER_SUPPLY_HEALTH_DEAD = 3,
  POWER_SUPPLY_HEALTH_OVERVOLTAGE = 4,
  POWER_SUPPLY_HEALTH_UNSPEC_FAILURE = 5,
  POWER_SUPPLY_HEALTH_COLD = 6,
  POWER_SUPPLY_HEALTH_WATCHDOG_TIMER_EXPIRE = 7,
  POWER_SUPPLY_HEALTH_SAFETY_TIMER_EXPIRE = 8,
  POWER_SUPPLY_TECHNOLOGY_UNKNOWN = 0,
  POWER_SUPPLY_TECHNOLOGY_NIMH = 1,
  POWER_SUPPLY_TECHNOLOGY_LION = 2,
  POWER_SUPPLY_TECHNOLOGY_LIPO = 3,
  POWER_SUPPLY_TECHNOLOGY_LIFE = 4,
  POWER_SUPPLY_TECHNOLOGY_NICD = 5,
  POWER_SUPPLY_TECHNOLOGY_LIMN = 6,
}

export interface IRosSensorMsgsCameraInfo {
  header: IRosStdMsgsHeader;
  height: number;
  width: number;
  distortion_model: string;
  d: number[];
  k: number[];
  r: number[];
  p: number[];
  binning_x: number;
  binning_y: number;
  roi: IRosSensorMsgsRegionOfInterest;
}

export interface IRosSensorMsgsChannelFloat32 {
  name: string;
  values: number[];
}

export interface IRosSensorMsgsCompressedImage {
  header: IRosStdMsgsHeader;
  format: string;
  data: number[];
}

export interface IRosSensorMsgsFluidPressure {
  header: IRosStdMsgsHeader;
  fluid_pressure: number;
  variance: number;
}

export interface IRosSensorMsgsIlluminance {
  header: IRosStdMsgsHeader;
  illuminance: number;
  variance: number;
}

export interface IRosSensorMsgsImage {
  header: IRosStdMsgsHeader;
  height: number;
  width: number;
  encoding: string;
  is_bigendian: number;
  step: number;
  data: number[];
}

export interface IRosSensorMsgsImu {
  header: IRosStdMsgsHeader;
  orientation: IRosGeometryMsgsQuaternion;
  orientation_covariance: number[];
  angular_velocity: IRosGeometryMsgsVector3;
  angular_velocity_covariance: number[];
  linear_acceleration: IRosGeometryMsgsVector3;
  linear_acceleration_covariance: number[];
}

export interface IRosSensorMsgsJointState {
  header: IRosStdMsgsHeader;
  name: string[];
  position: number[];
  velocity: number[];
  effort: number[];
}

export interface IRosSensorMsgsJoy {
  header: IRosStdMsgsHeader;
  axes: number[];
  buttons: number[];
}

export interface IRosSensorMsgsJoyFeedback {
  type: number;
  id: number;
  intensity: number;
}

export enum IRosSensorMsgsJoyFeedbackConst {
  TYPE_LED = 0,
  TYPE_RUMBLE = 1,
  TYPE_BUZZER = 2,
}

export interface IRosSensorMsgsJoyFeedbackArray {
  array: IRosSensorMsgsJoyFeedback[];
}

export interface IRosSensorMsgsLaserEcho {
  echoes: number[];
}

export interface IRosSensorMsgsLaserScan {
  header: IRosStdMsgsHeader;
  angle_min: number;
  angle_max: number;
  angle_increment: number;
  time_increment: number;
  scan_time: number;
  range_min: number;
  range_max: number;
  ranges: number[];
  intensities: number[];
}

export interface IRosSensorMsgsMagneticField {
  header: IRosStdMsgsHeader;
  magnetic_field: IRosGeometryMsgsVector3;
  magnetic_field_covariance: number[];
}

export interface IRosSensorMsgsMultiDofJointState {
  header: IRosStdMsgsHeader;
  joint_names: string[];
  transforms: IRosGeometryMsgsTransform[];
  twist: IRosGeometryMsgsTwist[];
  wrench: IRosGeometryMsgsWrench[];
}

export interface IRosSensorMsgsMultiEchoLaserScan {
  header: IRosStdMsgsHeader;
  angle_min: number;
  angle_max: number;
  angle_increment: number;
  time_increment: number;
  scan_time: number;
  range_min: number;
  range_max: number;
  ranges: IRosSensorMsgsLaserEcho[];
  intensities: IRosSensorMsgsLaserEcho[];
}

export interface IRosSensorMsgsNavSatFix {
  header: IRosStdMsgsHeader;
  status: IRosSensorMsgsNavSatStatus;
  latitude: number;
  longitude: number;
  altitude: number;
  position_covariance: number[];
  position_covariance_type: number;
}

export enum IRosSensorMsgsNavSatFixConst {
  COVARIANCE_TYPE_UNKNOWN = 0,
  COVARIANCE_TYPE_APPROXIMATED = 1,
  COVARIANCE_TYPE_DIAGONAL_KNOWN = 2,
  COVARIANCE_TYPE_KNOWN = 3,
}

export interface IRosSensorMsgsNavSatStatus {
  status: number;
  service: number;
}

export enum IRosSensorMsgsNavSatStatusConst {
  STATUS_NO_FIX = -1,
  STATUS_FIX = 0,
  STATUS_SBAS_FIX = 1,
  STATUS_GBAS_FIX = 2,
  SERVICE_GPS = 1,
  SERVICE_GLONASS = 2,
  SERVICE_COMPASS = 4,
  SERVICE_GALILEO = 8,
}

export interface IRosSensorMsgsPointCloud {
  header: IRosStdMsgsHeader;
  points: IRosGeometryMsgsPoint32[];
  channels: IRosSensorMsgsChannelFloat32[];
}

export interface IRosSensorMsgsPointCloud2 {
  header: IRosStdMsgsHeader;
  height: number;
  width: number;
  fields: IRosSensorMsgsPointField[];
  is_bigendian: boolean;
  point_step: number;
  row_step: number;
  data: number[];
  is_dense: boolean;
}

export interface IRosSensorMsgsPointField {
  name: string;
  offset: number;
  datatype: number;
  count: number;
}

export enum IRosSensorMsgsPointFieldConst {
  INT8 = 1,
  UINT8 = 2,
  INT16 = 3,
  UINT16 = 4,
  INT32 = 5,
  UINT32 = 6,
  FLOAT32 = 7,
  FLOAT64 = 8,
}

export interface IRosSensorMsgsRange {
  header: IRosStdMsgsHeader;
  radiation_type: number;
  field_of_view: number;
  min_range: number;
  max_range: number;
  range: number;
}

export enum IRosSensorMsgsRangeConst {
  ULTRASOUND = 0,
  INFRARED = 1,
}

export interface IRosSensorMsgsRegionOfInterest {
  x_offset: number;
  y_offset: number;
  height: number;
  width: number;
  do_rectify: boolean;
}

export interface IRosSensorMsgsRelativeHumidity {
  header: IRosStdMsgsHeader;
  relative_humidity: number;
  variance: number;
}

export interface IRosSensorMsgsSetCameraInfoRequest {
  camera_info: IRosSensorMsgsCameraInfo;
}

export interface IRosSensorMsgsSetCameraInfoResponse {
  success: boolean;
  status_message: string;
}

export interface IRosSensorMsgsTemperature {
  header: IRosStdMsgsHeader;
  temperature: number;
  variance: number;
}

export interface IRosSensorMsgsTimeReference {
  header: IRosStdMsgsHeader;
  time_ref: { sec: number; nanosec: number };
  source: string;
}

export interface IRosShapeMsgsMesh {
  triangles: IRosShapeMsgsMeshTriangle[];
  vertices: IRosGeometryMsgsPoint[];
}

export interface IRosShapeMsgsMeshTriangle {
  vertex_indices: number[];
}

export interface IRosShapeMsgsPlane {
  coef: number[];
}

export interface IRosShapeMsgsSolidPrimitive {
  type: number;
  dimensions: number[];
  polygon: IRosGeometryMsgsPolygon;
}

export enum IRosShapeMsgsSolidPrimitiveConst {
  BOX = 1,
  SPHERE = 2,
  CYLINDER = 3,
  CONE = 4,
  PRISM = 5,
  BOX_X = 0,
  BOX_Y = 1,
  BOX_Z = 2,
  SPHERE_RADIUS = 0,
  CYLINDER_HEIGHT = 0,
  CYLINDER_RADIUS = 1,
  CONE_HEIGHT = 0,
  CONE_RADIUS = 1,
  PRISM_HEIGHT = 0,
}

export interface IRosStatisticsMsgsMetricsMessage {
  measurement_source_name: string;
  metrics_source: string;
  unit: string;
  window_start: { sec: number; nanosec: number };
  window_stop: { sec: number; nanosec: number };
  statistics: IRosStatisticsMsgsStatisticDataPoint[];
}

export interface IRosStatisticsMsgsStatisticDataPoint {
  data_type: number;
  data: number;
}

export interface IRosStdMsgsBool {
  data: boolean;
}

export interface IRosStdMsgsByte {
  data: number;
}

export interface IRosStdMsgsByteMultiArray {
  layout: IRosStdMsgsMultiArrayLayout;
  data: number[];
}

export interface IRosStdMsgsChar {
  data: number;
}

export interface IRosStdMsgsColorRgba {
  r: number;
  g: number;
  b: number;
  a: number;
}

export interface IRosStdMsgsFloat32 {
  data: number;
}

export interface IRosStdMsgsFloat32MultiArray {
  layout: IRosStdMsgsMultiArrayLayout;
  data: number[];
}

export interface IRosStdMsgsFloat64 {
  data: number;
}

export interface IRosStdMsgsFloat64MultiArray {
  layout: IRosStdMsgsMultiArrayLayout;
  data: number[];
}

export interface IRosStdMsgsHeader {
  stamp: { sec: number; nanosec: number };
  frame_id: string;
}

export interface IRosStdMsgsInt16 {
  data: number;
}

export interface IRosStdMsgsInt16MultiArray {
  layout: IRosStdMsgsMultiArrayLayout;
  data: number[];
}

export interface IRosStdMsgsInt32 {
  data: number;
}

export interface IRosStdMsgsInt32MultiArray {
  layout: IRosStdMsgsMultiArrayLayout;
  data: number[];
}

export interface IRosStdMsgsInt64 {
  data: number;
}

export interface IRosStdMsgsInt64MultiArray {
  layout: IRosStdMsgsMultiArrayLayout;
  data: number[];
}

export interface IRosStdMsgsInt8 {
  data: number;
}

export interface IRosStdMsgsInt8MultiArray {
  layout: IRosStdMsgsMultiArrayLayout;
  data: number[];
}

export interface IRosStdMsgsMultiArrayDimension {
  label: string;
  size: number;
  stride: number;
}

export interface IRosStdMsgsMultiArrayLayout {
  dim: IRosStdMsgsMultiArrayDimension[];
  data_offset: number;
}

export interface IRosStdMsgsString {
  data: string;
}

export interface IRosStdMsgsUInt16 {
  data: number;
}

export interface IRosStdMsgsUInt16MultiArray {
  layout: IRosStdMsgsMultiArrayLayout;
  data: number[];
}

export interface IRosStdMsgsUInt32 {
  data: number;
}

export interface IRosStdMsgsUInt32MultiArray {
  layout: IRosStdMsgsMultiArrayLayout;
  data: number[];
}

export interface IRosStdMsgsUInt64 {
  data: number;
}

export interface IRosStdMsgsUInt64MultiArray {
  layout: IRosStdMsgsMultiArrayLayout;
  data: number[];
}

export interface IRosStdMsgsUInt8 {
  data: number;
}

export interface IRosStdMsgsUInt8MultiArray {
  layout: IRosStdMsgsMultiArrayLayout;
  data: number[];
}

export interface IRosStdSrvsSetBoolRequest {
  data: boolean;
}

export interface IRosStdSrvsSetBoolResponse {
  success: boolean;
  message: string;
}

export interface IRosStdSrvsTriggerResponse {
  success: boolean;
  message: string;
}

export interface IRosStereoMsgsDisparityImage {
  header: IRosStdMsgsHeader;
  image: IRosSensorMsgsImage;
  f: number;
  t: number;
  valid_window: IRosSensorMsgsRegionOfInterest;
  min_disparity: number;
  max_disparity: number;
  delta_d: number;
}

export interface IRosTrajectoryMsgsJointTrajectory {
  header: IRosStdMsgsHeader;
  joint_names: string[];
  points: IRosTrajectoryMsgsJointTrajectoryPoint[];
}

export interface IRosTrajectoryMsgsJointTrajectoryPoint {
  positions: number[];
  velocities: number[];
  accelerations: number[];
  effort: number[];
  time_from_start: { sec: number; nanosec: number };
}

export interface IRosTrajectoryMsgsMultiDofJointTrajectory {
  header: IRosStdMsgsHeader;
  joint_names: string[];
  points: IRosTrajectoryMsgsMultiDofJointTrajectoryPoint[];
}

export interface IRosTrajectoryMsgsMultiDofJointTrajectoryPoint {
  transforms: IRosGeometryMsgsTransform[];
  velocities: IRosGeometryMsgsTwist[];
  accelerations: IRosGeometryMsgsTwist[];
  time_from_start: { sec: number; nanosec: number };
}

export interface IRosUniqueIdentifierMsgsUuid {
  uuid: number[];
}

export interface IRosVisualizationMsgsGetInteractiveMarkersResponse {
  sequence_number: number;
  markers: IRosVisualizationMsgsInteractiveMarker[];
}

export interface IRosVisualizationMsgsImageMarker {
  header: IRosStdMsgsHeader;
  ns: string;
  id: number;
  type: number;
  action: number;
  position: IRosGeometryMsgsPoint;
  scale: number;
  outline_color: IRosStdMsgsColorRgba;
  filled: number;
  fill_color: IRosStdMsgsColorRgba;
  lifetime: { sec: number; nanosec: number };
  points: IRosGeometryMsgsPoint[];
  outline_colors: IRosStdMsgsColorRgba[];
}

export enum IRosVisualizationMsgsImageMarkerConst {
  CIRCLE = 0,
  LINE_STRIP = 1,
  LINE_LIST = 2,
  POLYGON = 3,
  POINTS = 4,
  ADD = 0,
  REMOVE = 1,
}

export interface IRosVisualizationMsgsInteractiveMarker {
  header: IRosStdMsgsHeader;
  pose: IRosGeometryMsgsPose;
  name: string;
  description: string;
  scale: number;
  menu_entries: IRosVisualizationMsgsMenuEntry[];
  controls: IRosVisualizationMsgsInteractiveMarkerControl[];
}

export interface IRosVisualizationMsgsInteractiveMarkerControl {
  name: string;
  orientation: IRosGeometryMsgsQuaternion;
  orientation_mode: number;
  interaction_mode: number;
  always_visible: boolean;
  markers: IRosVisualizationMsgsMarker[];
  independent_marker_orientation: boolean;
  description: string;
}

export enum IRosVisualizationMsgsInteractiveMarkerControlConst {
  INHERIT = 0,
  FIXED = 1,
  VIEW_FACING = 2,
  NONE = 0,
  MENU = 1,
  BUTTON = 2,
  MOVE_AXIS = 3,
  MOVE_PLANE = 4,
  ROTATE_AXIS = 5,
  MOVE_ROTATE = 6,
  MOVE_3D = 7,
  ROTATE_3D = 8,
  MOVE_ROTATE_3D = 9,
}

export interface IRosVisualizationMsgsInteractiveMarkerFeedback {
  header: IRosStdMsgsHeader;
  client_id: string;
  marker_name: string;
  control_name: string;
  event_type: number;
  pose: IRosGeometryMsgsPose;
  menu_entry_id: number;
  mouse_point: IRosGeometryMsgsPoint;
  mouse_point_valid: boolean;
}

export enum IRosVisualizationMsgsInteractiveMarkerFeedbackConst {
  KEEP_ALIVE = 0,
  POSE_UPDATE = 1,
  MENU_SELECT = 2,
  BUTTON_CLICK = 3,
  MOUSE_DOWN = 4,
  MOUSE_UP = 5,
}

export interface IRosVisualizationMsgsInteractiveMarkerInit {
  server_id: string;
  seq_num: number;
  markers: IRosVisualizationMsgsInteractiveMarker[];
}

export interface IRosVisualizationMsgsInteractiveMarkerPose {
  header: IRosStdMsgsHeader;
  pose: IRosGeometryMsgsPose;
  name: string;
}

export interface IRosVisualizationMsgsInteractiveMarkerUpdate {
  server_id: string;
  seq_num: number;
  type: number;
  markers: IRosVisualizationMsgsInteractiveMarker[];
  poses: IRosVisualizationMsgsInteractiveMarkerPose[];
  erases: string[];
}

export enum IRosVisualizationMsgsInteractiveMarkerUpdateConst {
  KEEP_ALIVE = 0,
  UPDATE = 1,
}

export interface IRosVisualizationMsgsMarker {
  header: IRosStdMsgsHeader;
  ns: string;
  id: number;
  type: number;
  action: number;
  pose: IRosGeometryMsgsPose;
  scale: IRosGeometryMsgsVector3;
  color: IRosStdMsgsColorRgba;
  lifetime: { sec: number; nanosec: number };
  frame_locked: boolean;
  points: IRosGeometryMsgsPoint[];
  colors: IRosStdMsgsColorRgba[];
  texture_resource: string;
  texture: IRosSensorMsgsCompressedImage;
  uv_coordinates: IRosVisualizationMsgsUvCoordinate[];
  text: string;
  mesh_resource: string;
  mesh_file: IRosVisualizationMsgsMeshFile;
  mesh_use_embedded_materials: boolean;
}

export enum IRosVisualizationMsgsMarkerConst {
  ARROW = 0,
  CUBE = 1,
  SPHERE = 2,
  CYLINDER = 3,
  LINE_STRIP = 4,
  LINE_LIST = 5,
  CUBE_LIST = 6,
  SPHERE_LIST = 7,
  POINTS = 8,
  TEXT_VIEW_FACING = 9,
  MESH_RESOURCE = 10,
  TRIANGLE_LIST = 11,
  ADD = 0,
  MODIFY = 0,
  DELETE = 2,
  DELETEALL = 3,
}

export interface IRosVisualizationMsgsMarkerArray {
  markers: IRosVisualizationMsgsMarker[];
}

export interface IRosVisualizationMsgsMenuEntry {
  id: number;
  parent_id: number;
  title: string;
  command: string;
  command_type: number;
}

export enum IRosVisualizationMsgsMenuEntryConst {
  FEEDBACK = 0,
  ROSRUN = 1,
  ROSLAUNCH = 2,
}

export interface IRosVisualizationMsgsMeshFile {
  filename: string;
  data: number[];
}

export interface IRosVisualizationMsgsUvCoordinate {
  u: number;
  v: number;
}

export interface CameraOperationMessage {
  serials: string[];
}
