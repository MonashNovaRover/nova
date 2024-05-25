// Config for Cartographer

import { RosTopic } from "../../ros/topics/rosTopic";

// This is the Name that comes with the MBTiles File Itself
export const MAP_NAME = "MDRS";

/*
 * Topic on Which the Rover Location is Broadcasted
 *
 * On the Autonomous Task, this will be RosTopic.AUTO_ROVER_LOCATION
 * On Other Tasks like ED , this will be RosTopic.ROVER_LOCATION
 * Refer to RosTopics.ts for specifics
 *
 */
export const ROVER_LOCATION_TOPIC:
  | RosTopic.AUTO_ROVER_LOCATION
  | RosTopic.ROVER_LOCATION = RosTopic.AUTO_ROVER_LOCATION;
