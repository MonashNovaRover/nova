import React from "react";
import DriveModeWidget from "../../components/drive/DriveModeWidget/DriveModeWidget";
import WheelTelemetryWidget from "../../components/drive/WheelTelemetryWidget/WheelTelemetryWidget";
import DriveSpeedWidget from "../../components/drive/DriveSpeedWidget/DriveSpeedWidget";

const ARCSpaceResourcesView: React.FC = () => {
  return (
    <div className="p-3 flex flex-col gap-3">
      <DriveModeWidget />
      <WheelTelemetryWidget />
      <DriveSpeedWidget />
    </div>
  );
};

export default ARCSpaceResourcesView;
