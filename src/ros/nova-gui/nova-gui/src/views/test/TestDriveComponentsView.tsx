import DriveModeWidget from "../../components/DriveModeWidget/DriveModeWidget.tsx";
import DriveSpeedWidget from "../../components/DriveSpeedWidget/DriveSpeedWidget.tsx";
import WheelTelemetryWidget from "../../components/WheelTelemetryWidget/WheelTelemetryWidget.tsx";

const TestDriveComponentsView: React.FC = () => {
  return (
    <div className="grid  w-full gap-3 p-3 auto-cols-fr s:grid-cols-2 md:grid-cols-3 lg:grid-cols-4 2xl:grid-cols-6">
      <DriveModeWidget className="w-full col-span-2"></DriveModeWidget>
      <DriveSpeedWidget className="w-full col-span-2"/>
      <WheelTelemetryWidget className="row-start-2 w-full col-span-2 row-span-1"></WheelTelemetryWidget>
    </div>
  )
};

export default TestDriveComponentsView;
