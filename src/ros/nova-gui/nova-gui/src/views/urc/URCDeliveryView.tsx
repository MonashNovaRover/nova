import React from "react";
import SerialMappedCameraComponent from "../shared/CamerasPage/SerialMappedCameraComponent.tsx";
import {CameraSerials} from "../shared/CamerasPage/CameraPageConstants.tsx";

const URCDeliveryView: React.FC = () => {
  return (
    <div className="grid grid-cols-2 gap-3 m-3">
      <SerialMappedCameraComponent cameraSerial={CameraSerials.ARM_END_PERISCOPE}/>
      <SerialMappedCameraComponent cameraSerial={CameraSerials.ARM_END_TOP}/>
    </div>
  );
};

export default URCDeliveryView;
