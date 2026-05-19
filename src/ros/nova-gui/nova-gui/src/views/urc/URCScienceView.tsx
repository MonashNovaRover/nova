import React, {useState} from "react";
import HydroprobeWidget from "../../components/science/SensorWidgets/HydroprobeWidget.tsx";
import PumpsWidget from "../../components/science/PumpsWidget/PumpsWidget";
import BMESensor from "../../components/science/SensorWidgets/BMESensor.tsx";
import URCNIRProbeWidget from "../../components/science/NIRProbe/URCNIRProbeWidget.tsx";
import CarouselWidgetV2 from "../../components/science/CarouselWidget/CarouselWidget.tsx";
import {CarouselPositionProvider} from "../../components/science/CarouselWidget/CarouselPositionContext.tsx";
import SegmentedPicker from "../../components/shared/components/SegmentedPicker/SegmentedPicker.tsx";
import SerialMappedCameraComponent from "../shared/CamerasPage/SerialMappedCameraComponent.tsx";
import URCUVVisSpecView from "./URCUVVisSpecView.tsx";
import HeaterWidget from "../../components/science/ThermalControl/HeaterWidget.tsx";
import {CameraControlModalButton} from "../../components/cameras/CameraPage/CameraControlModelButton.tsx";
import {CameraSerials} from "../shared/CamerasPage/CameraViewConstants.tsx";
import LitmusDipperWidget from "../../components/science/LitmusDipperWidget/LitmusDipperWidget.tsx";
import LedWidget from "../../components/science/LEDWidget/LEDWidget.tsx";
import CacheControlWidget from "../../components/science/CacheControlWidget/CacheControlWidget.tsx";

const URCScienceView: React.FC = () => {
  const [selectedTab, setSelectedTab] = useState(0)

  const siteAnalysisView = (
    <div className="grid grid-flow-col auto-cols-fr gap-3 p-3 overflow-auto flex-1 min-h-0">
      <div className="flex flex-col gap-3 col-span-2">
        <URCNIRProbeWidget/>
      </div>

      <div className="flex flex-col gap-3 col-span-2">
        <HydroprobeWidget/>
        <BMESensor/>
        <LitmusDipperWidget/>
        <CacheControlWidget/>
        <HeaterWidget/>
      </div>

      <div className="flex flex-col gap-3 col-span-2">
        <SerialMappedCameraComponent cameraSerial={CameraSerials.SCIENCE_MICROSCOPE}/>
        <CameraControlModalButton/>
      </div>
    </div>
  )

  /**
   * CarouselPositionProvider shares the carousel's current cuvette positions
   * with child components. This enables UVVisSpec to auto-populate graph names
   * based on which cuvette is currently under the spectrometer.
   *
   * The context respects the "Use Manual Position" toggle - it reflects what
   * the GUI shows, not necessarily the physical carousel feedback.
   */
  const VisSpecView = (
    <CarouselPositionProvider>
      <div className="grid grid-flow-col auto-cols-fr gap-3 p-3 overflow-auto flex-1 min-h-0">
        <div className="flex flex-col gap-3 col-span-3">
          <URCUVVisSpecView/>
        </div>

        <div className="flex flex-col gap-3 col-span-2">
          <CarouselWidgetV2/>
        </div>

        <div className="flex flex-col gap-3 col-span-3">
          <PumpsWidget/>
          <LedWidget/>
          <HeaterWidget/>
          <SerialMappedCameraComponent cameraSerial={CameraSerials.URC_SCIENCE_CUVETTE}/>
          <SerialMappedCameraComponent cameraSerial={CameraSerials.URC_SCIENCE_UV_VIS}/>
          <CameraControlModalButton/>
        </div>
      </div>
    </CarouselPositionProvider>
  )

  return (
    <div className="flex flex-col overflow-hidden" style={{ height: "calc(100vh - 4.05rem)" }}>
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
