import { RosTopics } from "./rosTopics";
import { IRosGeometryMsgsPose, IRosDriveInfo, IRosTelemetry } from "./rosMessageTypes";

/**
 * This Interface exists to link Individual topics to The Messages.
 * These Messages are defined as Interfaces on `rosTypes.ts`. This is not to be confused with
 * `rosMessages.ts`
 *
 * An Example is Given below to link the Topic RosTopics.DEMO_TOPIC to IRosDemoMessage
 */
export interface RosTopicInterfaces {
  [RosTopics.NULL_TOPIC]: undefined;
  [RosTopics.POSE]: IRosGeometryMsgsPose;
  [RosTopics.DRIVE_INFO]: IRosDriveInfo;
  [RosTopics.TELEMETRTY]: IRosTelemetry;
}
