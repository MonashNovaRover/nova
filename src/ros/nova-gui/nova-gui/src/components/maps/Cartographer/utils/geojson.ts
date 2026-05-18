import { SourceSpecification } from "@maptiler/sdk";
import { MapCoordinate } from "../../../../redux/models/CartographerState.ts";
import { getDistance as computeDistance } from "geolib";

export const getLineGeoJSONData = (line: MapCoordinate[]): GeoJSON.GeoJSON => {
  return {
    type: "FeatureCollection",
    features: [
      {
        type: "Feature",
        properties: {},
        geometry: {
          coordinates: line.map((point) => [point.long, point.lat]),
          type: "LineString",
        },
      },
    ],
  };
};

export const getLineGeoJSONSource = (
  line: MapCoordinate[]
): SourceSpecification => {
  return {
    type: "geojson",
    data: getLineGeoJSONData(line),
  };
};

export const getDistance = (from: MapCoordinate, to: MapCoordinate) => {
  return computeDistance(
    { lat: from.lat, lon: from.long },
    { lat: to.lat, lon: to.long }
  );
};
