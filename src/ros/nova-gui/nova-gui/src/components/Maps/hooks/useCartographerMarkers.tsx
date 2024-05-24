import { Map, Marker } from "@maptiler/sdk";
import { RosTopic } from "../../../ros/topics/rosTopic";
import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction";
import { useSelector } from "react-redux";
import { RootState } from "../../../redux/RootState";
import roverIcon from "../../../assets/rover-top-down-dark.png";
import { useEffect, useState } from "react";
export const useCartographerMarkers = (map?: Map) => {
  const [roverMarker, setRoverMarker] = useState<Marker>();
  const [baseMarker, setBaseMarker] = useState<Marker>();

  const roverLocationBifrost = useBifrost({
    topic: RosTopic.AUTO_ROVER_LOCATION,
  });

  const baseLocationBifrost = useBifrost({
    topic: RosTopic.BASE_LOCATION,
  });

  useEffect(() => {
    roverLocationBifrost.syncWithTopic();
  }, [roverLocationBifrost]);

  useEffect(() => {
    baseLocationBifrost.syncWithTopic();
  }, [baseLocationBifrost]);

  const roverLocation = useSelector((state: RootState) => state.autoRoverLocationStore);

  const baseLocationStore = useSelector((state: RootState) => state.baseLocationStore);

  useEffect(() => {
    if (!map) return;

    if (!roverMarker) {
      const marker = new Marker({
        element: createRoverIcon(),
      });
      marker.setLngLat([roverLocation.longitude, roverLocation.latitude]);
      marker.addTo(map);
      setRoverMarker(marker);
    } else {
      roverMarker.setLngLat([roverLocation.longitude, roverLocation.latitude]);
    }
  }, [map, roverLocation.latitude, roverLocation.longitude, roverMarker]);

  useEffect(() => {
    if (!map) return;

    if (!baseMarker) {
      const marker = new Marker({
        element: createRoverIcon(),
      });
      marker.setLngLat([baseLocationStore.longitude, baseLocationStore.latitude]);
      marker.addTo(map);
      setBaseMarker(marker);
    } else {
      baseMarker.setLngLat([baseLocationStore.longitude, baseLocationStore.latitude]);
    }
  }, [baseLocationStore.latitude, baseLocationStore.longitude, baseMarker, map]);
};

const createRoverIcon = () => {
  const imgElement = document.createElement("img");
  imgElement.src = roverIcon;
  imgElement.className = "w-10";
  return imgElement;
};
