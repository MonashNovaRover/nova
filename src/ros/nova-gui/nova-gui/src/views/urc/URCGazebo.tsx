import React from "react";
import { useState } from "react";
import { Button } from "@nextui-org/react";
import SerialMappedCameraComponent from "../shared/CamerasPage/SerialMappedCameraComponent.tsx";
import { CameraControlPanelModal } from "../../components/cameras/CameraComponent/components/CamerasControlPanelModal";
import { useCameraStreamer } from "../../components/cameras/CameraComponent/hooks/useCameraStreamer";

const URCGazeboView: React.FC = () => {
  const [controlPanelOpen, setControlPanelOpen] = useState(false);
  const { refreshAvailabilities } = useCameraStreamer();
  const closeControlPanel = () => setControlPanelOpen(false);
  return (
    <div className="p-3 flex flex-col gap-3">
      <div>
        <CameraControlPanelModal showModal={controlPanelOpen} closeModal={closeControlPanel} refreshAvailabilies={refreshAvailabilities} />
        <Button size="md" color="primary" variant="ghost" className="w-36" onPress={() => setControlPanelOpen(true)}> Control Panel </Button>
      </div>
      <div className="grid grid-cols-2 gap-3">
        <SerialMappedCameraComponent cameraSerial={"oak-rgb"} autostart={true}/>
        <SerialMappedCameraComponent cameraSerial={"bootie-rgb"} autostart={true}/>
      </div>
    </div>
  );
};

export default URCGazeboView;
