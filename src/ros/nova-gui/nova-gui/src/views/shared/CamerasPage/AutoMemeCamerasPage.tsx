import { Button } from "@nextui-org/react";
import { Play, Square } from "react-feather";
import { useCameraStreamer } from "../../../components/cameras/CameraComponent/hooks/useCameraStreamer";
import { CameraView } from "./CameraPageConstants";
import { useState } from "react";
import { CameraControlPanelModal } from "../../../components/cameras/CameraComponent/components/CamerasControlPanelModal";
import ArucoTagCameraComponent from "../../../components/cameras/CameraComponent/special/ArucoTagCameraComponent.tsx";
import SegmentedPicker from "../../../components/shared/components/SegmentedPicker/SegmentedPicker.tsx";
import { SaveAllCamerasModal } from "../../../components/navbar/TopBar/SaveAllCamerasModal";
import { CameraPresetDropdown } from "../../../components/cameras/CameraPresetDropdown";
import SerialMappedCameraComponent from "./SerialMappedCameraComponent.tsx";
import { driveCams } from "./CameraPageConstants.tsx";

/**
 * TODO: remove
 *
 * This is a temp copy of CamerasPage.tsx meant only for the 2026 ARCh meme gui
 */

export interface MemeCameraPageProps {
  views: CameraView[];
}

export const MemeCameraPage = (props: MemeCameraPageProps) => {
  const { views } = props;
  const [controlPanelOpen, setControlPanelOpen] = useState(false);
  const [isSaveModalOpen, setIsSaveModalOpen] = useState(false);

  const closeControlPanel = () => setControlPanelOpen(false);

  const { refreshAvailabilities } = useCameraStreamer();

  const [allCamsOn, setAllCamsOn] = useState(false);

  const [selectedTab, setSelectedTab] = useState(0);

const NON_VIDEO_SERIALS = new Set<string>(driveCams);

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

        <div className="flex flex-row gap-3">
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
        <div className="grid grid-cols-3 gap-3">
          {views[selectedTab].cameraSerials.map((serial, i) =>
            NON_VIDEO_SERIALS.has(serial)
              ? <SerialMappedCameraComponent key={i} cameraSerial={serial} autostart={allCamsOn} />
              : <ArucoTagCameraComponent key={i} cameraSerial={serial} autostart={allCamsOn} />
          )}
        </div>}

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
