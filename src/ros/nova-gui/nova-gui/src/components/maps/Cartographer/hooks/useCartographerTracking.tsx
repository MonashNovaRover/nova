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

export const useCartographerTracking = (mapRef: RefObject<Map | null>, enableDroneTracking: boolean = false) => {
  //   const [trace, setTrace] = useLocalStorage<MapCoordinate[]>("roverTrace", []);
  const [roverTrace, setRoverTrace] = useState<MapCoordinate[]>([]);
  const [droneTrace, setDroneTrace] = useState<MapCoordinate[]>([]);

  const { measure, centerOnRover, trackRover, showTrackRover, centerOnDrone, trackDrone, showTrackDrone } = useSelector(
    (state: RootState) => state.cartographerState
  );

  const addRoverPoint = (point: MapCoordinate) => {
    setRoverTrace([...roverTrace, point]);
  };

  const addDronePoint = (point: MapCoordinate) => {
    setDroneTrace([...droneTrace, point]);
  };

  const roverLocationBifrost = useBifrost({
    topic: RosTopic.ROVER_LOCATION,
  });

  const droneLocationBifrost = useBifrost({
    topic: RosTopic.DRONE_LOCATION,
  });

  useEffect(() => {
    roverLocationBifrost.syncWithTopic();
  }, [roverLocationBifrost]);

  useEffect(() => {
    if (enableDroneTracking) {
      droneLocationBifrost.syncWithTopic();
    }
  }, [droneLocationBifrost, enableDroneTracking]);

  const roverLocation = useSelector(
    (state: RootState) => state.roverLocationStore
  );

  const droneLocation = useSelector(
    (state: RootState) => state.droneLocationStore
  );

  const deBouncedRoverLocation = useDebounce(roverLocation, 100);

  const deBouncedDroneLocation = useDebounce(droneLocation, 100);

  useEffect(() => {
    if (trackRover && (deBouncedRoverLocation.latitude !== 0) && (deBouncedRoverLocation.longitude !== 0)) {
      addRoverPoint({
        lat: deBouncedRoverLocation.latitude,
        long: deBouncedRoverLocation.longitude,
      });
    }

    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [deBouncedRoverLocation.latitude, deBouncedRoverLocation.longitude]);

  useEffect(() => {
    if (!trackRover) setRoverTrace([]);
  }, [trackRover]);

  useEffect(() => {
    if (enableDroneTracking && trackDrone && (deBouncedRoverLocation.latitude !== 0) && (deBouncedRoverLocation.longitude !== 0)) {
      addDronePoint({
        lat: deBouncedDroneLocation.latitude,
        long: deBouncedDroneLocation.longitude,
      });
    }

    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [deBouncedDroneLocation.latitude, deBouncedDroneLocation.longitude, enableDroneTracking]);

  useEffect(() => {
    if (!trackDrone) setDroneTrace([]);
  }, [trackDrone]);

  useEffect(() => {
    if (!mapRef.current || trackRover) return;
    const source = mapRef.current.getSource("roverTrace") as GeoJSONSource;
    if (!source) return;

    source.setData(getLineGeoJSONData([]));
  }, [trackRover, mapRef]);

  useEffect(() => {
    if (!enableDroneTracking || !mapRef.current || trackDrone) return;
    const source = mapRef.current.getSource("droneTrace") as GeoJSONSource;
    if (!source) return;

    source.setData(getLineGeoJSONData([]));
  }, [trackDrone, mapRef, enableDroneTracking]);

  useEffect(() => {
    if (!mapRef.current) return;
    const source = mapRef.current.getSource("roverTrace") as GeoJSONSource;
    if (!source) return;

    source.setData(getLineGeoJSONData(showTrackRover ? roverTrace : []));
  }, [showTrackRover, roverTrace, mapRef]);

  useEffect(() => {
    if (!enableDroneTracking || !mapRef.current) return;
    const source = mapRef.current.getSource("droneTrace") as GeoJSONSource;
    if (!source) return;

    source.setData(getLineGeoJSONData(showTrackDrone ? droneTrace : []));
  }, [showTrackDrone, droneTrace, mapRef, enableDroneTracking]);

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

  // Drone Centering
  useEffect(() => {
    if (!enableDroneTracking || !mapRef.current) return;
    if (centerOnDrone) {
      mapRef.current.setCenter([
        deBouncedDroneLocation.longitude,
        deBouncedDroneLocation.latitude,
      ]);
    }
  }, [centerOnDrone, deBouncedDroneLocation, mapRef, enableDroneTracking]);
};
