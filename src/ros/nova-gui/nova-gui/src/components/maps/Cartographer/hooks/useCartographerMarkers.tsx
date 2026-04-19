import { Map, Marker, Popup } from "@maptiler/sdk";
import { RosTopic } from "../../../../ros/topics/rosTopic.ts";
import { useBifrost } from "../../../../redux/actions/bifrost/useBifrostAction.ts";
import { useSelector } from "react-redux";
import { RootState } from "../../../../redux/RootState.ts";
import roverIcon from "../../../../assets/rover-top-down-dark.png";
import novaLogo from "../../../../assets/nova-logo.png";
import { useEffect, useState, RefObject } from "react";
import { MapInteractionMode } from "../../../../redux/models/CartographerState.ts";

export const useCartographerMarkers = (mapRef: RefObject<Map | null>) => {
  const [roverMarker, setRoverMarker] = useState<Marker>();
  const [baseMarker, setBaseMarker] = useState<Marker>();


  const [pointMarkers, setPointMarkers] = useState<Marker[]>([]);

  const { points, mapInteractionMode } = useSelector(
    (state: RootState) => state.cartographerState
  );

  const roverLocationBifrost = useBifrost({
    topic: RosTopic.ROVER_LOCATION,
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
    (state: RootState) => state.roverLocationStore
  );

  const baseLocationStore = useSelector(
    (state: RootState) => state.baseLocationStore
  );

  useEffect(() => {
    if (!mapRef.current){
      setBaseMarker(undefined);
      return;
    } 

    if (!baseMarker) {
      const marker = new Marker({
        element: createBaseIcon(),
      });
      marker.setLngLat([
        baseLocationStore.longitude,
        baseLocationStore.latitude,
      ]);
      marker.addTo(mapRef.current);
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
    mapRef,
  ]);

  useEffect(() => {
    if (!mapRef.current) {
      setRoverMarker(undefined);
      return;
    }

    if (!roverMarker) {
      const marker = new Marker({
        element: createRoverIcon(),
      });
      marker.setLngLat([roverLocation.longitude, roverLocation.latitude]);
      marker.addTo(mapRef.current);
      setRoverMarker(marker);
    } else {
      roverMarker.setLngLat([roverLocation.longitude, roverLocation.latitude]);
    }
  }, [mapRef, roverLocation.latitude, roverLocation.longitude, roverMarker]);

  // Syncs Markers
  useEffect(() => {
    if (!mapRef.current) {
      setPointMarkers([])
      return;
    }

    const newPoints = points.filter((point) =>
      pointMarkers.every((marker) => {
        const lngLat = marker.getLngLat();
        console.log("Marker", marker, "Point", point, lngLat.lat !== point.lat && lngLat.lng !== point.long ? "Not Equal" : "Equal")
        return !(lngLat.lat === point.lat && lngLat.lng === point.long);
      })
    );

    const removedPointMarkers = pointMarkers.filter((marker) => {
      const latlng = marker.getLngLat();
    
      return points.every(
        (point) => !(point.lat === latlng.lat && point.long === latlng.lng)
      );
    });

    removedPointMarkers.forEach((marker) => marker.remove());

    const newMarkers = newPoints.map((point) => {
      const marker = new Marker();
      marker
        .setLngLat([point.long, point.lat])
        .setPopup(
          new Popup({
            closeOnClick: false,
            className: "text-black dark",
          }).setText( point.labelName != null
            ? `${point.name} ( ${point.labelName} )`
            : `${point.name}`)
        )
        .addTo(mapRef.current!)
        .togglePopup();
      return marker;
    });

    const finalPointMarkers = pointMarkers.filter((pointMarker) => {
      return !removedPointMarkers.includes(pointMarker);
    });

    setPointMarkers([...finalPointMarkers, ...newMarkers]);
    console.log("Points", points)
    console.log("Point Markers", [...finalPointMarkers, ...newMarkers])
    console.log("Removed Point Markers", removedPointMarkers)
    console.log("New Point Markers", newMarkers)
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [mapRef, points]);

  useEffect(() => {
    if (!mapRef.current) return;
    const canvas = mapRef.current.getCanvas();
    switch (mapInteractionMode) {
      case MapInteractionMode.PAN:
        canvas.style.cursor = "pointer";
        break;
      case MapInteractionMode.SELECT:
      case MapInteractionMode.MEASURE:
        canvas.style.cursor = "crosshair";
        break;

      default:
        break;
    }
  }, [mapRef, mapInteractionMode]);
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
