import { GeoJSONSource, Map } from "@maptiler/sdk";
import { useEffect, useState } from "react";
import { useLocalStorage } from "../../nir-probe/hooks/useLocalStorage";
import { MapCoordinate } from "../../../redux/models/CartographerState";
import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction";
import { RosTopic } from "../../../ros/topics/rosTopic";
import { useSelector } from "react-redux";
import { RootState } from "../../../redux/RootState";
import { useDebounce } from "@uidotdev/usehooks";

export const useCartographerTracking = (map?: Map) => {
  //   const [trace, setTrace] = useLocalStorage<MapCoordinate[]>("roverTrace", []);
  const [trace, setTrace] = useState<MapCoordinate[]>([]);

  const addPoint = (point: MapCoordinate) => {
    setTrace([...trace, point]);
  };

  const roverLocationBifrost = useBifrost({
    topic: RosTopic.AUTO_ROVER_LOCATION,
  });

  useEffect(() => {
    roverLocationBifrost.syncWithTopic();
  }, [roverLocationBifrost]);

  const roverLocation = useSelector(
    (state: RootState) => state.autoRoverLocationStore
  );

  const deBouncedRoverLocation = useDebounce(roverLocation, 100);

  useEffect(() => {
    addPoint({
      lat: deBouncedRoverLocation.latitude,
      long: deBouncedRoverLocation.longitude,
    });
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [deBouncedRoverLocation.latitude, deBouncedRoverLocation.longitude]);

  useEffect(() => {
    if (!map) return;
    const source = map.getSource("trace") as GeoJSONSource;
    if (!source) return;

    const newGeoJSONData: GeoJSON.GeoJSON = {
      type: "FeatureCollection",
      features: [
        {
          type: "Feature",
          properties: {},
          geometry: {
            coordinates: trace.map((point) => [point.long, point.lat]),
            type: "LineString",
          },
        },
      ],
    };

    source.setData(newGeoJSONData);
  }, [trace, map]);
};
