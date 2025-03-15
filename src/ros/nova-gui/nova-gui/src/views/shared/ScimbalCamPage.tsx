import {GimbalOverlayedCameraComponent} from "../../components/CameraComponent/special/GimbalOverlayedCameraComponent.tsx";
import {CameraSerials} from "./CamerasPage/CameraPageConstants";

export default function ScimbalCamView () {

  return (
    <div className="p-3">
      <GimbalOverlayedCameraComponent cameraSerial={CameraSerials.SCIENCE_GIMBAL}/>
    </div>
  );
}