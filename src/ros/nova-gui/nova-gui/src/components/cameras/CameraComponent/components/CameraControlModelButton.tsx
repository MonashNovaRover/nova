import { Button } from "@nextui-org/react";
import { useState } from "react";
import {CameraControlPanelModal} from "./CamerasControlPanelModal.tsx";
import {useCameraStreamer} from "../hooks/useCameraStreamer.ts";

export const CameraControlModalButton = () => {
  const [controlPanelOpen, setControlPanelOpen] = useState(false);
  const closeControlPanel = () => setControlPanelOpen(false);
  const { refreshAvailabilities } = useCameraStreamer();

  return (
    <div>
      <Button
        size="md"
        color="primary"
        variant="ghost"
        className="w-36"
        onPress={() => setControlPanelOpen(true)}
      >
        Control Panel
      </Button>
      <CameraControlPanelModal
        showModal={controlPanelOpen}
        closeModal={closeControlPanel}
        refreshAvailabilies={refreshAvailabilities}
      />
    </div>
  );
};
