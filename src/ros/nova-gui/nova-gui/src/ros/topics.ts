import { IRadioStatus, ITalker } from "./types";

export enum RosTopics {
  RADIO_STATUS = "radio_status",
  TALKER = "talker",
}

export interface RosTopicTypes {
  [RosTopics.TALKER]: ITalker;
  [RosTopics.RADIO_STATUS]: IRadioStatus;
}
