import React, { useEffect, useRef } from "react";
import * as maptilersdk from "@maptiler/sdk";
import "@maptiler/sdk/dist/maptiler-sdk.css";
import { useCartographerMarkers } from "../hooks/useCartographerMarkers.tsx";
import { useSelector } from "react-redux";
import { RootState } from "../../../../redux/RootState.ts";
import { useCartographerActions } from "../../../../redux/actions/useCartographerActions.ts";
import { useCartographerTracking } from "../hooks/useCartographerTracking.tsx";
import { getLineGeoJSONSource, getSearchRadiusGeoJSONData } from "../utils/geojson.ts";
import { MAP_BOUNDS, MapTile } from "../config.tsx";

export const MapTilerMap = (props: { overlay: React.ReactNode, mapTile: MapTile, enableDroneTracking?: boolean }) => {
  const { mapTile, enableDroneTracking = false } = props;
  const mapContainer = useRef<HTMLDivElement>(null);
  
  const mapRef = useRef<maptilersdk.Map | null>(null);
  
  const { updateMousePosition, handleMapClickEvent } = useCartographerActions();


  useEffect(()=>{
    if (mapRef.current || !mapContainer.current) return;
    const newMap = new maptilersdk.Map({
        maptilerLogo: false,
        geolocateControl: false,
        container: mapContainer.current ?? "ERROR"
    });
    newMap.on("load", ()=>{
      newMap.addSource("roverTrace", getLineGeoJSONSource([]));
      if (enableDroneTracking) {
        newMap.addSource("droneTrace", getLineGeoJSONSource([]));
      }
      newMap.addSource("measureLine", getLineGeoJSONSource([]));
      newMap.addSource("search-radius", {
        type: "geojson",
        data: {
          type: "FeatureCollection",
          features: [],
        },
      });

      newMap.addLayer({
        id: "rover-trace",
        type: "line",
        source: "roverTrace",
        paint: {
          "line-color": "red",
          "line-opacity": 0.75,
          "line-width": 3,
        },
      });

      if (enableDroneTracking) {
        newMap.addLayer({
          id: "drone-trace",
          type: "line",
          source: "droneTrace",
          paint: {
            "line-color": "blue",
            "line-opacity": 0.75,
            "line-width": 3,
          },
        });
      }

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

      newMap.addLayer({
        id: "search-radius-fill",
        type: "fill",
        source: "search-radius",
        paint: {
          "fill-color": "#3b82f6",
          "fill-opacity": 0.2,
        },
      });
      newMap.addLayer({
        id: "search-radius-outline",
        type: "line",
        source: "search-radius",
        paint: {
          "line-color": "#3b82f6",
          "line-width": 2,
        },
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
    });
    mapRef.current = newMap;
  }, [updateMousePosition, handleMapClickEvent, enableDroneTracking]);

  const baseStationIp = useSelector(
    (state: RootState) => state.uiState.baseStationIP
  );

  const points = useSelector(
    (state: RootState) => state.cartographerState.points
  );

  const showSearchZones = useSelector(
    (state: RootState) => state.cartographerState.showSearchZones
  );

  useCartographerMarkers(mapRef, enableDroneTracking);
  useCartographerTracking(mapRef, enableDroneTracking);

  useEffect(()=> {
    if (!mapRef.current) return;
    maptilersdk.config.apiKey = "PLZ ADD MEMES TO PR'S PLEASE"; // This Comment is to ensure that No API Key is Needed
    mapRef.current.setStyle({
        version: 8,
        name: "Cartographer",
        sources: {
          tiles: {
            tiles: [
              `http://${baseStationIp}:8080/services/${mapTile}/tiles/{z}/{x}/{y}.png`,
            ],
            type: "raster",
            bounds: MAP_BOUNDS[mapTile],
          },
        },
        layers: [{
          id: "tiles",
          source: "tiles",
          type: "raster",
        },],
    })
    mapRef.current.setMaxBounds(MAP_BOUNDS[mapTile])
  }, [baseStationIp, mapRef, mapTile]);

  useEffect(() => {
    if (!mapRef.current) return;

    const source = mapRef.current.getSource(
      "search-radius"
    ) as maptilersdk.GeoJSONSource;

    if (!source) return;

    source.setData(getSearchRadiusGeoJSONData(points, showSearchZones));
  }, [points, showSearchZones]);

  return (
    <div className="w-full h-full" ref={mapContainer}>
      <div className="map-wrap"></div>
      <div className="z-10 relative">{props.overlay}</div>
    </div>
  );
};
