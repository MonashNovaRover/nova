import { GeoJSONSource, Map } from "@maptiler/sdk";
import { useEffect, useState } from "react";
// import { useLocalStorage } from "../../nir-probe/hooks/useLocalStorage";
import { MapCoordinate } from "../../../../redux/models/CartographerState.ts";
import { useBifrost } from "../../../../redux/actions/bifrost/useBifrostAction.ts";
import { useSelector } from "react-redux";
import { RootState } from "../../../../redux/RootState.ts";
import { useDebounce } from "@uidotdev/usehooks";
import { getLineGeoJSONData } from "../utils/geojson.ts";
import { RosTopic } from "../../../../ros/topics/rosTopic.ts";

export const useCartographerTracking = (map?: Map) => {
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
    if (!map || trackRover) return;
    const source = map.getSource("trace") as GeoJSONSource;
    if (!source) return;

    source.setData(getLineGeoJSONData([]));
  }, [trackRover, map]);

  useEffect(() => {
    if (!map) return;
    const source = map.getSource("trace") as GeoJSONSource;
    if (!source) return;

    source.setData(getLineGeoJSONData(trace));
  }, [trace, map]);

  // Measure Line Augmentation
  useEffect(() => {
    if (!map) return;
    const source = map.getSource("measureLine") as GeoJSONSource;
    if (!source) return;
    if (measure.from && measure.to) {
      source.setData(getLineGeoJSONData([measure.from, measure.to]));
    } else {
      source.setData(getLineGeoJSONData([]));
    }
  }, [measure, map]);

  // Rover Centering
  useEffect(() => {
    if (!map) return;
    if (centerOnRover) {
      map.setCenter([
        deBouncedRoverLocation.longitude,
        deBouncedRoverLocation.latitude,
      ]);
    }
  }, [centerOnRover, deBouncedRoverLocation, map]);
};
