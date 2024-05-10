import { useState } from "react";

export interface MapPoint {
  lat: number;
  long: number;
  name: string;
}

export enum MapInteractionMode {
  PAN,
  SELECT,
}

export const useMapManager = () => {
  const [points, setPoints] = useState<MapPoint[]>([
    { lat: -37.9106996, long: 145.1359248, name: "Point 1" },
  ]);
  const [mapInteractionMode, setMapInteractionMode] = useState(
    MapInteractionMode.PAN
  );

  const addPoint = (point: MapPoint) => setPoints([...points, point]);

  const removePoint = (lat: number, long: number) =>
    setPoints(points.filter((pt) => pt.lat !== lat && pt.long !== long));

  return {
    addPoint,
    removePoint,
    points,
    mapInteractionMode,
    setMapInteractionMode,
  };
};
