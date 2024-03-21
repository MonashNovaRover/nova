import React from "react";
import KilnWidget from "../../components/KilnWidget/KilnWidget";
import MicroscopeThresholdWidget from "../../components/MicroscopeThresholdWidget/MicroscopeThresholdWidget";
import DriveModeWidget from "../../components/DriveModeWidget/DriveModeWidget";
import WheelTelemetryWidget from "../../components/WheelTelemetryWidget/WheelTelemetryWidget";
import DriveSpeedWidget from "../../components/DriveSpeedWidget/DriveSpeedWidget";

const ARCSpaceResourcesView: React.FC = () => {
  return (
    <div className="p-3 w-screen min-h-screen max-h-full">
      <div className="grid grid-flow-col auto-cols-fr justify-between items-stretch gap-3 ">
        <div className="flex flex-col gap-3 col-span-2">
          <DriveModeWidget />
          <WheelTelemetryWidget />
          <DriveSpeedWidget />
          <KilnWidget />
        </div>
        <div className="flex flex-col flex-grow col-span-3">
          <MicroscopeThresholdWidget cameraSerial="science_microscope" />
        </div>
      </div>
    </div>
  );
};

export default ARCSpaceResourcesView;
