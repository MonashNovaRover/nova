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
