import React from "react";
import AutoTypingWidget from "../../components/AutoTypingWidget/AutoTypingWidget.tsx";
import SerialMappedCameraComponent from "../shared/CamerasPage/SerialMappedCameraComponent.tsx";
import {CameraSerials} from "../shared/CamerasPage/CameraPageConstants.tsx";


const URCAutoTypingView: React.FC = () => {
  return <div className="m-3 grid grid-cols-2 gap-3">
    <div className="flex flex-col">
      <SerialMappedCameraComponent cameraSerial={CameraSerials.ARM_END_PERISCOPE}/>
      <AutoTypingWidget />
    </div>
    <div className="flex flex-col">
    </div>
  </div>;
};

export default URCAutoTypingView;
