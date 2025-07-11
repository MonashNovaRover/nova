import {CameraComponent} from "../../cameras/CameraComponent/CameraComponent.tsx";
import {CameraSerials} from "../../../views/shared/CamerasPage/CameraPageConstants.tsx";
import SiteSelectWidget from "../SiteSelectWidget/SiteSelectWidget.tsx";
import SerialMappedCameraComponent from "../../../views/shared/CamerasPage/SerialMappedCameraComponent.tsx";

/**
 * Widget for microscope camera feed and ML output
 * @constructor
 */
function MicroscopeCamerasWidget () {
  return (
    <div className="p-0">
      <CameraComponent cameraSerial={CameraSerials.SCIENCE_MICROSCOPE}/>
      <SiteSelectWidget pickerClassName="mt-4 mb-1"/>
      <div className="grid grid-cols-2 gap-3 mt-3">
        <SerialMappedCameraComponent cameraSerial={CameraSerials.SCIENCE_GIMBAL}/>
        <SerialMappedCameraComponent cameraSerial={CameraSerials.SCIENCE_KILN_BOTTOM}/>
      </div>
    </div>
  )
}

export default MicroscopeCamerasWidget;
