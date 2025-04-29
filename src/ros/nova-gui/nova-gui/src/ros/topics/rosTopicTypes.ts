import {
  IRosBlcmdInterfacesBlcmdStatusArray,
  IRosCameraMsgsCameras,
  IRosBlcmdInterfacesTelemetry,
  IRosGeometryMsgsPose,
  IRosSensorMsgsRange,
  IRosCmdInterfacesCmDsFeedback,
  IRosDriveInterfacesDriveInfo,
  IRosNovaInterfacesKilnData,
  IRosNovaInterfacesNirProbeData,
  IRosNovaInterfacesMicroscopeServoInfo,
  IRosNovaInterfacesRamanSpectrum,
  IRosStdMsgsString,
  IRosSensorMsgsNavSatFix,
  IRosNovaInterfacesUvVisSpecData,
  IRosNovaInterfacesRamanState,
  IRosNovaInterfacesHydraprobeData,
  IRosSensorMsgsCompressedImage,
  IRosNovaInterfacesBmeSensor,
  IRosSensorMsgsBatteryState, IRosNovaInterfacesActiveNodeStatus,
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

  // Cameras Related
  [RosTopic.CAMERAS]: IRosCameraMsgsCameras;

  // Errors Related
  [RosTopic.BLCMD_ERRORS]: IRosBlcmdInterfacesBlcmdStatusArray;

  // Science Related
  [RosTopic.TOF]: IRosSensorMsgsRange;
  [RosTopic.KILN_DATA]: IRosNovaInterfacesKilnData;
  [RosTopic.NIR_DATA]: IRosNovaInterfacesNirProbeData;
  [RosTopic.MICROSCOPE_SERVO]: IRosNovaInterfacesMicroscopeServoInfo;
  [RosTopic.RAMAN_SPEC_MSG]: IRosNovaInterfacesRamanSpectrum;
  [RosTopic.RAMAN_MECH_MSG]: IRosNovaInterfacesRamanState;
  [RosTopic.UV_VIS_SPEC]: IRosNovaInterfacesUvVisSpecData;
  [RosTopic.HYDRAPROBE_DATA]: IRosNovaInterfacesHydraprobeData;
  [RosTopic.THETA_360_CAM_IMAGE]: IRosSensorMsgsCompressedImage;
  [RosTopic.BME_SENSOR]: IRosNovaInterfacesBmeSensor;

  // Cameras Related
  [RosTopic.CAMERAS]: IRosCameraMsgsCameras;

  // Maps Related
  [RosTopic.ROVER_LOCATION]: IRosSensorMsgsNavSatFix;
  [RosTopic.BASE_LOCATION]: IRosSensorMsgsNavSatFix;

  [RosTopic.BATTERY_STATE]: IRosSensorMsgsBatteryState;
  [RosTopic.ACTIVATED_NODES]: IRosNovaInterfacesActiveNodeStatus;
}
