import React from "react";
import SerialMappedCameraComponent from "../shared/CamerasPage/SerialMappedCameraComponent.tsx";

const URCAutonomousNavigationView: React.FC = () => {
  return (
    <div className="grid grid-cols-2 gap-3">
      <SerialMappedCameraComponent cameraSerial={"oak-rgb"} />
      <SerialMappedCameraComponent cameraSerial={"oak-depth"} />
      <SerialMappedCameraComponent cameraSerial={"bootie-rgb"} />
      <SerialMappedCameraComponent cameraSerial={"bootie-depth"} />
    </div>
  );
};

export default URCAutonomousNavigationView;
