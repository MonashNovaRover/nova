import React from "react";
import DriveModeWidget from "../../components/DriveModeWidget/DriveModeWidget";
import WheelTelemetryWidget from "../../components/WheelTelemetryWidget/WheelTelemetryWidget";
import DriveSpeedWidget from "../../components/DriveSpeedWidget/DriveSpeedWidget";
import RFIDWidget from "../../components/RFIDWidget/RFIDWidget";
import ArmWidget from "../../components/ArmWidget/ArmWidget";

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
