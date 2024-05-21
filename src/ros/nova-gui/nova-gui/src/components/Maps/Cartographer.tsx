import { useState, useEffect } from "react";
import { useSelector } from "react-redux";
import { useBifrost } from "../../redux/actions/bifrost/useBifrostAction";
import { useCartographerActions } from "../../redux/actions/useCartographerActions";
import { RootState } from "../../redux/RootState";
import { RosTopic } from "../../ros/topics/rosTopic";
import { MarkerModal } from "./components/MarkerModal";
import { NewMarkerModal } from "./components/NewMarkerModal";
import { MapTilerMap } from "./maptiler/MapTilerMap";
import { ToolTipButton } from "../shared/TooltipButton";
import { PropRenderer } from "../shared/PropRenderer";
import { MapInteractionMode } from "../../redux/models/CartographerState";
import { BottomOverlay } from "./components/BottomOverlay";
import { MapPin } from "react-feather";

export const Cartographer = () => {
  const [roverInfoOpen, setRoverInfoOpen] = useState(false);
  const [baseStationModal, setBaseStationModal] = useState(false);
  const [newMarkerModal, setNewMarkerModal] = useState(false);

  const roverLocationBifrost = useBifrost({
    topic: RosTopic.AUTO_ROVER_LOCATION,
  });

  const baseLocationBifrost = useBifrost({
    topic: RosTopic.BASE_LOCATION,
  });

  const [mouseCoordinates, setMouseCoordinates] =
    useState<google.maps.LatLngLiteral>();

  const { points, mapInteractionMode } = useSelector(
    (state: RootState) => state.cartographerState
  );

  useEffect(() => {
    roverLocationBifrost.syncWithTopic();
  }, [roverLocationBifrost]);

  useEffect(() => {
    baseLocationBifrost.syncWithTopic();
  }, [baseLocationBifrost]);

  const autoRoverLocation = useSelector(
    (state: RootState) => state.autoRoverLocationStore
  );

  const baseLocationStore = useSelector(
    (state: RootState) => state.baseLocationStore
  );

  const { addPoint, toggleMapInteractionMode } = useCartographerActions();

  return (
    <div className="w-full ">
      <MarkerModal
        isOpen={roverInfoOpen}
        setOpen={setRoverInfoOpen}
        data={{}}
        title="Waratah"
      />
      <MarkerModal
        isOpen={baseStationModal}
        setOpen={setBaseStationModal}
        data={{}}
        title="Base Station"
      />
      <NewMarkerModal
        isOpen={newMarkerModal}
        setOpen={setNewMarkerModal}
        latitude={mouseCoordinates?.lat}
        longitude={mouseCoordinates?.lng}
        addPoint={addPoint}
      />
      <MapTilerMap
        overlay={
          <>
            <div className="flex flex-row gap-1 absolute top-2 right-2">
              {mapInteractionMode === MapInteractionMode.SELECT &&
                mouseCoordinates && (
                  <PropRenderer
                    props={mouseCoordinates}
                    ignoreProps={[]}
                    row
                    size="sm"
                  />
                )}

              <ToolTipButton
                placement="left"
                tooltipContent={"Drop Pins"}
                className="bottom-0 right-0"
                variant="shadow"
                isIconOnly
                size="lg"
                color={
                  mapInteractionMode === MapInteractionMode.PAN
                    ? "default"
                    : "success"
                }
                onClick={toggleMapInteractionMode}
              >
                <MapPin size={20} />
              </ToolTipButton>
            </div>
            <BottomOverlay />
          </>
        }
      />
    </div>
  );
};
