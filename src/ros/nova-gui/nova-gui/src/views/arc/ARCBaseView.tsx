import React from "react";
import KilnWidget from "../../components/KilnWidget/KilnWidget";
import WheelTelemetryWidget from "../../components/WheelTelemetryWidget/WheelTelemetryWidget";
import DriveModeWidget from "../../components/DriveModeWidget/DriveModeWidget";

const ARCBaseView: React.FC = () => {
  return (
    <div>
      <KilnWidget/>
      <WheelTelemetryWidget/>
      <DriveModeWidget/>
    </div>
  );
};

export default ARCBaseView;
