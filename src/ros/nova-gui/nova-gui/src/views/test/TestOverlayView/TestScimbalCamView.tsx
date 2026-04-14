// TestScimbalCamView.tsx
import {GimbalOverlayedCameraComponent} from "../../../components/cameras/CameraComponent/special/GimbalOverlayedCameraComponent.tsx";
import {CameraSerials} from "../../shared/CamerasPage/CameraViewConstants.tsx";

export default function TestScimbalCamView () {

  return (
    <div className="p-3">
      <GimbalOverlayedCameraComponent cameraSerial={CameraSerials.SCIENCE_GIMBAL}/>
    </div>
  );
}