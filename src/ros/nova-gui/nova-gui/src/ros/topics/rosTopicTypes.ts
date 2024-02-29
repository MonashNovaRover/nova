import { RosTopic } from "./rosTopic";
import { IRosCoreTelemetry, IRosGeometryMsgsPose } from "../rosTypes";
import { IRosCoreDriveInfo } from "../rosTypes";
import { IRosCoreCmDsFeedback } from "../rosTypes";

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
  [RosTopic.DRIVE_INFO]: IRosCoreDriveInfo;
  [RosTopic.TELEMETRY]: IRosCoreTelemetry;
  // [RosTopic.ARM]: CMDsFeedback;
  [RosTopic.ARM]:IRosCoreCmDsFeedback
}
