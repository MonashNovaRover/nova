import DriveModeWidget from "../../components/DriveModeWidget/DriveModeWidget.tsx";
import WheelTelemetryWidget from "../../components/WheelTelemetryWidget/WheelTelemetryWidget.tsx";
import DriveSpeedWidget from "../../components/DriveSpeedWidget/DriveSpeedWidget.tsx";
import TOFHeight from "../../components/AnalysisPlatformHeight/AnalysisPlatformHeight.tsx";
import KilnWidget from "../../components/KilnWidget/KilnWidget.tsx";
import MicroscopeWidget from "../../components/MicroscopeThresholdWidget/MicroscopeWidget.tsx";

export const ARCMicroscopeView = () => {
  return (
    <div className="p-3 max-h-full">
      <div className="grid grid-flow-col auto-cols-fr justify-between items-stretch gap-3 ">
        <div className="flex flex-col flex-grow col-span-3">
          <MicroscopeWidget cameraSerial="science_microscope" />
        </div>
        <div className="flex flex-col gap-3 col-span-3">
          <KilnWidget />
          <DriveModeWidget />
          <WheelTelemetryWidget />
          <DriveSpeedWidget />
          <TOFHeight/>
        </div>
      </div>
    </div>
  );
};
