import React from "react";
import KilnWidget from "../../components/KilnWidget/KilnWidget";
import MicroscopeComponent from "../../components/CameraComponent/MicroscopeComponent";
import MicroscopeThresholdWidget from "../../components/MicroscopeThresholdWidget/MicroscopeThresholdWidget";
import DriveModeWidget from "../../components/DriveModeWidget/DriveModeWidget";
import WheelTelemetryWidget from "../../components/WheelTelemetryWidget/WheelTelemetryWidget";
import DriveSpeedWidget from "../../components/DriveSpeedWidget/DriveSpeedWidget";

const ARCSpaceResourcesView: React.FC = () => {
  return (
    <div className="p-4 w-screen min-h-screen max-h-full">
      <div className="flex flex-row justify-between items-stretch ">
        <div className="flex flex-col w-[35vw]">
          <DriveModeWidget />
          <WheelTelemetryWidget />
          <DriveSpeedWidget />
          <KilnWidget />
        </div>
        <div className="flex flex-col flex-grow ">
          <MicroscopeComponent />
          <MicroscopeThresholdWidget cameraSerial="science_microscope" />
        </div>
      </div>
    </div>
  );
};

export default ARCSpaceResourcesView;
