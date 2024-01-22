import { useState } from "react";
import './DriveWidget.css';
import DriveSpeedWidget from "../DriveSpeedWidget/DriveSpeedWidget";
import WheelTelemetryWidget from "../WheelTelemetryWidget/WheelTelemetryWidget.tsx";
import DriveModeWidget from "./DriveModeWidget.tsx";

const DriveWidgetDemo: React.FC = () => {

  const [driveModeIndex, setDriveModeIndex] = useState("0");

  const handleDriveModeSelectChange =
    (e: React.ChangeEvent<HTMLSelectElement>) => {
      setDriveModeIndex(e.target.value);
    };

  return (
    <div className="grid  w-full gap-3 p-3 auto-cols-fr
      s:grid-cols-2 md:grid-cols-3 lg:grid-cols-4 2xl:grid-cols-6">

      <DriveModeWidget className="w-full col-span-2"></DriveModeWidget>
      
      <DriveSpeedWidget className="w-full col-span-2"/>

      <WheelTelemetryWidget className="row-start-2 w-full col-span-2 row-span-1"></WheelTelemetryWidget>

    </div>


  );
};

export default DriveWidgetDemo;
