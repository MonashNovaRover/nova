import React, { useEffect, useRef } from "react";
import * as maptilersdk from "@maptiler/sdk";
import "@maptiler/sdk/dist/maptiler-sdk.css";

export const MapTilerMap = (props: { overlay: React.ReactNode }) => {
  const mapContainer = useRef<HTMLDivElement>(null);
  const map = useRef<maptilersdk.Map>();

  useEffect(() => {
    if (map.current || !mapContainer.current) return; // stops map from intializing more than once

    maptilersdk.config.apiKey = "PLZ ADD MEMES TO PR'S PLEASE"; // This Comment is to ensure that No API Key is Needed
    map.current = new maptilersdk.Map({
      navigationControl: false,
      maptilerLogo: false,
      geolocateControl: false,
      container: mapContainer.current,
      bounds: [-110.809, 38.3917, -110.765, 38.4177],
      fitBoundsOptions: {},
      zoom: 1,
      style: {
        version: 8,
        name: "Cartographer",
        sources: {
          tiles: {
            tiles: ["http://localhost:8080/data/MDRS_Hi_Res/{z}/{x}/{y}.png"],
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
        center: [-110.787, 38.404700000000005, 10],
      },
    });
  }, [mapContainer, map]);

  return (
    <div className="map-wrap">
      <div className="w-full h-[95vh]" ref={mapContainer}>
        <div className="z-10 relative">{props.overlay}</div>
      </div>
    </div>
  );
};
