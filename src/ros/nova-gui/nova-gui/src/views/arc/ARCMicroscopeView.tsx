import DriveModeWidget from "../../components/drive/DriveModeWidget/DriveModeWidget.tsx";
import WheelTelemetryWidget from "../../components/drive/WheelTelemetryWidget/WheelTelemetryWidget.tsx";
import DriveSpeedWidget from "../../components/drive/DriveSpeedWidget/DriveSpeedWidget.tsx";
import TOFHeight from "../../components/science/AnalysisPlatformHeight/AnalysisPlatformHeight.tsx";
import KilnWidget from "../../components/science/KilnWidget/KilnWidget.tsx";
import MicroscopeWidget from "../../components/science/MicroscopeThresholdWidget/MicroscopeWidget.tsx";
import EffortWidget from "../../components/science/EffortWidget/EffortWidget.tsx";
import { RosService } from "../../ros/services/rosService.ts";
import { RosTopic } from "../../ros/topics/rosTopic.ts";
import { RootState } from "../../redux/RootState.ts";

export const ARCMicroscopeView = () => {
  return (
    <div className="p-3 max-h-full">
      <div className="grid grid-flow-col auto-cols-fr justify-between items-stretch gap-3 ">
        <div className="flex flex-col flex-grow gap-3 col-span-3">
          <MicroscopeWidget cameraSerial="science_microscope" />
          <div className="grid grid-cols-[1fr_1.1fr] gap-3">
            <EffortWidget
              label="Water Pump"
              topic={RosTopic.WATER_PUMP_STATUS}
              service={RosService.WATER_PUMP_COMMAND}
              statusSelector={(state: RootState) => state.waterPumpStatus}
              storeName="waterPumpEffort" />
            <EffortWidget
              label="Diaphragm Pump"
              topic={RosTopic.DIAPHRAGM_PUMP_STATUS}
              service={RosService.DIAPHRAGM_PUMP_COMMAND}
              statusSelector={(state: RootState) => state.diaphragmPumpStatus}
              storeName="diaphragmPumpEffort" />
          </div>
        </div>
        <div className="flex flex-col gap-3 col-span-3">
          <KilnWidget />
          <DriveModeWidget />
          <WheelTelemetryWidget />
          <DriveSpeedWidget />
          <TOFHeight />
        </div>
      </div>
    </div>
  );
};
