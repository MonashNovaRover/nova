import React, {useState} from "react";
import HydroprobeWidget from "../../components/science/HydroprobeWidget/HydroprobeWidget";
import PumpsWidget from "../../components/science/PumpsWidget/PumpsWidget";
import BMESensor from "../../components/science/BMESensor/BMESensor";
import URCNIRProbeWidget from "../../components/science/NIRProbe/URCNIRProbeWidget.tsx";
import CarouselWidgetV2 from "../../components/science/CarouselWidget/CarouselWidget.tsx";
import SegmentedPicker from "../../components/shared/components/SegmentedPicker/SegmentedPicker.tsx";
import SerialMappedCameraComponent from "../shared/CamerasPage/SerialMappedCameraComponent.tsx";
import URCUVVisSpecView from "./URCUVVisSpecView.tsx";
import HeaterWidget from "../../components/science/ThermalControl/HeaterWidget.tsx";
import {CameraControlModalButton} from "../../components/cameras/CameraPage/CameraControlModelButton.tsx";
import {CameraSerials} from "../shared/CamerasPage/CameraViewConstants.tsx";
import LitmusDipperWidget from "../../components/science/LitmusDipperWidget/LitmusDipperWidget.tsx";
import LedWidget from "../../components/science/LEDWidget/LEDWidget.tsx";

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
        <LitmusDipperWidget/>
        <HeaterWidget/>
      </div>

      <div className="flex flex-col gap-3 col-span-2">
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
      </div>

      <div className="flex flex-col gap-3 col-span-2">
        <HeaterWidget/>
        <LedWidget/>
        <SerialMappedCameraComponent cameraSerial={CameraSerials.URC_SCIENCE_CUVETTE}/>
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
  )
};

export default URCScienceView;
