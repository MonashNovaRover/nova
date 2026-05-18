import { SourceSpecification } from "@maptiler/sdk";
import { MapCoordinate, MapPoint } from "../../../../redux/models/CartographerState.ts";
import { getDistance as computeDistance } from "geolib";
import { Feature, FeatureCollection, Polygon } from "geojson";

const createCircleCoordinates = (
  centerLat: number,
  centerLon: number,
  radiusMeters: number,
  points = 64
): number[][] => {
  const coordinates: number[][] = [];

  const earthRadius = 6378137; // meters

  for (let i = 0; i <= points; i++) {
    const angle = (i / points) * 2 * Math.PI;

    const dx = radiusMeters * Math.cos(angle);
    const dy = radiusMeters * Math.sin(angle);

    const dLat = dy / earthRadius;
    const dLon =
      dx / (earthRadius * Math.cos((centerLat * Math.PI) / 180));

    const lat = centerLat + (dLat * 180) / Math.PI;
    const lon = centerLon + (dLon * 180) / Math.PI;

    coordinates.push([lon, lat]);
  }

  return coordinates;
};

export const getLineGeoJSONData = (line: MapPoint[]): GeoJSON.GeoJSON => {
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

export const getSearchRadiusGeoJSONData = (
  points: MapCoordinate[]
): FeatureCollection<Polygon> => {
  return {
    type: "FeatureCollection",
    features: points
      .filter(
        (point) =>
          point.showSearchZone &&
          point.searchRadius &&
          point.searchRadius > 0
      )
      .map((point) => ({
        type: "Feature",
        properties: {
          name: point.name,
          radius: point.searchRadius,
        },
        geometry: {
          type: "Polygon",
          coordinates: [
            createCircleCoordinates(
              point.lat,
              point.long,
              point.searchRadius
            ),
          ],
        },
      })),
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
