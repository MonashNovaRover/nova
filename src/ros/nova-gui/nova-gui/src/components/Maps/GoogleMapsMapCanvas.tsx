import { AdvancedMarker, Map } from "@vis.gl/react-google-maps";
import novaLogo from "../../assets/nova-logo.png";
import rover from "../../assets/rover-top-down-dark.png";
import { Card, CardBody } from "@nextui-org/react";
import { useEffect, useState } from "react";
import { MarkerModal } from "./components/MarkerModal";
import { MapPin } from "react-feather";
import { ToolTipButton } from "../shared/TooltipButton";
import { PropRenderer } from "../shared/PropRenderer";
import { NewMarkerModal } from "./components/NewMarkerModal";
import { BottomOverlay } from "./components/BottomOverlay";
import { useSelector } from "react-redux";
import { RootState } from "../../redux/RootState";
import { useCartographerActions } from "../../redux/actions/useCartographerActions";
import { MapInteractionMode } from "../../redux/models/CartographerState";
import { useBifrost } from "../../redux/actions/bifrost/useBifrostAction";
import { RosTopic } from "../../ros/topics/rosTopic";

export const MapCanvas = () => {
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
    <>
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
      <Map
        className=" h-[95vh]"
        defaultCenter={{ lat: -37.9106066, lng: 145.1350033 }}
        defaultZoom={20}
        gestureHandling={"greedy"}
        disableDefaultUI
        mapTypeId={"satellite"}
        mapId={"eebc051b3c467e94"}
        onClick={(event) => {
          const latLng = event.detail.latLng;
          if (!latLng || mapInteractionMode === MapInteractionMode.PAN) return;

          setNewMarkerModal(true);
        }}
        onMousemove={(event) => {
          setMouseCoordinates(event.detail.latLng ?? undefined);
        }}
      >
        <AdvancedMarker
          position={{
            lat: baseLocationStore.latitude,
            lng: baseLocationStore.longitude,
          }}
          title={"Base Station"}
          onClick={() => setBaseStationModal(true)}
        >
          <Card>
            <CardBody>
              <img src={novaLogo} className="w-16 " />
            </CardBody>
          </Card>
        </AdvancedMarker>
        <AdvancedMarker
          position={{
            lat: autoRoverLocation.latitude,
            lng: autoRoverLocation.longitude,
          }}
          title={"Waratah"}
          onClick={() => setRoverInfoOpen(true)}
        >
          <img src={rover} className="w-20 " />
        </AdvancedMarker>
        {points.map((point) => (
          <AdvancedMarker
            position={{
              lat: point.lat,
              lng: point.long,
            }}
            title={point.name}
          />
        ))}
        <div className="w-full h-full flex flex-col justify-between">
          <div className="w-full h-full flex align-middle justify-end p-4">
            <div className="flex flex-row gap-1">
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
          </div>
          <BottomOverlay />
        </div>
      </Map>
    </>
  );
};
