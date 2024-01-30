import { Button, Tab, Tabs } from "@nextui-org/react";
import { CameraComponent } from "../../../components/CameraComponent/CameraComponent";
import { Play, Square } from "react-feather";
import { useCameraStreamer } from "../../../components/CameraComponent/hooks/useCameraStreamer";
import { useSelector } from "react-redux";
import { RootState } from "../../../redux/RootState";
import humanizeString from "humanize-string";
import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction";
import { RosService } from "../../../ros/services/rosService";
import { cameraSections } from "./CameraPageConstants";

export const CameraPage = () => {
  const cameras = useSelector(
    (state: RootState) => state.cameraStreamerState.cameras
  );

  const bifrostStarter = useBifrost({ service: RosService.START_CAMS });

  const bifrostStopper = useBifrost({ service: RosService.STOP_CAMS });

  const { refreshAvailability } = useCameraStreamer();
  return (
    <div>
      <div className="flex flex-row m-4 gap-4">
        <Button
          size="sm"
          color="primary"
          onClick={() =>
            bifrostStarter.callService(
              { serials: [] },
              {
                responseToast: true,
                successToastMessage: "All Cameras Started Up!",
                handleResponse: () => refreshAvailability(),
              }
            )
          }
        >
          <Play size="15px" fill="white" /> Start All
        </Button>
        <Button
          size="sm"
          color="danger"
          onClick={() =>
            bifrostStopper.callService(
              { serials: [] },
              {
                responseToast: true,
                successToastMessage: "All Cameras Stopped!",
                handleResponse: () => refreshAvailability(),
              }
            )
          }
        >
          <Square size="15px" fill="white" /> Stop All
        </Button>
      </div>
      <Tabs
        size="lg"
        color="primary"
        className=" p-4"
        fullWidth
        variant="bordered"
      >
        {cameraSections.map((section) => (
          <Tab title={section.sectionTitle}>
            <div className="grid grid-cols-3">
              {section.cameraSerials.map((serial, i) => (
                <CameraComponent
                  cameraName={humanizeString(serial)}
                  camera={camera}
                  key={i}
                />
              ))}
            </div>
          </Tab>
        ))}
        <Tab title="Extreme Delivery"></Tab>
        <Tab title="Autonomous"></Tab>
        <Tab title="Science"></Tab>
        <Tab title="Equipment Servicing"></Tab>
      </Tabs>
    </div>
  );
};
