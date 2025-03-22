import { Button } from "@nextui-org/react";
import { Play, Square } from "react-feather";
import { useCameraStreamer } from "../../../components/CameraComponent/hooks/useCameraStreamer";
import { CameraView } from "./CameraPageConstants";
import { useState } from "react";
import { CameraControlPanelModal } from "../../../components/CameraComponent/components/CamerasControlPanelModal";
import SerialMappedCameraComponent from "./SerialMappedCameraComponent.tsx";
import SegmentedPicker from "../../../components/SegmentedPicker/SegmentedPicker.tsx";

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
      <div className="flex flex-row justify-between items-center gap-32 pl-1 mb-3">
        <div className="flex flex-row gap-3 items-center">
          {!allCamsOn ? (
            <Button
              size="md"
              color="primary"
              className="w-28"
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

        <SegmentedPicker
          selectedIndex={selectedTab}
          onIndexChange={setSelectedTab}
          children={[
            views.map(v => v.viewTitle)
          ]}
          color="primary"
          className="pb-0"
          fullWidth
          variant="bordered"
        />

        <Button
          size="md"
          color="primary"
          variant="ghost"
          className="w-36"
          onPress={() => setControlPanelOpen(true)}
        >
          Control Panel
        </Button>
      </div>

      {
        <div className="grid grid-cols-4 gap-3">
          {views[selectedTab].cameraSerials.map((serial, i) => (
            <SerialMappedCameraComponent
              cameraSerial={serial}
              key={i}
              autostart={allCamsOn}
            />
          ))}
        </div>
      }

      <CameraControlPanelModal
        showModal={controlPanelOpen}
        closeModal={closeControlPanel}
        refreshAvailabilies={refreshAvailabilities}
      />
    </div>
  );
};
