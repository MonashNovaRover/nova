import React, { useEffect, useRef, useState } from "react";
import * as maptilersdk from "@maptiler/sdk";
import "@maptiler/sdk/dist/maptiler-sdk.css";
import { useCartographerActions } from "../../../redux/actions/useCartographerActions";
import { useCartographerMarkers } from "../hooks/useCartographerMarkers";

export const MapTilerMap = (props: { overlay: React.ReactNode }) => {
  const mapContainer = useRef<HTMLDivElement>(null);
  const [map, setMap] = useState<maptilersdk.Map>();

  useCartographerMarkers(map);
  useEffect(() => {
    if (map || !mapContainer.current) return; // stops map from intializing more than once

    maptilersdk.config.apiKey = "PLZ ADD MEMES TO PR'S PLEASE"; // This Comment is to ensure that No API Key is Needed
    const newMap = new maptilersdk.Map({
      navigationControl: false,
      maptilerLogo: false,
      geolocateControl: false,
      container: mapContainer.current,
      maxBounds: [-110.809, 38.3917, -110.765, 38.4177],
      style: {
        version: 8,
        name: "Cartographer",
        sources: {
          tiles: {
            tiles: ["http://localhost:8080/data/MDRS_Hi_Res/{z}/{x}/{y}.png"], // Todo: Replace with Rover IP
            type: "raster",
            bounds: [-110.809, 38.3917, -110.765, 38.4177],
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
    setMap(newMap);
  }, [mapContainer, map]);

  return (
    <div className="map-wrap">
      <div className="w-full h-[95vh]" ref={mapContainer}>
        <div className="z-10 relative">{props.overlay}</div>
      </div>
    </div>
  );
};
