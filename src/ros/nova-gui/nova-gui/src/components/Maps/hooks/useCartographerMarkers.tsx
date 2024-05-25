import { Map, Marker, Popup } from "@maptiler/sdk";
import { RosTopic } from "../../../ros/topics/rosTopic";
import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction";
import { useSelector } from "react-redux";
import { RootState } from "../../../redux/RootState";
import roverIcon from "../../../assets/rover-top-down-dark.png";
import novaLogo from "../../../assets/nova-logo.png";

import { useEffect, useState } from "react";
import { MapPoint } from "../../../redux/models/CartographerState";
export const useCartographerMarkers = (map?: Map) => {
  const [roverMarker, setRoverMarker] = useState<Marker>();
  const [baseMarker, setBaseMarker] = useState<Marker>();

  const [pointMarkers, setPointMarkers] = useState<Marker[]>([]);

  const { points } = useSelector((state: RootState) => state.cartographerState);

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

  const roverLocation = useSelector(
    (state: RootState) => state.autoRoverLocationStore
  );

  const baseLocationStore = useSelector(
    (state: RootState) => state.baseLocationStore
  );

  useEffect(() => {
    if (!map) return;

    if (!baseMarker) {
      const marker = new Marker({
        element: createBaseIcon(),
      });
      marker.setLngLat([
        baseLocationStore.longitude,
        baseLocationStore.latitude,
      ]);
      marker.addTo(map);
      setBaseMarker(marker);
    } else {
      baseMarker.setLngLat([
        baseLocationStore.longitude,
        baseLocationStore.latitude,
      ]);
    }
  }, [
    baseLocationStore.latitude,
    baseLocationStore.longitude,
    baseMarker,
    map,
  ]);

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

    const newPoints = points.filter((point) =>
      pointMarkers.every((marker) => {
        const lngLat = marker.getLngLat();
        return lngLat.lat !== point.lat && lngLat.lng !== point.long;
      })
    );

    const newMarkers = newPoints.map((point) => {
      const marker = new Marker();
      marker
        .setLngLat([point.long, point.lat])
        .setPopup(
          new Popup({
            closeOnClick: false,
            className: "text-black dark",
          }).setText(point.name)
        )
        .addTo(map)
        .togglePopup();
      return marker;
    });

    setPointMarkers([...pointMarkers, ...newMarkers]);
  }, [
    map,
    pointMarkers,
    points,
    roverLocation.latitude,
    roverLocation.longitude,
  ]);
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
