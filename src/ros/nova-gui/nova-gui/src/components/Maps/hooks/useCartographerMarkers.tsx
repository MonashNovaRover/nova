import { Map, Marker } from "@maptiler/sdk";
import { RosTopic } from "../../../ros/topics/rosTopic";
import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction";
import { useSelector } from "react-redux";
import { RootState } from "../../../redux/RootState";
import roverIcon from "../../../assets/rover-top-down-dark.png";
import novaLogo from "../../../assets/nova-logo.png";

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

    if (!baseMarker) {
      const marker = new Marker({
        element: createBaseIcon(),
      });
      marker.setLngLat([baseLocationStore.longitude, baseLocationStore.latitude]);
      marker.addTo(map);
      setBaseMarker(marker);
    } else {
      baseMarker.setLngLat([baseLocationStore.longitude, baseLocationStore.latitude]);
    }
  }, [baseLocationStore.latitude, baseLocationStore.longitude, baseMarker, map]);

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
};

const createRoverIcon = () => {
  const imgElement = document.createElement("img");
  imgElement.src = roverIcon;
  imgElement.className = "w-14";
  return imgElement;
};

const createBaseIcon = () => {
  const imgElement = document.createElement("img");
  imgElement.src = novaLogo;
  imgElement.className = "w-20 bg-black p-3 rounded-md";
  return imgElement;
};
