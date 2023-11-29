/**
 * This file contains all ROS Messages represented as Interfaces
 * The ROS TS Generator is responsible for converting ROS msgs into Interfaces
 * Fell Free to Add Interfaces that Represent ROS Messages
 *
 * The Interface IRosDemoMessage is Illustrated below
 * Consider `demo_msg.msg` consisting of
 *
 * string info
 *
 */
export interface IRosDemoMessage {
  data: string;
}

export interface IRosBLCMDStatusMessage{
  gate_fault:boolean;
  stall_fault:boolean;
  resolver_fault: boolean;
  overspeed_fault: boolean;
  id: number;
}