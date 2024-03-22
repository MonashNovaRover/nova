import DriveModeWidget from "../../components/DriveModeWidget/DriveModeWidget.tsx";
import WheelTelemetryWidget from "../../components/WheelTelemetryWidget/WheelTelemetryWidget.tsx";
import DriveSpeedWidget from "../../components/DriveSpeedWidget/DriveSpeedWidget.tsx";
import MicroscopeThresholdWidget from "../../components/MicroscopeThresholdWidget/MicroscopeThresholdWidget.tsx";
import AnalysisPlatformHeight from "../../components/AnalysisPlatformHeight/AnalysisPlatformHeight.tsx";
import KilnWidget from "../../components/KilnWidget/KilnWidget.tsx";

export const ARCMicroscopeView = () => {
  return (
    <div className="p-3 max-h-full">
      <div className="grid grid-flow-col auto-cols-fr justify-between items-stretch gap-3 ">
        <div className="flex flex-col flex-grow col-span-3">
          <MicroscopeThresholdWidget cameraSerial="science_microscope" />
        </div>
        <div className="flex flex-col gap-3 col-span-2">
          <AnalysisPlatformHeight/>
          <DriveModeWidget />
          <WheelTelemetryWidget />
          <DriveSpeedWidget />
          <KilnWidget />
        </div>
      </div>
    </div>
  );
};
