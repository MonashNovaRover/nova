import React from "react";
import AutoTypingKeyEntryWidget from "../../components/AutoTyping/AutoTypingKeyEntryWidget.tsx";
import SerialMappedCameraComponent from "../shared/CamerasPage/SerialMappedCameraComponent.tsx";
import KeyboardOverlayedCameraComponent from "../../components/CameraComponent/special/KeyboardOverlayedCameraComponent.tsx";
import ArmTypingWidget from "../../components/ArmWidget/ArmTypingWidget.tsx";
import {CameraSerials} from "../shared/CamerasPage/CameraPageConstants.tsx";
import { useCameraStreamer } from "../../components/CameraComponent/hooks/useCameraStreamer";


const URCAutoTypingView: React.FC = () => {
  useCameraStreamer();
  return <div className="grid m-3 gap-3">
    <div className="grid grid-cols-2 gap-3 h-2/3">
      <div className="grid grid-col gap-3">
        <KeyboardOverlayedCameraComponent cameraSerial={CameraSerials.ARM_END_PERISCOPE}/>
        <AutoTypingKeyEntryWidget/>
      </div>
      <div className="grid grid-cols-2 gap-3">
        <ArmTypingWidget/>
        <div className="grid grid-row-2 gap-3">
          <SerialMappedCameraComponent cameraSerial={CameraSerials.DRIVE_TELEMETRY}/>
          <SerialMappedCameraComponent cameraSerial={CameraSerials.WHEEL_TELEMETRY}/>
        </div>
      </div>
    </div>
    <div className="flex row-5 gap-3">
      <SerialMappedCameraComponent cameraSerial={CameraSerials.ARM_END_FINGER}/>
      <SerialMappedCameraComponent cameraSerial={CameraSerials.ARM_END_SIDE}/>
      <SerialMappedCameraComponent cameraSerial={CameraSerials.ARM_END_TOP}/>
      <SerialMappedCameraComponent cameraSerial={CameraSerials.MAST_FORWARD}/>
      <SerialMappedCameraComponent cameraSerial={CameraSerials.MAST_ARM_STOW}/>
    </div>
  </div>;
};

export default URCAutoTypingView;
