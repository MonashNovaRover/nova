import React from "react";
import HydroprobeWidget from "../../components/HydroprobeWidget/HydroprobeWidget";
import CarouselWidget from "../../components/CarouselWidget/CarouselWidget";
import TOFHeight from "../../components/AnalysisPlatformHeight/AnalysisPlatformHeight";
import PumpsWidget from "../../components/PumpsWidget/PumpsWidget";
import BMESensor from "../../components/BMESensor/BMESensor";
import WheelTelemetryWidget from "../../components/WheelTelemetryWidget/WheelTelemetryWidget";
import DriveModeWidget from "../../components/DriveModeWidget/DriveModeWidget";
import DriveSpeedWidget from "../../components/DriveSpeedWidget/DriveSpeedWidget";
import GenericSetBoolWidget from "../../components/GenericSetBoolWidget/GenericSetBoolWidget.tsx";
import {RosService} from "../../ros/services/rosService.ts";

const URCScienceView: React.FC = () => {
  return (
    <div className="grid grid-flow-col auto-cols-fr gap-3 m-3">
      <div className="flex flex-col gap-3 col-span-2">
        <HydroprobeWidget/>
        <PumpsWidget/>
      </div>

      <div className="flex flex-col gap-3 col-span-2">
        <div className="flex flex-row gap-3">
          <TOFHeight className="grow"/>
          <BMESensor className="grow"/>
        </div>
        <CarouselWidget/>
        <div className="flex flex-row gap-3">
          <GenericSetBoolWidget className="w-full" label="Cache" service={RosService.CACHE}/>
          <GenericSetBoolWidget className="w-full" label="Heater" service={RosService.HEATER}/>
          <GenericSetBoolWidget className="w-full" label="Mixers" service={RosService.MIXERS}/>
        </div>
      </div>

      <div className="flex flex-col gap-3 col-span-2">
        <DriveModeWidget/>
        <WheelTelemetryWidget/>
        <DriveSpeedWidget/>
      </div>
    </div>
  );
};

export default URCScienceView;
