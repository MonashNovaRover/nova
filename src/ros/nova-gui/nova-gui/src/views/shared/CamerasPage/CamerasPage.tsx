import { Button, Tab, Tabs } from "@nextui-org/react";
import { CameraComponent } from "../../../components/CameraComponent/CameraComponent";
import { Play, Square } from "react-feather";
import { useCameraStreamer } from "../../../components/CameraComponent/hooks/useCameraStreamer";
import humanizeString from "humanize-string";
import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction";
import { RosService } from "../../../ros/services/rosService";
import { cameraSections } from "./CameraPageConstants";
import { useState } from "react";
import { CameraControlPanelModal } from "../../../components/CameraComponent/components/CamerasControlPanelModal";

export const CameraPage = () => {
  const [controlPanelOpen, setControlPanelOpen] = useState(false);

  const closeControlPanel = () => setControlPanelOpen(false);

  const bifrostStarter = useBifrost({ service: RosService.START_CAMS });

  const bifrostStopper = useBifrost({ service: RosService.STOP_CAMS });

  const { refreshAvailability } = useCameraStreamer();

  return (
    <div>
      <div className="flex flex-row justify-between items-center m-6 gap-4">
        <div className="flex flex-row m-4 ml-0 gap-4 items-center">
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
        <Button
          className="m-4 mr-0"
          size="sm"
          color="primary"
          onClick={() => setControlPanelOpen(true)}
        >
          Control Panel
        </Button>
      </div>
      <Tabs
        size="lg"
        color="primary"
        className=" p-4"
        fullWidth
        variant="bordered"
      >
        {cameraSections.map((section, i) => (
          <Tab title={section.sectionTitle} key={i}>
            <div className="grid grid-cols-3">
              {section.cameraSerials.map((serial, i) => (
                <CameraComponent
                  cameraName={humanizeString(serial)}
                  cameraSerial={serial}
                  key={i}
                />
              ))}
            </div>
          </Tab>
        ))}
      </Tabs>
      <CameraControlPanelModal
        showModal={controlPanelOpen}
        closeModal={closeControlPanel}
      />
    </div>
  );
};
