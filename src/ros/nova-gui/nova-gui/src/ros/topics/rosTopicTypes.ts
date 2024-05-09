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
  IRosStdMsgsString,
  IRosNovaInterfacesRamanSpectrum,
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

  // Cameras Related
  [RosTopic.CAMERAS]: IRosCameraMsgsCameras;

}
