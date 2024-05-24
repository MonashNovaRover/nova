import { IRosSensorMsgsNavSatFix } from "../../ros/rosTypes";

export interface MapPoint {
  lat: number;
  long: number;
  name: string;
}

export enum MapInteractionMode {
  PAN,
  SELECT,
}

export interface CartographerState {
  points: MapPoint[];
  mapInteractionMode: MapInteractionMode;
}

export const initialNavSatMessage = <IRosSensorMsgsNavSatFix>{
  header: {
    stamp: {
      sec: 0,
      nanosec: 0,
    },
    frame_id: "",
  },
  status: {
    status: 0,
    service: 0,
  },
  latitude: 38.4062649, // Location: MDRS Hanksville
  longitude: -110.7917894,
  altitude: 0,
  position_covariance: [],
  position_covariance_type: 0,
};
