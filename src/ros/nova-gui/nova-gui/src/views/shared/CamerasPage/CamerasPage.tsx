import { Button, Tab, Tabs } from "@nextui-org/react";
import { Play, Square } from "react-feather";
import { useCameraStreamer } from "../../../components/CameraComponent/hooks/useCameraStreamer";
import { CameraView } from "./CameraPageConstants";
import { useState } from "react";
import { CameraControlPanelModal } from "../../../components/CameraComponent/components/CamerasControlPanelModal";
import SerialMappedCameraComponent from "./SerialMappedCameraComponent.tsx";

export interface CameraPageProps {
  views: CameraView[];
}

export const CameraPage = (props: CameraPageProps) => {
  const { views } = props;
  const [controlPanelOpen, setControlPanelOpen] = useState(false);

  const closeControlPanel = () => setControlPanelOpen(false);

  const { refreshAvailabilities } = useCameraStreamer();

  const [allCamsOn, setAllCamsOn] = useState(false);

  const [selectedTab, setSelectedTab] = useState(0);

  return (
    <div className="p-3 flex flex-col gap-0">
      <div className="flex flex-row justify-between items-center gap-3 pl-1 mb-3">
        <div className="flex flex-row gap-3 items-center">
          {!allCamsOn ? (
            <Button
              size="md"
              color="primary"
              onPress={() => setAllCamsOn(true)}
            >
              <Play size="15px" fill="white" /> Start All
            </Button>
          ) : (
            <Button
              size="md"
              color="danger"
              onPress={() => setAllCamsOn(false)}
            >
              <Square size="15px" fill="white" /> Stop All
            </Button>
          )}
        </div>
        <Button
          size="md"
          color="primary"
          variant="ghost"
          onPress={() => setControlPanelOpen(true)}
        >
          Control Panel
        </Button>
      </div>
      <Tabs
        size="lg"
        color="primary"
        className="pb-0"
        fullWidth
        variant="bordered"
        selectedKey={selectedTab}
        onSelectionChange={(key) => {
          setSelectedTab(key as number);
        }}
      >
        {views.map((view, i) => (
          <Tab title={view.viewTitle} key={i}>
            <div className="grid grid-cols-4 gap-3">
              {view.cameraSerials.map((serial, i) => (
                <SerialMappedCameraComponent
                  cameraSerial={serial}
                  key={i}
                  autostart={allCamsOn}
                />
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
