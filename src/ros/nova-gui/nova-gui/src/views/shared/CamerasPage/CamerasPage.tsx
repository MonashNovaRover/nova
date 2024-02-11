import { Button, Tab, Tabs } from "@nextui-org/react";
import { CameraComponent } from "../../../components/CameraComponent/CameraComponent";
import { Play, Square } from "react-feather";
import { useCameraStreamer } from "../../../components/CameraComponent/hooks/useCameraStreamer";
import { CameraView } from "./CameraPageConstants";
import { useState } from "react";
import { CameraControlPanelModal } from "../../../components/CameraComponent/components/CamerasControlPanelModal";

export interface CameraPageProps {
  views: CameraView[];
}

export const CameraPage = (props: CameraPageProps) => {
  const { views } = props;
  const [controlPanelOpen, setControlPanelOpen] = useState(false);

  const closeControlPanel = () => setControlPanelOpen(false);

  const { refreshAvailabilities } = useCameraStreamer();

  return (
    <div>
      <div className="flex flex-row justify-between items-center m-6 gap-4">
        <div className="flex flex-row m-4 ml-0 gap-4 items-center">
          <Button size="sm" color="primary">
            <Play size="15px" fill="white" /> Start All
          </Button>
          <Button size="sm" color="danger">
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
        {views.map((view, i) => (
          <Tab title={view.viewTitle} key={i}>
            <div className="grid grid-cols-3">
              {view.cameraSerials.map((serial, i) => (
                <CameraComponent cameraSerial={serial} key={i} />
              ))}
            </div>
          </Tab>
        ))}
      </Tabs>
      <CameraControlPanelModal
        showModal={controlPanelOpen}
        closeModal={closeControlPanel}
        refreshAvailabilies={refreshAvailabilities}
      />
    </div>
  );
};
