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

export interface IRosPose {
  position: {
    x: number;
    y: number;
    z: number;
  };
  orientation: {
    x: number;
    y: number;
    z: number;
    w: number;
  };
}

// Enum used for drive_mode in IRosDriveInfo
export enum DriveMode {
  PIVOT = 1,
  STRAFE = 2,
  TANK = 3
}

export interface IRosDriveInfo {
  multiplier: number,
  locked: boolean,
  autonomous_mode: boolean,
  connected: boolean,
  drive_mode: DriveMode,
  handbrake: boolean
}

export interface IRosSingleTelemetry {
  bus: string;
  id: number;
  rotor_velocity: number;
  q_current: number;
  rotor_interval: number;
  d_current: number;
  resolver_position: number;
  resolver_velocity: number;
  power: number;
  voltage: number;
  temperature: number;
}

export interface IRosTelemetry {
  wheels: IRosSingleTelemetry[];
  pivots: IRosSingleTelemetry[];
}