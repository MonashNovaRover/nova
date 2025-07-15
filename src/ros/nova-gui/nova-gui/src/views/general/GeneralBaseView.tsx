import DriveModeWidget from "../../components/drive/DriveModeWidget/DriveModeWidget";
import DriveSpeedWidget from "../../components/drive/DriveSpeedWidget/DriveSpeedWidget";
import WheelTelemetryWidget from "../../components/drive/WheelTelemetryWidget/WheelTelemetryWidget";
import ArmWidget from "../../components/arm/ArmWidget/ArmWidget";

const GeneralBaseView: React.FC = () => {
  return (
    <>
      <div className="grid w-full gap-3 p-3 auto-cols-fr s:grid-cols-2 md:grid-cols-3 lg:grid-cols-4 2xl:grid-cols-6">
        <DriveModeWidget className="row-start-1 w-full col-span-2"/>
        <WheelTelemetryWidget className="row-start-2 w-full col-span-2 row-span-1"/>
        <ArmWidget className="row-start-2 w-full col-span-2"/>
        <DriveSpeedWidget className="row-start-3 w-full col-span-2"/>
      </div>
    </>
  );
};

export default GeneralBaseView;
