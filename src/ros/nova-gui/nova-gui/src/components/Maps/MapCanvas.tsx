import { AdvancedMarker, APIProvider, Map } from "@vis.gl/react-google-maps";
import novaLogo from "../../assets/nova-logo.png";
import rover from "../../assets/rover-top-down-dark.png";
import { Card, CardBody } from "@nextui-org/react";
// import { useDrawingManager } from "./hooks/useDrawingManager";
import { useState } from "react";
import { WaratahInfoModal } from "./components/RoverInfoModal";
import { BaseStationModal } from "./components/BaseStationModal";

export const MapCanvas = () => {
  // const drawingManager = useDrawingManager();

  const [roverInfoOpen, setRoverInfoOpen] = useState(false);
  const [baseStationModal, setBaseStationModal] = useState(false);

  return (
    <>
      <WaratahInfoModal isOpen={roverInfoOpen} setOpen={setRoverInfoOpen} />
      <BaseStationModal
        isOpen={baseStationModal}
        setOpen={setBaseStationModal}
      />
      <APIProvider apiKey={"AIzaSyAHBHVjWPwibfLTFqf6PZQVdj_5mbQGwyA"}>
        <Map
          className=" h-[95vh]"
          defaultCenter={{ lat: -37.9106066, lng: 145.1350033 }}
          defaultZoom={20}
          gestureHandling={"greedy"}
          disableDefaultUI
          mapTypeId={"satellite"}
          mapId={"eebc051b3c467e94"}
        >
          <AdvancedMarker
            position={{ lat: -37.9106066, lng: 145.1350033 }}
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
            position={{ lat: -37.9106996, lng: 145.1359039 }}
            title={"Waratah"}
            onClick={() => setRoverInfoOpen(true)}
          >
            <img src={rover} className="w-20 " />
          </AdvancedMarker>
        </Map>
      </APIProvider>
    </>
  );
};
