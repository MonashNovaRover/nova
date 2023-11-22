import { RosTopics } from "../../ros/topics";

export enum BifrostConnectionStatus {
  DISCONNECTED = "Disconnected",
  CONNECTING = "Connecting",
  CONNECTED = "Connected",
}

export interface BifrostStatus {
  connectionStatus: BifrostConnectionStatus;
  subsribedTopics: RosTopics[];
}

export const initalBifrostState: BifrostStatus = {
  connectionStatus: BifrostConnectionStatus.DISCONNECTED,
  subsribedTopics: [],
};
