import { useState, useEffect } from "react";
import { useSelector } from "react-redux";
import { useBifrost } from "../../redux/actions/bifrost/useBifrostAction";
import { useCartographerActions } from "../../redux/actions/useCartographerActions";
import { RootState } from "../../redux/RootState";
import { RosTopic } from "../../ros/topics/rosTopic";
import { MarkerModal } from "./components/MarkerModal";
import { MapTilerMap } from "./maptiler/MapTilerMap";
import { ToolTipButton } from "../shared/TooltipButton";
import { PropRenderer } from "../shared/PropRenderer";
import { MapInteractionMode } from "../../redux/models/CartographerState";
import { MapPin } from "react-feather";
import { NewMarkerModal } from "./components/NewMarkerModal";
import { BottomOverlay } from "./components/BottomOverlay";

export const Cartographer = () => {
  const [roverInfoOpen, setRoverInfoOpen] = useState(false);
  const [baseStationModal, setBaseStationModal] = useState(false);

  const roverLocationBifrost = useBifrost({
    topic: RosTopic.AUTO_ROVER_LOCATION,
  });

  const baseLocationBifrost = useBifrost({
    topic: RosTopic.BASE_LOCATION,
  });

  const { mapInteractionMode, mousePosition, newMarkerModal } = useSelector(
    (state: RootState) => state.cartographerState
  );

  useEffect(() => {
    roverLocationBifrost.syncWithTopic();
  }, [roverLocationBifrost]);

  useEffect(() => {
    baseLocationBifrost.syncWithTopic();
  }, [baseLocationBifrost]);

  const { addPoint, toggleMapInteractionMode, closeNewModal } =
    useCartographerActions();

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
        isOpen={newMarkerModal.open}
        addPoint={addPoint}
        closeModal={closeNewModal}
        latitude={newMarkerModal.coordinate?.lat}
        longitude={newMarkerModal.coordinate?.long}
      />
      <div className="flex h-[95vh]">
        <MapTilerMap
          overlay={
            <>
              <div className="flex flex-row gap-1 absolute top-2 right-2">
                {mapInteractionMode === MapInteractionMode.SELECT &&
                  mousePosition && (
                    <PropRenderer
                      props={{
                        latitude: mousePosition.lat,
                        longitude: mousePosition.long,
                      }}
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
            </>
          }
        />
        <div className="fixed bottom-0 w-full">
          <BottomOverlay />
        </div>
      </div>
    </div>
  );
};
