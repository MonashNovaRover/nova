import { GeoJSONSource, Map } from "@maptiler/sdk";
import { useEffect, useState, RefObject } from "react";
// import { useLocalStorage } from "../../nir-probe/hooks/useLocalStorage";
import { MapCoordinate } from "../../../../redux/models/CartographerState.ts";
import { useBifrost } from "../../../../redux/actions/bifrost/useBifrostAction.ts";
import { useSelector } from "react-redux";
import { RootState } from "../../../../redux/RootState.ts";
import { useDebounce } from "@uidotdev/usehooks";
import { getLineGeoJSONData } from "../utils/geojson.ts";
import { RosTopic } from "../../../../ros/topics/rosTopic.ts";

export const useCartographerTracking = (mapRef: RefObject<Map | null>) => {
  //   const [trace, setTrace] = useLocalStorage<MapCoordinate[]>("roverTrace", []);
  const [trace, setTrace] = useState<MapCoordinate[]>([]);

  const { measure, centerOnRover, trackRover } = useSelector(
    (state: RootState) => state.cartographerState
  );

  const addPoint = (point: MapCoordinate) => {
    setTrace([...trace, point]);
  };

  const roverLocationBifrost = useBifrost({
    topic: RosTopic.ROVER_LOCATION,
  });

  useEffect(() => {
    roverLocationBifrost.syncWithTopic();
  }, [roverLocationBifrost]);

  const roverLocation = useSelector(
    (state: RootState) => state.roverLocationStore
  );

  const deBouncedRoverLocation = useDebounce(roverLocation, 100);

  useEffect(() => {
    if (trackRover) {
      addPoint({
        lat: deBouncedRoverLocation.latitude,
        long: deBouncedRoverLocation.longitude,
      });
    }

    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [deBouncedRoverLocation.latitude, deBouncedRoverLocation.longitude]);

  useEffect(() => {
    if (!mapRef.current || trackRover) return;
    const source = mapRef.current.getSource("trace") as GeoJSONSource;
    if (!source) return;

    source.setData(getLineGeoJSONData([]));
  }, [trackRover, mapRef]);

  useEffect(() => {
    if (!mapRef.current) return;
    const source = mapRef.current.getSource("trace") as GeoJSONSource;
    if (!source) return;

    source.setData(getLineGeoJSONData(trace));
  }, [trace, mapRef]);

  // Measure Line Augmentation
  useEffect(() => {
    if (!mapRef.current) return;
    const source = mapRef.current.getSource("measureLine") as GeoJSONSource;
    if (!source) return;
    if (measure.from && measure.to) {
      source.setData(getLineGeoJSONData([measure.from, measure.to]));
    } else {
      source.setData(getLineGeoJSONData([]));
    }
  }, [measure, mapRef]);

  // Rover Centering
  useEffect(() => {
    if (!mapRef.current) return;
    if (centerOnRover) {
      mapRef.current.setCenter([
        deBouncedRoverLocation.longitude,
        deBouncedRoverLocation.latitude,
      ]);
    }
  }, [centerOnRover, deBouncedRoverLocation, mapRef]);
};
