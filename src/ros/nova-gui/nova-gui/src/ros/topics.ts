import { IRadioStatus, ITalker } from "./types";

export enum RosTopics {
  RADIO_STATUS = "radio_status",
  TALKER = "talker",
}

export const rosTopicMessages = {
  [RosTopics.RADIO_STATUS]: "nova_msgs/radio",
  [RosTopics.TALKER]: "nova_msgs/talker",
};

export interface RosTopicInterfaces {
  [RosTopics.TALKER]: ITalker;
  [RosTopics.RADIO_STATUS]: IRadioStatus;
}
