import React, { useEffect, useRef, useState } from "react";
import * as maptilersdk from "@maptiler/sdk";
import "@maptiler/sdk/dist/maptiler-sdk.css";
import { useCartographerMarkers } from "../hooks/useCartographerMarkers.tsx";
import { useSelector } from "react-redux";
import { RootState } from "../../../../redux/RootState.ts";
import { useCartographerActions } from "../../../../redux/actions/useCartographerActions.ts";
import { useCartographerTracking } from "../hooks/useCartographerTracking.tsx";
import { getLineGeoJSONSource } from "../utils/geojson.ts";
import { MAP_BOUNDS, MapTile } from "../config.tsx";

export const MapTilerMap = (props: { overlay: React.ReactNode, mapTile: MapTile }) => {
  const { mapTile } = props;
  const mapContainer = useRef<HTMLDivElement>(null);
  const [map, setMap] = useState<maptilersdk.Map>();

  const baseStationIp = useSelector(
    (state: RootState) => state.uiState.baseStationIP
  );


  useCartographerMarkers(map);
  useCartographerTracking(map);

  const { updateMousePosition, handleMapClickEvent } = useCartographerActions();

  useEffect(() => {
    setMap(undefined);
  }, [mapTile]);


  useEffect(() => {
    if ((map || !mapContainer.current)) return; // stops map from intializing more than once

    maptilersdk.config.apiKey = "PLZ ADD MEMES TO PR'S PLEASE"; // This Comment is to ensure that No API Key is Needed
    const newMap = new maptilersdk.Map({
      maptilerLogo: false,
      geolocateControl: false,
      container: mapContainer.current,
      maxBounds: MAP_BOUNDS[mapTile],
      style: {
        version: 8,
        name: "Cartographer",
        sources: {
          tiles: {
            tiles: [
              `http://${baseStationIp}:8080/data/${mapTile}/{z}/{x}/{y}.png`,
            ],
            type: "raster",
            bounds: MAP_BOUNDS[mapTile],
          },
        },
        layers: [
          {
            id: "tiles",
            source: "tiles",
            type: "raster",
          },
        ],
      },
    });

    newMap.on("load", () => {
      newMap.addSource("trace", getLineGeoJSONSource([]));

      newMap.addSource("measureLine", getLineGeoJSONSource([]));

      newMap.addLayer({
        id: "trace",
        type: "line",
        source: "trace",
        paint: {
          "line-color": "red",
          "line-opacity": 0.75,
          "line-width": 3,
        },
      });

      newMap.addLayer({
        id: "measure-points",
        type: "circle",
        source: "measureLine",
        paint: {
          "circle-radius": 5,
          "circle-color": "#000",
        },
        filter: ["in", "$type", "Point"],
      });
      newMap.addLayer({
        id: "measure-lines",
        type: "line",
        source: "measureLine",
        layout: {
          "line-cap": "round",
          "line-join": "round",
        },
        paint: {
          "line-color": "#000",
          "line-width": 2.5,
        },
        filter: ["in", "$type", "LineString"],
      });
    });

    // Add Event Listeners
    newMap.on("mousemove", (event) => {
      updateMousePosition({
        lat: event.lngLat.lat,
        long: event.lngLat.lng,
      });
    });
    newMap.on("click", (event) => {
      handleMapClickEvent({
        lat: event.lngLat.lat,
        long: event.lngLat.lng,
      });
    });

    setMap(newMap);
  }, [
    mapContainer,
    map,
    baseStationIp,
    mapTile,
    updateMousePosition,
    handleMapClickEvent,
  ]);

  return (
    <div className="w-full h-full" ref={mapContainer}>
      <div className="map-wrap"></div>
      <div className="z-10 relative">{props.overlay}</div>
    </div>
  );
};
