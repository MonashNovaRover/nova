import {CameraSerials} from "../../../views/shared/CamerasPage/CameraPageConstants.tsx";
import SiteSelectWidget from "../SiteSelectWidget/SiteSelectWidget.tsx";
import SerialMappedCameraComponent from "../../../views/shared/CamerasPage/SerialMappedCameraComponent.tsx";
import MicroscopeScaleOverlayedCameraComponent from "../../cameras/CameraComponent/special/MicroscopeScaleOverlayedCameraComponent.tsx";

/**
 * Widget for microscope camera feed and ML output
 * @constructor
 */
function MicroscopeCamerasWidget () {
  return (
    <div className="p-0">
      <MicroscopeScaleOverlayedCameraComponent cameraSerial={CameraSerials.SCIENCE_MICROSCOPE}/>
      <SiteSelectWidget pickerClassName="mt-4 mb-1"/>
      <div className="grid grid-cols-2 gap-3 mt-3">
        <SerialMappedCameraComponent cameraSerial={CameraSerials.SCIENCE_GIMBAL}/>
        <SerialMappedCameraComponent cameraSerial={CameraSerials.SCIENCE_ANALYSIS_ARM_DOWN}/>
      </div>
    </div>
  )
}

export default MicroscopeCamerasWidget;
