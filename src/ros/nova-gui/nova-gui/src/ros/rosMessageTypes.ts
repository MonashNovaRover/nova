/**
 * This file contains all ROS Messages represented as Interfaces
 * The ROS TS Generator is responsible for converting ROS msgs into Interfaces
 * Fell Free to Add Interfaces that Represent ROS Messages
 *
 * The Interface IRosDemoMessage is Illustrated below
 * Consider `demo_msg.msg` consisting of
 *
 * string info
 * int32 info_id
 *
 */
export interface IRosDemoMessage {
  info: string;
  info_id: number;
}
