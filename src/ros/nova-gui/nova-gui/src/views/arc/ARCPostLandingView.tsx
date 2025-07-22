import React from "react";
import DriveModeWidget from "../../components/drive/DriveModeWidget/DriveModeWidget";
import WheelTelemetryWidget from "../../components/drive/WheelTelemetryWidget/WheelTelemetryWidget";
import DriveSpeedWidget from "../../components/drive/DriveSpeedWidget/DriveSpeedWidget";
import RFIDWidget from "../../components/arm/RFIDWidget/RFIDWidget";
import ArmWidget from "../../components/arm/ArmWidget/ArmWidget";

const ARCPostLandingView: React.FC = () => {
  return (
    <div className="w-full grid grid-cols-1 gap-3 p-3 md:grid-cols-2">
      <WheelTelemetryWidget className="row-span-2 w-full" />
      <DriveModeWidget className="w-full" />
      <DriveSpeedWidget className="w-full" />
      <ArmWidget className="row-span-2 w-full" />
      <RFIDWidget className="w-full h-full row-span-2" />
    </div>
  );
};

export default ARCPostLandingView;
