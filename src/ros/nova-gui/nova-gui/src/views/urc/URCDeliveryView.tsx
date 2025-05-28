import React from "react";
import SerialMappedCameraComponent from "../shared/CamerasPage/SerialMappedCameraComponent.tsx";
import {CameraSerials} from "../shared/CamerasPage/CameraPageConstants.tsx";
import {CameraControlModalButton} from "../../components/CameraComponent/components/CameraControlModelButton.tsx";

export const URCDeliveryView: React.FC = () => {
  return (
    <div className="grid grid-cols-5 gap-3 m-3">
      <div className="col-span-3">
        <SerialMappedCameraComponent cameraSerial={CameraSerials.ARM_END_PERISCOPE}/>
      </div>
      <div  className="col-span-2">
        <SerialMappedCameraComponent cameraSerial={CameraSerials.ARM_END_TOP}/>
      </div>
      <CameraControlModalButton/>
    </div>
  );
};