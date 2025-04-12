import React from "react";
import PlatformWidget from "../../components/PlatformWidget/PlatformWidget";
import HydroprobeWidget from "../../components/HydroprobeWidget/HydroprobeWidget";
import CarouselWidget from "../../components/CarouselWidget/CarouselWidget";
import PumpsWidget from "../../components/PumpsWidget/PumpsWidget";
import BMESensor from "../../components/BMESensor/BMESensor";
import WheelTelemetryWidget from "../../components/WheelTelemetryWidget/WheelTelemetryWidget";
import DriveModeWidget from "../../components/DriveModeWidget/DriveModeWidget";
import DriveSpeedWidget from "../../components/DriveSpeedWidget/DriveSpeedWidget";
import GenericSetBoolWidget from "../../components/GenericSetBoolWidget/GenericSetBoolWidget.tsx";
import {RosService} from "../../ros/services/rosService.ts";
import DepthSensor from "../../components/DepthSensor/DepthSensor.tsx";


const URCScienceView: React.FC = () => {
  return (
    <div className="grid w-full h-full gap-3 p-3 grid-cols-6">
      <HydroprobeWidget className="row-start-1 w-full col-span-2 row-span-1"/>
      <DepthSensor className="row-start-1 w-full col-span-1 row-span-1"/>
      <BMESensor className="row-start-1 w-full col-span-1 row-span-1"/>
      <PlatformWidget className="row-start-2 w-full col-span-2 row-span-2"/>
      <CarouselWidget className="row-start-2 w-full col-span-2 row-span-2"/>
      <PumpsWidget className="row-start-4 w-full col-span-2 row-span-2"/>
      <DriveModeWidget className="row-start-1 w-full col-span-2 row-span-1"/>
      <WheelTelemetryWidget className="row-start-2 w-full col-span-2"/>
      <DriveSpeedWidget className="row-start-3 col-start-5 w-full col-span-2 row-span-1"/>
      <GenericSetBoolWidget className="row-start-4 w-full col-span-1 row-span-1" label="Cache" service={RosService.CACHE}/>
      <GenericSetBoolWidget className="row-start-4 w-full col-span-1 row-span-1" label="Heater" service={RosService.HEATER}/>
    </div>
  );
};

export default URCScienceView;
