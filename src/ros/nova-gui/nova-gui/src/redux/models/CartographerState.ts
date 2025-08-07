import {IRosSensorMsgsNavSatFix} from "../../ros/rosTypes";

export interface MapCoordinate {
  lat: number;
  long: number;
}

export interface MapPoint extends MapCoordinate {
  name: string;
  labelNumber: number | null;
  labelName: string | null;
}

export enum MapInteractionMode {
  PAN = "PAN",
  SELECT = "SELECT",
  MEASURE = "MEASURE",
}

export enum GoalType {
  GNSS = 0,
  AR_TAG = 1,
  OBJECT = 2,
}

export interface CartographerState {
  points: MapPoint[];
  mapInteractionMode: MapInteractionMode;
  mousePosition?: MapCoordinate;
  newMarkerModal: {
    open: boolean;
    coordinate?: MapCoordinate;
  };
  measure: {
    from?: MapCoordinate;
    to?: MapCoordinate;
    measuring: boolean;
  };
  centerOnRover: boolean;
  trackRover: boolean;
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
  latitude: 38.4062649, // Location: 1.7 Metres Away from MDRS Hanksville
  longitude: -110.7917894,
  altitude: 0,
  position_covariance: [],
  position_covariance_type: 0,
};
