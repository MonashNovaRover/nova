import React from "react";
import DriveModeWidget from "../../components/DriveModeWidget/DriveModeWidget";
import WheelTelemetryWidget from "../../components/WheelTelemetryWidget/WheelTelemetryWidget";
import DriveSpeedWidget from "../../components/DriveSpeedWidget/DriveSpeedWidget";
import RFIDWidget from "../../components/RFIDWidget/RFIDWidget";

const ARCPostLandingView: React.FC = () => {
  return (
    <div className="grid w-full gap-3 p-3 auto-cols-fr s:grid-cols-2 md:grid-cols-3 lg:grid-cols-4 2xl:grid-cols-6">
      <DriveModeWidget className="row-start-1 w-full col-span-2"/>
      <WheelTelemetryWidget className="row-start-2 w-full col-span-2 row-span-1"/>
      <DriveSpeedWidget className="row-start-3 w-full col-span-2"/>
      <RFIDWidget className="row-start-2 w-full"/>
    </div>
  );
};

export default ARCPostLandingView;
