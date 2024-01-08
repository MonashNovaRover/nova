import { RosTopics } from "../../../ros/rosTopics";

export enum BifrostConnectionStatus {
  DISCONNECTED = "Disconnected",
  CONNECTING = "Connecting",
  CONNECTED = "Connected",
}

export interface BifrostStatus {
  connectionStatus: BifrostConnectionStatus;
  subscribedTopics: RosTopics[];
}

export const initalBifrostState: BifrostStatus = {
  connectionStatus: BifrostConnectionStatus.DISCONNECTED,
  subscribedTopics: [],
};
