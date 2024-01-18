import { useState } from "react";
import './DriveWidget.css';
import DriveWidget from "./DriveWidget";
import DriveWheelWidget from "../DriveWheelWidget/DriveWheelWidget.tsx";
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
      
      <DriveWidget className="w-full col-span-2"
                   driveModeIndex={driveModeIndex}
                   handleDriveModeSelectChange={handleDriveModeSelectChange}
                   setDriveModeIndex={setDriveModeIndex}>
      </DriveWidget>

      <DriveWheelWidget className="row-start-2 w-full col-span-2 row-span-1"></DriveWheelWidget>

    </div>


  );
};

export default DriveWidgetDemo;
