import {IRosNovaInterfacesGpsData} from "../../ros/rosTypes";

export interface MapCoordinate {
  lat: number;
  long: number;
}

export interface MapPoint extends MapCoordinate {
  name: string;
  labelNumber: number | null;
  labelName: string | null;
  selected: boolean;
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

export enum Vehicle {
  ROVER = 0,
  DRONE = 1,
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
  showTrackRover: boolean;
  centerOnDrone: boolean;
  trackDrone: boolean;
  showTrackDrone: boolean;
  focusVehicle: Vehicle;
}

export const initialGPSMessage = <IRosNovaInterfacesGpsData>{
  header: {
    stamp: {
      sec: 0,
      nanosec: 0,
    },
    frame_id: "",
  },
  latitude: 0, // (38.4062649,-110.7917894) Location: 1.7 Metres Away from MDRS Hanksville
  longitude: 0,
  altitude: 0,
  heading: 45.0,
};
