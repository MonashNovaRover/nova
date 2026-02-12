import {
  IRosBlcmdInterfacesBlcmdStatusArray,
  IRosCameraMsgsCameras,
  IRosBlcmdInterfacesTelemetry,
  IRosGeometryMsgsPose,
  IRosSensorMsgsRange,
  IRosCmdInterfacesCmDsFeedback,
  IRosDriveInterfacesDriveInfo,
  IRosScienceInterfacesKilnData,
  IRosScienceInterfacesNirProbeData,
  IRosScienceInterfacesMicroscopeServoInfo,
  IRosScienceInterfacesRamanSpectrum,
  IRosStdMsgsString,
  IRosStdMsgsBool,
  IRosScienceInterfacesUvVisSpecData,
  IRosScienceInterfacesRamanState,
  IRosScienceInterfacesHydraprobeData,
  IRosSensorMsgsCompressedImage,
  IRosScienceInterfacesBmeSensor,
  IRosArmInterfacesKeyboardPoints,
  IRosSensorMsgsBatteryState,
  IRosNovaInterfacesActiveNodeStatus,
  IRosNovaInterfacesStatus,
  IRosSensorMsgsNavSatFix,
  IRosArmInterfacesSequencerFeedback,
  IRosNovaInterfacesRadioStatus,
} from "../rosTypes";
import { RosTopic } from "./rosTopic";

/**
 * This Interface exists to link Individual topics to The Messages.
 * These Messages are defined as Interfaces on `rosTypes.ts`. This is not to be confused with
 * `rosMessages.ts`
 *
 * An Example is Given below to link the Topic RosTopics.DEMO_TOPIC to IRosDemoMessage
 */
export interface RosTopicInterfaces {
  [RosTopic.NULL_TOPIC]: undefined;
  [RosTopic.POSE]: IRosGeometryMsgsPose;

  // Drive Related
  [RosTopic.DRIVE_INFO]: IRosDriveInterfacesDriveInfo;
  [RosTopic.DRIVE_TELEMETRY]: IRosBlcmdInterfacesTelemetry;

  // Arm Related
  [RosTopic.ARM_TELEMETRY]: IRosCmdInterfacesCmDsFeedback;
  [RosTopic.RFID_DATA]: IRosStdMsgsString;
  [RosTopic.KEYBOARD_DATA]: IRosArmInterfacesKeyboardPoints;
  [RosTopic.TYPE_SEQUENCE]: IRosArmInterfacesSequencerFeedback;

  // Cameras Related
  [RosTopic.CAMERAS]: IRosCameraMsgsCameras;

  // Errors Related
  [RosTopic.BLCMD_ERRORS]: IRosBlcmdInterfacesBlcmdStatusArray;

  // Science Related
  [RosTopic.TOF]: IRosSensorMsgsRange;
  [RosTopic.KILN_DATA]: IRosScienceInterfacesKilnData;
  [RosTopic.NIR_DATA]: IRosScienceInterfacesNirProbeData;
  [RosTopic.MICROSCOPE_SERVO]: IRosScienceInterfacesMicroscopeServoInfo;
  [RosTopic.RAMAN_SPEC_MSG]: IRosScienceInterfacesRamanSpectrum;
  [RosTopic.RAMAN_MECH_MSG]: IRosScienceInterfacesRamanState;
  [RosTopic.UV_VIS_SPEC]: IRosScienceInterfacesUvVisSpecData;
  [RosTopic.HYDRAPROBE_DATA]: IRosScienceInterfacesHydraprobeData;
  [RosTopic.THETA_360_CAM_IMAGE]: IRosSensorMsgsCompressedImage;
  [RosTopic.BME_SENSOR]: IRosScienceInterfacesBmeSensor;
  [RosTopic.AUGER1_DEPTH_SENSOR]: IRosStdMsgsBool;
  [RosTopic.AUGER2_DEPTH_SENSOR]: IRosStdMsgsBool;

  // Cameras Related
  [RosTopic.CAMERAS]: IRosCameraMsgsCameras;

  // Maps Related
  [RosTopic.ROVER_LOCATION]: IRosSensorMsgsNavSatFix;
  [RosTopic.BASE_LOCATION]: IRosSensorMsgsNavSatFix;
  [RosTopic.AUTO_STATUS]: IRosNovaInterfacesStatus;

  // Other
  [RosTopic.BATTERY_STATE]: IRosSensorMsgsBatteryState;
  [RosTopic.ACTIVATED_NODES]: IRosNovaInterfacesActiveNodeStatus;
  [RosTopic.RADIO_STATUS]: IRosNovaInterfacesRadioStatus;
}
