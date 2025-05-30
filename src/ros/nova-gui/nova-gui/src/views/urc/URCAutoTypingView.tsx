import React, { useState } from "react";
import AutoTypingKeyEntryWidget from "../../components/AutoTyping/AutoTypingKeyEntryWidget.tsx";
import SerialMappedCameraComponent from "../shared/CamerasPage/SerialMappedCameraComponent.tsx";
import ArmTypingWidget from "../../components/ArmWidget/ArmTypingWidget.tsx";
import {CameraSerials} from "../shared/CamerasPage/CameraPageConstants.tsx";
import { useCameraStreamer } from "../../components/CameraComponent/hooks/useCameraStreamer";
import AutoTypingModal from "../../components/AutoTyping/AutoTypingModal.tsx";


const URCAutoTypingView: React.FC = () => {
  useCameraStreamer();

  const [infoPanelOpen, setInfoPanelOpen] = useState(false);
  const closeInfoPanel = () => setInfoPanelOpen(false);

  return <div>
  <div className="grid m-3 gap-3">
    <div className="flex flex-col lg:flex-row gap-3">
      {/* Left Column (Col 1) */}
      <div className="flex flex-col gap-3 w-full lg:w-1/2">
        <AutoTypingKeyEntryWidget 
          cameraSerial={CameraSerials.ARM_END_PERISCOPE}
          showHelp={() => setInfoPanelOpen(true)}
          />
      </div>

      {/* Right Columns (Cols 2 + 3) */}
      <div className="grid grid-cols-1 lg:grid-cols-2 gap-3 w-full lg:w-1/2">
        <ArmTypingWidget/>
        <div className="grid grid-rows-2 gap-3">
          <SerialMappedCameraComponent cameraSerial={CameraSerials.DRIVE_TELEMETRY}/>
          <SerialMappedCameraComponent cameraSerial={CameraSerials.WHEEL_TELEMETRY}/>
        </div>
      </div>
    </div>

    {/* Bottom Row of 5 cameras */}
    <div className="flex flex-wrap gap-3">
      <SerialMappedCameraComponent cameraSerial={CameraSerials.ARM_END_FINGER}/>
      <SerialMappedCameraComponent cameraSerial={CameraSerials.ARM_END_SIDE}/>
      <SerialMappedCameraComponent cameraSerial={CameraSerials.ARM_END_TOP}/>
      <SerialMappedCameraComponent cameraSerial={CameraSerials.MAST_FORWARD}/>
      <SerialMappedCameraComponent cameraSerial={CameraSerials.MAST_ARM_STOW}/>
    </div>
  </div>

  <AutoTypingModal showModal={infoPanelOpen} closeModal={closeInfoPanel}/>
</div>
};

export default URCAutoTypingView;
