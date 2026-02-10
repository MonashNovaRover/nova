import { Button } from "@nextui-org/react";
import { Play, Square } from "react-feather";
import { useCameraStreamer } from "../../../components/cameras/CameraComponent/hooks/useCameraStreamer";
import { CameraView } from "./CameraPageConstants";
import { useState } from "react";
import { CameraControlPanelModal } from "../../../components/cameras/CameraComponent/components/CamerasControlPanelModal";
import SerialMappedCameraComponent from "./SerialMappedCameraComponent.tsx";
import SegmentedPicker from "../../../components/shared/components/SegmentedPicker/SegmentedPicker.tsx";
import {CameraStreamingButton} from "../../../components/cameras/CameraComponent/components/CameraStreamingButtons.tsx";
import {
  CamerasCircularProgress
} from "../../../components/cameras/CameraComponent/components/CameraCircularProgress.tsx";
import { SaveAllCamerasModal } from "../../../components/navbar/TopBar/SaveAllCamerasModal";
import { CameraPresetDropdown } from "../../../components/cameras/CameraPresetDropdown";

export interface CameraPageProps {
  views: CameraView[];
}

export const CameraPage = (props: CameraPageProps) => {
  const { views } = props;
  const [controlPanelOpen, setControlPanelOpen] = useState(false);
  const [isSaveModalOpen, setIsSaveModalOpen] = useState(false);

  const closeControlPanel = () => setControlPanelOpen(false);

  const { refreshAvailabilities } = useCameraStreamer();

  const [allCamsOn, setAllCamsOn] = useState(false);

  const [selectedTab, setSelectedTab] = useState(0);

  return (
    <div className="p-3 flex flex-col gap-0">
      <div className="relative flex items-center mb-3 gap-16">
        <div className="flex-1 flex flex-row gap-3 items-center">
          {!allCamsOn ? (
            <Button
              size="md"
              color="primary"
              className="w-36"
              onPress={() => setAllCamsOn(true)}
            >
              <Play size="15px" fill="white" /> Display All
            </Button>
          ) : (
            <Button
              size="md"
              color="danger"
              className="w-36"
              onPress={() => setAllCamsOn(false)}
            >
              <Square size="15px" fill="white" /> Stop All
            </Button>
          )}

          <CameraStreamingButton isStartButton={true} refreshAvailabilies={refreshAvailabilities} size={"md"}/>
          <CamerasCircularProgress/>
        </div>

        <SegmentedPicker
          selectedIndex={selectedTab}
          onIndexChange={setSelectedTab}
          children={[
            views.map(v => v.viewTitle)
          ]}
          color="primary"
          className="pb-0 mx-auto"
          fullWidth
          variant="bordered"
        />

        <div className="flex-1 flex flex-row gap-3 ml-auto pl-36">
          <Button
            size="md"
            color="primary"
            variant="ghost"
            className="w-36"
            onPress={() => setControlPanelOpen(true)}
          >
            Control Panel
          </Button>
          <CameraPresetDropdown onSavePress={() => setIsSaveModalOpen(true)} />
        </div>
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
      <SaveAllCamerasModal
        isOpen={isSaveModalOpen}
        onClose={() => setIsSaveModalOpen(false)}
      />
    </div>
  );
};
