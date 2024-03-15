import { 
  IRosCameraMsgsCameras, 
  IRosCoreCmDsFeedback, 
  IRosCoreDriveInfo, 
  IRosCoreKilnData, 
  IRosCoreNirProbeData, 
  IRosCoreTelemetry, 
  IRosGeometryMsgsPose, 
  IRosStdMsgsString 
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
  [RosTopic.DRIVE_INFO]: IRosCoreDriveInfo;
  [RosTopic.DRIVE_TELEMETRY]: IRosCoreTelemetry;

  // Arm Related
  [RosTopic.ARM_TELEMETRY]: IRosCoreCmDsFeedback;
  [RosTopic.RFID_DATA]: IRosStdMsgsString;

  // Cameras Related
  [RosTopic.CAMERAS]: IRosCameraMsgsCameras;

  // Science Related
  [RosTopic.KILN_DATA]: IRosCoreKilnData;
  [RosTopic.NIR_DATA]: IRosCoreNirProbeData;
}
