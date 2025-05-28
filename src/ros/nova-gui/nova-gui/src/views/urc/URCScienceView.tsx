import React, {useState} from "react";
import HydroprobeWidget from "../../components/HydroprobeWidget/HydroprobeWidget";
import PumpsWidget from "../../components/PumpsWidget/PumpsWidget";
import BMESensor from "../../components/BMESensor/BMESensor";
import {RosService} from "../../ros/services/rosService.ts";
import URCNIRProbeWidget from "../../components/NIRProbe/URCNIRProbeWidget.tsx";
import CacheControlWidget from "../../components/science/CacheControlWidget/CacheControlWidget.tsx";
import CarouselWidgetV2 from "../../components/science/CarouselWidget/CarouselWidget.tsx";
import SegmentedPicker from "../../components/SegmentedPicker/SegmentedPicker.tsx";
import SerialMappedCameraComponent from "../shared/CamerasPage/SerialMappedCameraComponent.tsx";
import {CameraSerials} from "../shared/CamerasPage/CameraPageConstants.tsx";
import URCUVVisSpecView from "./URCUVVisSpecView.tsx";
import HeaterWidget from "../../components/science/HeaterWidget/HeaterWidget.tsx";
import {CameraControlModalButton} from "../../components/CameraComponent/components/CameraControlModelButton.tsx";


const URCScienceView: React.FC = () => {
  const [selectedTab, setSelectedTab] = useState(0)

  const siteAnalysisView = (
    <div className="grid grid-flow-col auto-cols-fr gap-3 m-3">
      <div className="flex flex-col gap-3 col-span-2">
        <URCNIRProbeWidget/>
      </div>

      <div className="flex flex-col gap-3 col-span-2">
        <HydroprobeWidget/>
        <BMESensor/>
        <div className="flex flex-row gap-3">
          <CacheControlWidget className="w-full" label="Left Cache" service={RosService.CACHE_2}/>
          <CacheControlWidget className="w-full" label="Right Cache" service={RosService.CACHE_1}/>
        </div>
        <HeaterWidget/>
      </div>

      <div className="flex flex-col gap-3 col-span-2">
        <SerialMappedCameraComponent cameraSerial={CameraSerials.SCIENCE_GIMBAL}/>
        <SerialMappedCameraComponent cameraSerial={CameraSerials.SCIENCE_MICROSCOPE}/>
        <CameraControlModalButton/>
      </div>
    </div>
  )

  const VisSpecView = (
    <div className="grid grid-flow-col auto-cols-fr gap-3 m-3">
      <div className="flex flex-col gap-3 col-span-2">
        <URCUVVisSpecView/>
      </div>

      <div className="flex flex-col gap-3 col-span-2">
        <PumpsWidget/>
        <CarouselWidgetV2/>
        <HeaterWidget/>
      </div>

      <div className="flex flex-col gap-3 col-span-2">
        <SerialMappedCameraComponent cameraSerial={CameraSerials.URC_SCIENCE_CUVETTE}/>
        <SerialMappedCameraComponent cameraSerial={CameraSerials.URC_SCIENCE_UV_VIS}/>
        <CameraControlModalButton/>
      </div>
    </div>
  )

  return (
    <div>
      <SegmentedPicker
        selectedIndex={selectedTab}
        onIndexChange={setSelectedTab}
        children={[
          "Site Analysis", "Vis Spec"
        ]}
        color="primary"
        className="pb-0"
        fullWidth
        variant="bordered"
      />

      {selectedTab === 0 && siteAnalysisView}

      {selectedTab === 1 && VisSpecView}

    </div>

  );
};

export default URCScienceView;
